#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define NMAX_PROCS 100

typedef struct {
	float t0, dt, deadline;
	char nome[50];
	int p, id;
} PROCESS;

typedef struct no {
	struct no* next;
	PROCESS proc;
} Node;


/********************** VARIAVEIS GLOBAIS ****************************/
int numEscalonamento, nCores = 0, debug = 0, nProcs = 0; 
FILE* arqEntrada, *arqSaida;
Node *head, *tail;
struct timeval inicio;
float quantum = 0.5;

sem_t semCore, semThread[NMAX_PROCS];
sem_t semQueue; 

PROCESS* buffer[NMAX_PROCS];

/************* ASSINATURA DAS FUNCOES ******************/
void parserArgumentosEntrada(int argc, char* argv[]);
void leArquivoEntrada(PROCESS listaProcessos[]);
int compare_arrive(const void *a,const void *b);
float tempoDesdeInicio(struct timeval inicio);
void imprime(PROCESS listaProcessos[]);
void* mallocSeguro(size_t bytes);
void inicializacao();

/*** QUEUE ***/
void initQueue();
void insertOrderedByArrivedQueue(PROCESS proc);
void insertOrderedByJobQueue(PROCESS proc);
void printQueue();
int emptyQueue();
Node* getNextQueue(Node* p);
void removeNextQueue(Node *p);
void decreaseQuantumNextQueue(Node* p);

/*** THREADS DO SIMULADOR ***/
void *Processo(void *a);
void *Escalonador(void *a);
void FCFS_SJF(PROCESS *val);
void roundRobin(PROCESS *val);

/*******************************************************/

int main(int argc, char* argv[]) {
	gettimeofday(&inicio, NULL);
	
	PROCESS listaProcessos[NMAX_PROCS];
	pthread_t procs[NMAX_PROCS];
	pthread_t escalonador;
	
	long i;
	int rear = 0;

	parserArgumentosEntrada(argc, argv);
	leArquivoEntrada(listaProcessos);
	inicializacao();
	initQueue();
	//imprime(listaProcessos);

	qsort(listaProcessos, nProcs, sizeof(PROCESS), compare_arrive);
	
	if (pthread_create(&escalonador, NULL, Escalonador, (void *) NULL)) {
        printf("\n ERROR creating thread Escalonador\n");
        exit(1);
    }

	for (i = 0; i < nProcs; i++) {
		while (tempoDesdeInicio(inicio) < listaProcessos[i].t0) usleep(50000);
		
		if (pthread_create(&procs[i], NULL, Processo, (void *) &listaProcessos[i])) {
            printf("\n ERROR creating thread procs[%ld]\n", i);
            exit(1);
        }

        if(numEscalonamento == 1){
			insertOrderedByArrivedQueue(listaProcessos[i]);
		}
		else if(numEscalonamento == 2){
			insertOrderedByJobQueue(listaProcessos[i]);
		}

		/* sinaliza que chegou um processo na fila */
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


/****************************** THREADS ******************************/

// TODO:
void *Processo(void *a) {
	PROCESS* val = (PROCESS*) a;

	if(numEscalonamento == 1 || numEscalonamento == 2) {
		FCFS_SJF(val);
	}

	if(numEscalonamento == 4){
		roundRobin(val);
	}

	return NULL;
}

void roundRobin(PROCESS *val){
	struct timeval inicioProcesso;
	float quantumThread = quantum;
	int cont = 0;
	
	while (tempoDesdeInicio(inicio) < val->deadline && val->dt > 0) {

		if(val->dt < quantum) quantumThread = val->dt; 
		
		sem_wait(&semThread[val->id]);	
		gettimeofday(&inicioProcesso, NULL);

		while((tempoDesdeInicio(inicio) < val->deadline) &&
			(tempoDesdeInicio(inicioProcesso) < quantumThread)){

			if(cont < 3)
				printf("[Operação] %s : %d\n", val->nome, cont);
			cont++;
		}

		sem_post(&semCore);

		val->dt -= quantumThread;	
	}
	

}

void FCFS_SJF(PROCESS *val){
	struct timeval inicioProcesso;
	int cont = 0, tmp1 = 1, tmp2 = 1;
	
	sem_wait(&semThread[val->id]);	
	gettimeofday(&inicioProcesso, NULL);

	while ((tmp1 = (tempoDesdeInicio(inicioProcesso) < val->dt)) 
		&& (tmp2 = (tempoDesdeInicio(inicio) < val->deadline))) {
		if(cont < 3)
			printf("[Operação] %s : %d\n", val->nome, cont);
		cont++;
	}
	
	if (!tmp1 && tmp2)
		printf("Sai por causa do dt: %s\n\n", val->nome);
	else if (tmp1 && !tmp2)
		printf("Sai por culpa do deadline: %s\n\n", val->nome);

	sem_post(&semCore);
}

// TODO:
void *Escalonador(void *a) {
	int deadProc = 0, front = 0;

	if (numEscalonamento == 1 || numEscalonamento == 2) {
		while (deadProc < nProcs) {
			sem_wait(&semQueue);
			
			sem_wait(&semCore);
			sem_post(&semThread[getNextQueue(head)->proc.id]);
			removeNextQueue(head);
			deadProc++;
		} 
	}

	else if (numEscalonamento == 4) {
		Node *p = head;

		while (deadProc < nProcs) {
			sem_wait(&semQueue);
	
			sem_wait(&semCore);
			sem_post(&semThread[getNextQueue(p)->proc.id]);
			decreaseQuantumNextQueue(p);
			p = getNextQueue(p);
		}	
	}
	
	return NULL;
}

/************************** FUNÇÕES AUXILIARES *******************************/

void decreaseQuantumNextQueue(Node* p){
	getNextQueue(p)->proc.dt -= quantum;

	if(getNextQueue(p)->proc.dt <= 0){
		removeNextQueue(p);
	}
}

void leArquivoEntrada(PROCESS listaProcessos[]){
	int i = 0;

	while (fscanf(arqEntrada,"%f %s %f %f %d", &listaProcessos[i].t0, listaProcessos[i].nome, 
		&listaProcessos[i].dt, &listaProcessos[i].deadline, &listaProcessos[i].p) != EOF) {		
		listaProcessos[i].id = i;
		i++;
	}
	nProcs = i;
}

void parserArgumentosEntrada(int argc, char* argv[]){
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
	for (i = 0; i < NMAX_PROCS; i++) 
		buffer[i] = NULL;

	if (sem_init(&semCore, 0, 1)) {
		fprintf(stderr, "ERRO ao criar semaforo semCore\n");
		exit(0);
	}
	if (sem_init(&semQueue, 0, 0)) {
		fprintf(stderr, "ERRO ao criar semaforo semQueue\n");
		exit(0);
	}
	for (i = 0; i < NMAX_PROCS; i++) {
		if (sem_init(&semThread[i], 0, 0)) {
			fprintf(stderr, "ERRO ao criar semaforo semThread[%ld]\n", i);
			exit(0);
		}
	}
}

/************************** FUNÇÕES DE FILA *****************************/

void initQueue() {
	head = (Node*) mallocSeguro(sizeof(Node));
	head->next = head;
	tail = head;
}

void insertOrderedByArrivedQueue(PROCESS proc) {
	Node* novo = (Node*) mallocSeguro(sizeof(Node));
	novo->proc = proc;
	novo->next = head;
	tail->next = novo;
	tail = novo;
}
 
void insertOrderedByJobQueue(PROCESS proc) {
	Node* aux;
	Node* novo = (Node*) mallocSeguro(sizeof(Node));
	novo->proc = proc;

	for (aux = head; aux != tail; aux = aux->next) {
		if (proc.dt < aux->next->proc.dt) {
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
}

void printQueue() {
	Node* aux;

	printf("Fila:  ");
	for (aux = head->next; aux != head; aux = aux->next) {
		printf("%s   ", aux->proc.nome);
	}
	printf("\n");
}

void removeNextQueue(Node *p) {
	if(!emptyQueue()) {
		Node* aux = p->next;
		p->next = aux->next;
		aux->next = NULL;
		free(aux);
		if (emptyQueue()) {
			tail = p;
		}
	}
}

int emptyQueue() {
	return (head->next == head);
}

// A fila não pode estar vazia para chamar a função
Node* getNextQueue(Node* p) {
	if(p->next == head) {
		p = head;
	}

	return p->next;
}

/************************ FUNÇÕES DE PROPÓSITO GERAL *********************/

int compare_arrive(const void *a,const void *b) {
	PROCESS *x = (PROCESS *) a;
	PROCESS *y = (PROCESS *) b;

	if (x->t0 < y->t0) return -1;
	else if (x->t0 > y->t0) return 1; 

	return 0;
}

float tempoDesdeInicio(struct timeval inicio){
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

/************* FUNCOES APENAS PARA TESTE *************/

void imprime(PROCESS listaProcessos[]) {
	int i;

	for (i = 0; i < nProcs; i++) {
		// fprintf(arqSaida ,"Nome: %s\n", listaProcessos[i].nome);
		// fprintf(arqSaida ,"t0: %f\n", listaProcessos[i].t0);
		// fprintf(arqSaida ,"dt: %f\n", listaProcessos[i].dt);
		// fprintf(arqSaida ,"deadline: %f\n", listaProcessos[i].deadline);
		// fprintf(arqSaida ,"p: %d\n\n", listaProcessos[i].p);
		printf("id: %d\n", listaProcessos[i].id);
	}
}
