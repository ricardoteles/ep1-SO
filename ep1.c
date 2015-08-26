#include <stdio.h>
#include <stdlib.h>
#define NUM_PROCESSOS 8

typedef struct{
	float t0, dt, deadline;
	char nome[50];
	int p;
} PROC;


/************* ASSINATURA DAS FUNCOES ******************/
void imprime(PROC entradaProcessos[]);
void parserEntrada(int argc, char* argv[]);
void inicializaThreads(PROC entradaProcessos[]);
void *processo(void *a);

/************* VARIAVEIS GLOBAIS ***********************/
int numEscalonamento = 0;
FILE* arqEntrada = NULL;  
FILE* arqSaida = NULL;
char d = '0';
/*******************************************************/

int main(int argc, char* argv[]) {
	PROC entradaProcessos[NUM_PROCESSOS];				// TODO: criar um vetor com tamanho variavel
	pthread_t procs[NUM_PROCESSOS];
	long i;

	parserEntrada(argc, argv);
	inicializaThreads(entradaProcessos);
	
	for(i = 0; i < NUM_PROCESSOS; i++){
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

	imprime(entradaProcessos);
	fclose(arqEntrada);

	return 0;
}

void *processo(void *a){
	PROC *val = (PROC *) a;

	printf("%s\n", val->nome);
	printf("%f\n", val->t0);
	
	return NULL;
}

void inicializaThreads(PROC entradaProcessos[]){
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

/************* FUNCOES APENAS PARA TESTE *************/

void imprime(PROC entradaProcessos[]){
	int i;

	for (i = 0; i < 8; i++) {
		fprintf(arqSaida ,"Nome: %s\n", entradaProcessos[i].nome);
		fprintf(arqSaida ,"t0: %f\n", entradaProcessos[i].t0);
		fprintf(arqSaida ,"dt: %f\n", entradaProcessos[i].dt);
		fprintf(arqSaida ,"deadline: %f\n", entradaProcessos[i].deadline);
		fprintf(arqSaida ,"p: %d\n\n", entradaProcessos[i].p);
	}
}