#include <readline/readline.h>
#include <string.h>
#include <unistd.h>

#define LINMAX 10
#define COLMAX 50

char *path;
char format[80] = "";
char command[LINMAX][COLMAX];

void criaPrefixoShell();
void shell();
void apagaMatriz();
void parserCommand(char *line);
void imprimeCommand();

/****************************************/

int main() {	
	criaPrefixoShell();
	
	printf("\n---------- SHELL EP1 ----------\n\n");

	char* line = readline(format);

	while(strcmp(line,"exit") != 0){
		add_history (line);
		apagaMatriz();
		parserCommand(line);
		shell();
		imprimeCommand();

		line = readline(format);
	}
	return 0;
}

void criaPrefixoShell(){
	path = getcwd (NULL,0);
	format[0] = '\0';
	strcat(format, "[");				
	strcat(format, path);
	strcat(format, "] ");	
}

void shell(){
	if(strcmp(command[0],"cd") == 0){
		chdir(command[1]);
		criaPrefixoShell();
	}
	else if(strcmp(command[0],"pwd") == 0){
		printf("%s\n", path);	
	}
	else if(strcmp(command[0],"/bin/ls") == 0){
		char *argv[] = {command[0], command[1], NULL};
		if(fork() == 0){
			execve(command[0], argv, NULL);
		}
		else{
			waitpid(-1, 0, 0);
		}
	}
	else if(strcmp(command[0],"./ep1") == 0){
		if(fork() == 0){
			if(command[4][0] != '\0'){
				char *argv[] = {command[0], command[1], command[2], command[3], command[4], NULL};
				execve(command[0], argv, NULL);
			}
			else{
				char *argv[] = {command[0], command[1], command[2], command[3], NULL};
				execve(command[0], argv, NULL);
			}
		}
		else{
			waitpid(-1, 0, 0);
		}
	}
	else{
		printf("comando invalido\n");
	}
}

void apagaMatriz(){
	int i, j;

	for (i = 0; i < LINMAX; i++) {
		for (j = 0; j < COLMAX; j++) {
			command[i][j] = 0;
		}
	}
}

void parserCommand(char *line){
	int i, lin = 0, col = 0;

	for(i = 0; line[i] != '\0'; i++){
		if(line[i] != ' '){
			command[lin][col++] = line[i];
		}
		else if(col != 0){
			lin++;
			col = 0;
		}
	}
}

/************** FUNCOES TESTES **************************/

void imprimeCommand(){
	int i, j;

	for(i = 0; command[i][0] != 0; i++){
		printf("%s ", command[i]);
	}
	printf("\n");
}