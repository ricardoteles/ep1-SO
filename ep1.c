#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
// #include <semaphore.h>

#define NUM_PROCESSOS 8

typedef struct{
	float t0, dt, deadline;
	char nome[50];
	int p;
} PROC;

struct timeval inicio, fim;

/************* VARIAVEIS GLOBAIS ***********************/
int numEscalonamento = 0;
FILE* arqEntrada = NULL;  
FILE* arqSaida = NULL;
char d = '0';

/************* ASSINATURA DAS FUNCOES ******************/
void parserEntrada(int argc, char* argv[]);
void inicializaProcessos(PROC entradaProcessos[]);
void *processo(void *a);
int compare_arrive(const void *a,const void *b);
float tempoDesdeInicio();
void imprime(PROC entradaProcessos[]);

/*******************************************************/

int main(int argc, char* argv[]) {
	gettimeofday(&inicio, NULL);
	
	PROC entradaProcessos[NUM_PROCESSOS];				// TODO: criar um vetor com tamanho variavel
	pthread_t procs[NUM_PROCESSOS];
	long i;

	parserEntrada(argc, argv);
	inicializaProcessos(entradaProcessos);
	
	if(numEscalonamento == 1){
		qsort(entradaProcessos,NUM_PROCESSOS,sizeof(PROC),compare_arrive);
	}

	for(i = 0; i < NUM_PROCESSOS; i++){
		while(tempoDesdeInicio() < entradaProcessos[i].t0);
		
		if(pthread_create(&procs[i], NULL, processo, (void *) &entradaProcessos[i])){
            printf("\n ERROR creating thread %ld\n", i);
            exit(1);
        }
	}

	for(i = 0; i < NUM_PROCESSOS; i++){
        if(pthread_join(procs[i], NULL)){
            printf("\n ERROR joining thread\n");
            exit(1);
        }
    }

	fclose(arqEntrada);

	// printf("%f seg\n\n", tempoDesdeInicio());
	return 0;
}

void *processo(void *a){
	PROC *val = (PROC *) a;

	printf("Ola eu sou o processo:  %s\n", val->nome);
	
	return NULL;
}

void inicializaProcessos(PROC entradaProcessos[]){
	int i = 0;

	while (fscanf(arqEntrada,"%f %s %f %f %d", &entradaProcessos[i].t0, entradaProcessos[i].nome, 
		&entradaProcessos[i].dt, &entradaProcessos[i].deadline, &entradaProcessos[i].p) != EOF) {		
		i++;
	}
}

void parserEntrada(int argc, char* argv[]){
	if (argc == 4 || argc == 5) {
		numEscalonamento = atoi(argv[1]);
		arqEntrada = fopen(argv[2], "r");  
		arqSaida = fopen(argv[3], "w");;

		if(argc == 5 && argv[4][0] == 'd') {
			d = 'd';
			printf("%c\n", argv[4][0]);
		}
	}
	else {
		printf("ERRO no numero de argumentos\n");
		exit(-2);
	}
}

int compare_arrive(const void *a,const void *b) {
	PROC *x = (PROC *) a;
	PROC *y = (PROC *) b;

	if (x->t0 < y->t0) return -1;
	else if (x->t0 > y->t0) return 1; 

	return 0;
}

/************* FUNCOES APENAS PARA TESTE *************/

float tempoDesdeInicio(){
	float timedif;

	gettimeofday(&fim, NULL);
	timedif = (float)(fim.tv_sec - inicio.tv_sec);
	timedif += (float)(fim.tv_usec - inicio.tv_usec)/1000000;

	return timedif;
}

void imprime(PROC entradaProcessos[]){
	int i;

	for (i = 0; i < 8; i++) {
		fprintf(arqSaida ,"Nome: %s\n", entradaProcessos[i].nome);
		// fprintf(arqSaida ,"t0: %f\n", entradaProcessos[i].t0);
		// fprintf(arqSaida ,"dt: %f\n", entradaProcessos[i].dt);
		// fprintf(arqSaida ,"deadline: %f\n", entradaProcessos[i].deadline);
		// fprintf(arqSaida ,"p: %d\n\n", entradaProcessos[i].p);
	}
}