#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define NMAX_PROCS 100

typedef struct {
	float t0, dt, deadline, tempoRodada;
	char nome[50];
	int p, id;
} PROCESS;

typedef struct node* Link;

typedef struct node {
	int id;
	Link next;
} Node;


/********************** VARIAVEIS GLOBAIS ****************************/
int numEscalonamento, nCores = 0, debug = 0, nProcs = 0, deadProc = 0;
PROCESS tabelaProcessos[NMAX_PROCS];
FILE* arqEntrada, *arqSaida;
struct timeval inicio;
float quantum = 1.0;
Link head, tail;

sem_t semCore, semThread[NMAX_PROCS];
sem_t semQueue, mutexQueue;

/*=============================== ASSINATURA DAS FUNCOES =======================================*/

/************* INICIALIZACAO ******************/
void parserArgumentosEntrada(int argc, char* argv[]);
void leArquivoEntrada();
void inicializacao();
/*************  UTILS ******************/
int compare_arrive(const void *a,const void *b);
float tempoDesdeInicio(struct timeval inicio);
void* mallocSeguro(size_t bytes);
void imprimeSaida();
/*************  QUEUE ******************/
void initQueue();
void insertOrderedByArrivedQueue(int id);
void insertOrderedByJobQueue(int id);
void insertQueue(int id);
int removeQueue();
void removeNextQueue(Link p);
int emptyQueue();
void printQueue();
Link getNextQueue(Link p);
void decreaseQuantumNextQueue();
/*************  THREADS ******************/
void *Processo(void *a);
void *Escalonador(void *a);
void roundRobin(int id);
void FCFS_SJF(int id);
void Operacao(int id, int cont);
/*====================================== MAIN ==================================================*/

int main(int argc, char* argv[]) {
	gettimeofday(&inicio, NULL);
	
	pthread_t procs[NMAX_PROCS];
	pthread_t escalonador;
	
	long i;
	int pos;

	parserArgumentosEntrada(argc, argv);
	leArquivoEntrada();
	inicializacao();
	initQueue();

	qsort(tabelaProcessos, nProcs, sizeof(PROCESS), compare_arrive);
	
	for(pos = 0; pos < nProcs; pos++){
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

        if(numEscalonamento == 1 || numEscalonamento == 4){
			insertOrderedByArrivedQueue(i);
		}
		else if(numEscalonamento == 2){
			insertOrderedByJobQueue(i);
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

	fclose(arqEntrada);
	fclose(arqSaida);

	return 0;
}

/*====================================== FUNCOES ==================================================*/

/*************** THREADS **********************/
void *Processo(void *a) {
	int* id = (int*) a;

	if(numEscalonamento == 1 || numEscalonamento == 2) {
		FCFS_SJF(*id);
	}
	else if(numEscalonamento == 4) {
		roundRobin(*id);
	}
	return NULL;
}

void roundRobin(int id) {
	
	while (tempoDesdeInicio(inicio) < tabelaProcessos[id].deadline &&
		tabelaProcessos[id].dt > 0)
	{
		sem_wait(&semThread[id]);
	
		struct timeval inicioProcesso;
		gettimeofday(&inicioProcesso, NULL);	//define o tempo inicial a cada vez que 
												// eh escalonado
		int cont = 0;
		while ((tempoDesdeInicio(inicio) < tabelaProcessos[id].deadline) &&
			  (tempoDesdeInicio(inicioProcesso) < tabelaProcessos[id].tempoRodada)) 
		{
			Operacao(id, cont);
			cont++;					
		}
		printf("\n");
		sem_post(&semCore);
	}
}

void *Escalonador(void *a) {
	if (numEscalonamento == 1 || numEscalonamento == 2) {
		while (deadProc < nProcs) {
			sem_wait(&semQueue);
			
			sem_wait(&semCore);
			
			Link next = getNextQueue(head);
			tabelaProcessos[next->id].tempoRodada =tabelaProcessos[next->id].dt;

			sem_post(&semThread[next->id]);
			
			removeNextQueue(head);
			
			sem_post(&semQueue);

			deadProc++;
		} 
	}

	else if (numEscalonamento == 4) {

		while (deadProc < nProcs) {
			//printf("deadProc = %d\n", deadProc);
			sem_wait(&semQueue);
		
			sem_wait(&semCore);
			
			decreaseQuantumNextQueue();
		} 
	}
	return NULL;
}

void Operacao(int id, int cont) {
	if(cont < 3) {
		printf("[Operação] %s : %d\n", tabelaProcessos[id].nome, cont);
	}
}

/************************** QUEUE *****************************/
void decreaseQuantumNextQueue() {
	int id = removeQueue(); 
	
	// fila não está vazia
	if (id != -1) { 
		// definimos tempo do run da proxima rodada
		if (tabelaProcessos[id].dt >= quantum) {
			tabelaProcessos[id].tempoRodada = quantum;	
		} 
		else {
			tabelaProcessos[id].tempoRodada = tabelaProcessos[id].dt;
		}
		
		tabelaProcessos[id].dt -= tabelaProcessos[id].tempoRodada; // processo ainda vai rodar
		
		if (tabelaProcessos[id].dt > 0 && tempoDesdeInicio(inicio) < tabelaProcessos[id].deadline) {	
			insertQueue(id);   // processo continua pra proxima rodada
			sem_post(&semQueue);
		}
		else {
			// deixa o processo removido e contabiliza qtos processos terminaram
			sem_wait(&mutexQueue);
			deadProc++;
			sem_post(&mutexQueue);
		}
		
		sem_post(&semThread[id]); // sinaliza o próximo processo da fila
	}
}

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
		head->next = nextHead->next;
		nextHead->next = NULL;
		free(nextHead);
	}
	
	sem_post(&mutexQueue);
	
	return content;
}

void insertQueue(int id) {
	insertOrderedByArrivedQueue(id);
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

void printQueue() {
	Link aux;

	printf("Fila:  ");
	sem_wait(&mutexQueue);
	
	for (aux = head->next; aux != tail; aux = aux->next) {
		printf("%s   ", tabelaProcessos[aux->id].nome);
	}
	printf("%s\n", tabelaProcessos[tail->id].nome);

	sem_post(&mutexQueue);
}

// A fila não pode estar vazia para chamar a função
Link getNextQueue(Link p) {
	sem_wait(&mutexQueue);
	Link next = p->next; 
	sem_post(&mutexQueue);
	
	return next;
}

/************* INICIALIZACAO ******************/
void leArquivoEntrada() {
	int i = 0;

	while (fscanf(arqEntrada,"%f %s %f %f %d", &tabelaProcessos[i].t0, tabelaProcessos[i].nome, 
		&tabelaProcessos[i].dt, &tabelaProcessos[i].deadline, &tabelaProcessos[i].p) != EOF) {		
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
		
		arqSaida = fopen(argv[3], "w");;

		// descobre a qtde de nucleos do computador
		nCores = sysconf(_SC_NPROCESSORS_ONLN);
		printf("Escalonador: %s\n", argv[1]);
		printf("Arquivo de entrada: %s\n", argv[2]);
		printf("Arquivo de saida: %s\n", argv[3]);
		printf("# cores: %d\n\n", nCores);

		if(argc >= 5 && argv[4][0] == 'd' && argv[4][1] == '\0') {
			debug = 1;
			printf("Opção debug ativada.\n");
		}
	}
	else {
		printf("Formato esperado:\n./ep1 <num_escalonador (1-6)> <arq_entrada> <arq_saida> [d]\n");
		exit(-2);
	}
}

void inicializacao() {
	long i;

	if (sem_init(&semCore, 0, 1)) {
		fprintf(stderr, "ERRO ao criar semaforo semCore\n");
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

/************* UTILS ******************/
int compare_arrive(const void *a,const void *b) {
	PROCESS *x = (PROCESS *) a;
	PROCESS *y = (PROCESS *) b;

	if (x->t0 < y->t0) return -1;
	else if (x->t0 > y->t0) return 1; 

	return 0;
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

void imprimeSaida() {
	int i;

	for (i = 0; i < nProcs; i++) {
		// fprintf(arqSaida ,"Nome: %s\n", tabelaProcessos[i].nome);
		// fprintf(arqSaida ,"t0: %f\n", tabelaProcessos[i].t0);
		// fprintf(arqSaida ,"dt: %f\n", tabelaProcessos[i].dt);
		// fprintf(arqSaida ,"deadline: %f\n", tabelaProcessos[i].deadline);
		// fprintf(arqSaida ,"p: %d\n\n", tabelaProcessos[i].p);
		printf("id: %d\n", tabelaProcessos[i].id);
	}
}

void FCFS_SJF(int id){
	struct timeval inicioProcesso;
	int tmp1 = 1, tmp2 = 1;
	int cont = 0;
	
	sem_wait(&semThread[id]);	
	gettimeofday(&inicioProcesso, NULL);

	while ((tmp2 = (tempoDesdeInicio(inicio) < tabelaProcessos[id].deadline)) &&
		(tmp1 = (tempoDesdeInicio(inicioProcesso) < tabelaProcessos[id].tempoRodada)) ) 
	{
			Operacao(id, cont);
			cont++;
	}
	
	if (!tmp1 && tmp2)
		printf("Sai por causa do dt: %s\n\n", tabelaProcessos[id].nome);
	else if (tmp1 && !tmp2)
		printf("Sai por culpa do deadline: %s\n\n", tabelaProcessos[id].nome);

	sem_post(&semCore);
}
