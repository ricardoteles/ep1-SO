#include <readline/readline.h>
#include <string.h>
#include <unistd.h>

char *path;
char format[80] = "";

void shell(char* line);
void criaPrefixoShell();

int main() {	
	criaPrefixoShell();
	
	printf("\n---------- SHELL EP1 ----------\n\n");

	char* line = readline(format);

	while(strcmp(line,"exit") != 0){
		add_history (line);

		shell(line);

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

void shell(char* line){
	//quebra o line em comando e argumento

	if(strcmp(line,"cd") == 0){
		chdir(line);
		criaPrefixoShell();
	}
	else if(strcmp(line,"pwd") == 0){
		printf("%s\n", path);	
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