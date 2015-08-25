#include <stdio.h>
#include <stdlib.h>

typedef struct{
	float t0, dt, deadline;
	char nome[50];
	int p;
} PROC;


/************* ASSINATURA DAS FUNCOES ******************/
void imprime(PROC proc[]);
void parserEntrada(int argc, char* argv[]);

/************* VARIAVEIS GLOBAIS ***********************/
int numEscal = 0;
FILE* arqEntrada = NULL;  
FILE* arqSaida = NULL;
char d = '0';
/*******************************************************/

int main(int argc, char* argv[]) {
	PROC proc[8];
	int i = 0;

	parserEntrada(argc, argv);

	while (fscanf(arqEntrada,"%f %s %f %f %d", &proc[i].t0, proc[i].nome, 
		&proc[i].dt, &proc[i].deadline, &proc[i].p) != EOF) {
		i++;
	}

	imprime(proc);
	fclose(arqEntrada);

	return 0;
}

void parserEntrada(int argc, char* argv[]){
	if (argc == 4 || argc == 5) {
		numEscal = atoi(argv[1]);
		arqEntrada = fopen(argv[2], "r");  
		arqSaida = fopen(argv[3], "w");;

		printf("%d\n", numEscal);
		printf("%s\n", argv[2]);
		printf("%s\n", argv[3]);

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

void imprime(PROC proc[]){
	int i;

	for (i = 0; i < 8; i++) {
		printf("Nome: %s\n", proc[i].nome);
		printf("t0: %f\n", proc[i].t0);
		printf("dt: %f\n", proc[i].dt);
		printf("deadline: %f\n", proc[i].deadline);
		printf("p: %d\n\n", proc[i].p);
	}
}