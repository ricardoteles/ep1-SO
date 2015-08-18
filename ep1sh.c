#include <readline/readline.h>
#include <string.h>
#include <unistd.h>

char *diretorio;

void shell(char* line);

int main() {
	char shellName[80] = "";
	diretorio = getcwd (NULL,0);
	
	strcat(shellName, "[");
	strcat(shellName, diretorio);
	strcat(shellName, "] ");

	printf("\n---------- SHELL EP1 ----------\n\n");

	char* line = readline(shellName);

	while(strcmp(line,"exit") != 0){
		add_history (line);

		shell(line);

		line = readline(shellName);
	}
	return 0;
}

void shell(char* line){
	if(strcmp(line,"cd") == 0){
		printf("CD\n");
	}
	else if(strcmp(line,"pwd") == 0){
		printf("%s\n", diretorio);	
	}
	else if(strcmp(line,"/bin/ls -1") == 0){
		printf("Eh isso daqui: /bin/ls -1\n");
	}
	else if(strcmp(line,"./ep1") == 0){
		printf("Eh isso daqui: ./ep1\n");
	}
	else{
		printf("comando invalido\n");
	}
}