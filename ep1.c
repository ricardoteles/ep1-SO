#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define NMAX_PROCS 100

typedef struct {
	float t0, dt, deadline, tempoRodada, tempoJob, tempoUso;
	char nome[50];
	int p, id;
} PROCESS;

typedef struct node* Link;

typedef struct node {
	int id;
	Link next;
} Node;

/*=============================== VARIÁVEIS GLOBAIS =======================================*/
PROCESS tabelaProcessos[NMAX_PROCS];

FILE* arqEntrada, *arqSaida;
Link head, tail;

struct timeval inicio;
int numEscalonamento, nCores = 0, debug = 0;
int nProcs = 0, deadProc = 0, mudancaContexto = 0; 


int qtdadeChegaram = 0; // qtd de processos que vão chegando durante 
						// a execução do escalonador SRTN;
						// os processos iniciais na fila não contam
float quantum = 0.2;

sem_t semCore, semThread[NMAX_PROCS];
sem_t semQueue, mutexQueue; // este ultimo para exclusão mutua da fila de processos
sem_t semTroca;	


/*=============================== ASSINATURA DAS FUNCOES ===================================*/

/*========================== UTILS ===============================*/
void parserArgumentosEntrada(int argc, char* argv[]);
void leArquivoEntrada();
void inicializaSemaforos();
float tempoDesdeInicio(struct timeval inicio);
void* mallocSeguro(size_t bytes);
/*======================== ESCALONAMENTO =========================*/
void decreaseQuantumNext();
int compare_arrive(const void *a,const void *b);
void *Escalonador(void *a);
/*========================  QUEUE ==============================*/
void initQueue();
void insertOrderedByArrivedQueue(int id);
void insertOrderedByJobQueue(int id);
int removeQueue();
void removeNextQueue(Link p);
int emptyQueue();
Link getNextQueue(Link p);
/*========================  PROCESSO ==============================*/
void *Processo(void *a);
void RoundRobin(int id, float deadline);
void FCFS_SJF(int id, float dt, float deadline);
void SRTN(int id, float deadline);
void Operacao();


/*====================================== MAIN ==================================================*/
int main(int argc, char* argv[]) {
	gettimeofday(&inicio, NULL);
	
	pthread_t procs[NMAX_PROCS];
	pthread_t escalonador;
	
	long i;
	int pos;

	parserArgumentosEntrada(argc, argv);
	leArquivoEntrada();
	inicializaSemaforos();
	initQueue();

	qsort(tabelaProcessos, nProcs, sizeof(PROCESS), compare_arrive);
	
	for(pos = 0; pos < nProcs; pos++) {
		tabelaProcessos[pos].id = pos;	
	}

	if (pthread_create(&escalonador, NULL, Escalonador, (void *) NULL)) {
        printf("\n ERROR creating thread Escalonador\n");
        exit(1);
    }

	for (i = 0; i < nProcs; i++) {
		while (tempoDesdeInicio(inicio) < tabelaProcessos[i].t0) usleep(50000);
		
		if (pthread_create(&procs[i], NULL, Processo, (void *) &tabelaProcessos[i].id)) {
            printf("\n ERROR creating thread procs[%ld]\n", i);
            exit(1);
        }

        if (debug) {
        	fprintf(stderr, "[t0=%.2f nome=%s dt=%.2f deadline=%.2f p=%d chegou]\n", tabelaProcessos[i].t0, tabelaProcessos[i].nome,
        		tabelaProcessos[i].dt, tabelaProcessos[i].deadline, tabelaProcessos[i].p);
        }

        if(numEscalonamento == 1 || numEscalonamento == 4) {
			insertOrderedByArrivedQueue(i);
		}
		else if(numEscalonamento == 2 || numEscalonamento == 3) {
			insertOrderedByJobQueue(i);
			qtdadeChegaram++;
		}

		/* sinaliza que chegou processo na fila */
		sem_post(&semQueue);
	}
	
	for (i = 0; i < nProcs; i++) {
        if (pthread_join(procs[i], NULL)) {
			printf("\n ERROR joining thread procs[%ld]\n", i);
			exit(1);
        }
    }

    if (pthread_join(escalonador, NULL)) {
		printf("\n ERROR joining thread escalonador\n");
		exit(1);
    }

    if (debug) {
		fprintf(stderr, "\n# mudanças de contexto: %d\n", mudancaContexto);
	}

    fprintf(arqSaida, "%d", mudancaContexto);

	fclose(arqEntrada);
	fclose(arqSaida);

	return 0;
}

/*==========================  ESCALONAMENTO =====================================*/
int compare_arrive(const void *a,const void *b) {
	PROCESS *x = (PROCESS *) a;
	PROCESS *y = (PROCESS *) b;

	if (x->t0 < y->t0) return -1;
	else if (x->t0 > y->t0) return 1; 
}

void *Escalonador(void *a) {
	if (numEscalonamento == 1 || numEscalonamento == 2) {
		while (deadProc < nProcs) {
			sem_wait(&semQueue);
			
			sem_wait(&semCore); // quer Core pra mandar alguém executar
			
			Link next = getNextQueue(head);

			sem_post(&semThread[next->id]);
			
			removeNextQueue(head);
			
			deadProc++;
		}
	}
	else if (numEscalonamento == 3) {
		while (deadProc < nProcs) {
			sem_wait(&semQueue);
			
			sem_wait(&semCore);
			
			Link next = getNextQueue(head);
			
			sem_post(&semThread[next->id]);

			// espera chegar processos atrasados
			sem_wait(&semTroca); 			

			if(tabelaProcessos[next->id].dt <= 0 || tabelaProcessos[next->id].deadline < tempoDesdeInicio(inicio)) {
				removeNextQueue(head);
				deadProc++;
			}
			else {
				sem_post(&semQueue); // sinaliza pro escalonador que continua na fila
			}
		}
	}
	else if (numEscalonamento == 4) {

		while (deadProc < nProcs) {
			sem_wait(&semQueue); // espera a fila não estar vazia
		
			sem_wait(&semCore); 
			
			decreaseQuantumNext();
		} 
	}
	return NULL;
}

// decrementa quantum da proxima rodada e manda executar
void decreaseQuantumNext() {
	int id;
	float tempoRodada;

	id = removeQueue(); 
	
	// fila não estava vazia
	if (id != -1) { 

		// definimos tempo do run da rodada atual
		if (tabelaProcessos[id].dt >= quantum) {
			tempoRodada = quantum;	
		} 
		else if (tabelaProcessos[id].dt > 0) {
			tempoRodada = tabelaProcessos[id].dt;
		}
		else {
			tempoRodada = 0; // condição de parada
		}
		
		tabelaProcessos[id].tempoRodada = tempoRodada;
		tabelaProcessos[id].dt -= tempoRodada; // decrementa do total
		
		if (tabelaProcessos[id].tempoRodada > 0 && tempoDesdeInicio(inicio) < tabelaProcessos[id].deadline) {	
			insertOrderedByArrivedQueue(id);   // move o mesmo processo pro fim da fila, mesmo se dt == 0
			mudancaContexto++;
			sem_post(&semQueue); 
		}
		else {
			// deixa o processo removido e contabiliza qtos processos terminaram
			sem_wait(&mutexQueue);
			deadProc++;
			sem_post(&mutexQueue);
		}
		
		// sinaliza o processo que foi removido da fila pra rodar ou retomar execução
		sem_post(&semThread[id]); 
	}
}

/*============================== PROCESSO =====================================*/
void *Processo(void *a) {
	int* id1 = (int*) a;
	int id = (*id1);

	if(numEscalonamento == 1 || numEscalonamento == 2) {
		FCFS_SJF(id, tabelaProcessos[id].dt, tabelaProcessos[id].deadline);
	}
	else if(numEscalonamento == 3) {
		SRTN(id, tabelaProcessos[id].deadline);
	}
	else if(numEscalonamento == 4) {
		RoundRobin(id, tabelaProcessos[id].deadline);
	}

	return NULL;
}

// função chamada pelo processo no escalonamento 4
void RoundRobin(int id, float deadline) {
	struct timeval inicioProcesso;
	float tempoFim;
	
	do {
		sem_wait(&semThread[id]);

		if (debug) {
			fprintf(stderr, "Processo %s começou a usar a CPU.\n", tabelaProcessos[id].nome);
		}
	
		gettimeofday(&inicioProcesso, NULL);	//define o tempo inicial a cada vez que 
												// eh escalonado

		while ((tempoDesdeInicio(inicio) < deadline) &&
			  (tempoDesdeInicio(inicioProcesso) < tabelaProcessos[id].tempoRodada)) 
		{
			Operacao();					
		}
		
		sem_post(&semCore);

		if (debug) {
			fprintf(stderr, "Processo %s liberou a CPU.\n", tabelaProcessos[id].nome);
		}
	
	} while (tempoDesdeInicio(inicio) < deadline && tabelaProcessos[id].tempoRodada > 0);

	tempoFim = tempoDesdeInicio(inicio); // informação de saída

	fprintf(arqSaida, "%s %.3f %.3f\n", tabelaProcessos[id].nome, tempoFim, 
		tempoFim - tabelaProcessos[id].t0);

	if (debug) {
		fprintf(stderr, "[nome=%s tf=%.3f tr=%.3f terminou]\n", 
			tabelaProcessos[id].nome, tempoFim, tempoFim - tabelaProcessos[id].t0);
	}
}

// função chamada pelo processo no escalonamento 3
void SRTN(int id, float deadline) {
	struct timeval inicioProcesso;
	float tempoFim;
	int chegou; 

	while (tempoDesdeInicio(inicio) < deadline && tabelaProcessos[id].dt > 0) 
	{	
		sem_wait(&semThread[id]);
		
		if (debug) {
			fprintf(stderr, "Processo %s começou a usar a CPU.\n", tabelaProcessos[id].nome);
		}

		chegou = qtdadeChegaram; 

		gettimeofday(&inicioProcesso, NULL);	//define o tempo inicial a cada vez que 
												// eh escalonado

		// excedente de nós que chegaram mais recentemente é maior q zero ?
		while (qtdadeChegaram - chegou == 0 && 
			  tempoDesdeInicio(inicio) < deadline &&
			  tempoDesdeInicio(inicioProcesso) < tabelaProcessos[id].dt) 
		{
			Operacao();			
		}
		
		sem_wait(&mutexQueue);
		tabelaProcessos[id].dt -= tempoDesdeInicio(inicioProcesso); // define tempo restante
		sem_post(&mutexQueue);
		
		sem_post(&semCore);

		if (debug) {
			fprintf(stderr, "Processo %s liberou a CPU.\n", tabelaProcessos[id].nome);
		}

		if (tempoDesdeInicio(inicio) < deadline && tabelaProcessos[id].dt > 0){
			mudancaContexto++;
		}
		
		sem_post(&semTroca); // sinaliza pro escalonador que pode escalonar o 
							//  próximo da fila (a inserção ja ordena)  => mudei pra baixo
						
	}
	
	tempoFim = tempoDesdeInicio(inicio);

	fprintf(arqSaida, "%s %.3f %.3f\n", tabelaProcessos[id].nome, tempoFim, 
		tempoFim - tabelaProcessos[id].t0);

	if (debug) {
		fprintf(stderr, "[nome=%s tf=%.3f tr=%.3f terminou]\n", 
			tabelaProcessos[id].nome, tempoFim, tempoFim - tabelaProcessos[id].t0);
	}
}

// função chamada pelo processo no escalonamento 1 e 2
void FCFS_SJF(int id, float dt, float deadline) {
	struct timeval inicioProcesso;
	float tempoFim;
	
	sem_wait(&semThread[id]);	// aguarda autorização do escalonador pra poder rodar

	if (debug) {
		fprintf(stderr, "Processo %s começou a usar a CPU.\n", tabelaProcessos[id].nome);
	}

	gettimeofday(&inicioProcesso, NULL);

	while ((tempoDesdeInicio(inicioProcesso) < dt) 
		&& (tempoDesdeInicio(inicio) < deadline)) {
			Operacao();
	}

	sem_post(&semCore); // libera Core

	if (debug) {
		fprintf(stderr, "Processo %s liberou a CPU.\n", tabelaProcessos[id].nome);
	}

	tempoFim = tempoDesdeInicio(inicio);

	fprintf(arqSaida, "%s %.3f %.3f\n", tabelaProcessos[id].nome, tempoFim, 
		tempoFim - tabelaProcessos[id].t0);

	if (debug) {
		fprintf(stderr, "[nome=%s tf=%.3f tr=%.3f terminou]\n", 
			tabelaProcessos[id].nome, tempoFim, tempoFim - tabelaProcessos[id].t0);
	}	
}

// Definimos uma operação qualquer
void Operacao() {
	int x = 0;
	x++;
	if(x) x = 0;
}

/*============================== QUEUE =====================================*/
void removeNextQueue(Link p) {
	sem_wait(&mutexQueue);

	if(head->next != head) {
		Link aux = p->next;

		if (aux == tail) {
			tail = p;
		}

		p->next = aux->next;
		aux->next = NULL;
		free(aux);
	}

	sem_post(&mutexQueue);
}

void initQueue() {
	head = (Link) mallocSeguro(sizeof(Node));
	head->next = head;
	tail = head;
}

int removeQueue() {
	int content = -1;
	
	sem_wait(&mutexQueue);
		
	if (head->next != head) {
		Link nextHead = head->next;
		content = nextHead->id;
		
		if (nextHead->next == head) {
			tail = head;	
		}

		head->next = nextHead->next;
		nextHead->next = NULL;
		free(nextHead);
	}
	
	sem_post(&mutexQueue);
	
	return content;
}

void insertOrderedByArrivedQueue(int id) {
	Link novo = (Link) mallocSeguro(sizeof(Node));
	novo->id = id;
	sem_wait(&mutexQueue);
	novo->next = head;
	tail->next = novo;
	tail = novo;
	sem_post(&mutexQueue);
}

void insertOrderedByJobQueue(int id) {
	Link aux;
	Link novo = (Link) mallocSeguro(sizeof(Node));
	novo->id = id;

	sem_wait(&mutexQueue);
	
	for (aux = head; aux != tail; aux = aux->next) {
		if (tabelaProcessos[id].dt < tabelaProcessos[aux->next->id].dt) {
			novo->next = aux->next;
			aux->next = novo;
			break;
		}
	}
	if (aux == tail) {
		novo->next = head;
		aux->next = novo;
		tail = novo; 
	}

	sem_post(&mutexQueue);
}

int emptyQueue() {
	sem_wait(&mutexQueue);
	int condition = (head->next == head); 
	sem_post(&mutexQueue);
	
	return condition;
}

Link getNextQueue(Link p) {
	sem_wait(&mutexQueue);
	Link next = p->next; 
	sem_post(&mutexQueue);
	
	return next;
}

/*============================== UTILS =====================================*/
void leArquivoEntrada() {
	int i = 0;

	while (fscanf(arqEntrada,"%f %s %f %f %d", &tabelaProcessos[i].t0, tabelaProcessos[i].nome, 
		&tabelaProcessos[i].dt, &tabelaProcessos[i].deadline, &tabelaProcessos[i].p) != EOF) {		
		
		tabelaProcessos[i].tempoJob = tabelaProcessos[i].dt; 
		tabelaProcessos[i].tempoUso = 0; 
		i++;
	}

	nProcs = i;
}

void parserArgumentosEntrada(int argc, char* argv[]) {
	if (argc >= 4) {
		numEscalonamento = atoi(argv[1]);
		arqEntrada = fopen(argv[2], "r");

		if (!arqEntrada) {
			fprintf(stderr, "ERRO ao abrir o arquivo %s\n", argv[2]);
			exit(0);
		}  
		
		arqSaida = fopen(argv[3], "w");

		// descobre a qtde de nucleos do computador
		nCores = sysconf(_SC_NPROCESSORS_ONLN);
		
		if(argc >= 5 && argv[4][0] == 'd' && argv[4][1] == '\0') {
			debug = 1;
			printf("Opção debug ativada.\n\n");
		}
	}
	else {
		printf("Formato esperado:\n./ep1 <num_escalonador (1-6)> <arq_entrada> <arq_saida> [d]\n");
		exit(-2);
	}
}

void inicializaSemaforos() {
	long i;

	if (sem_init(&semCore, 0, nCores)) {
		fprintf(stderr, "ERRO ao criar semaforo semCore\n");
		exit(0);
	}

	if (sem_init(&semTroca, 0, 0)) {
		fprintf(stderr, "ERRO ao criar semaforo semTroca\n");
		exit(0);
	}

	if (sem_init(&semQueue, 0, 0)) {
		fprintf(stderr, "ERRO ao criar semaforo semQueue\n");
		exit(0);
	}

	if (sem_init(&mutexQueue, 0, 1)) {
		fprintf(stderr, "ERRO ao criar semaforo mutexQueue\n");
		exit(0);
	}

	for (i = 0; i < NMAX_PROCS; i++) {
		if (sem_init(&semThread[i], 0, 0)) {
			fprintf(stderr, "ERRO ao criar semaforo semThread[%ld]\n", i);
			exit(0);
		}
	}
}

float tempoDesdeInicio(struct timeval inicio) {
	struct timeval fim;
	float timedif;

	gettimeofday(&fim, NULL);
	timedif = (float)(fim.tv_sec - inicio.tv_sec);
	timedif += (float)(fim.tv_usec - inicio.tv_usec)/1000000;

	return timedif;
}

void* mallocSeguro(size_t bytes) {
	void* p = malloc(bytes);
	if (!p) {
		fprintf(stderr, "ERRO na alocação de memória!\n");
		exit(0);
	}
	return p;
}