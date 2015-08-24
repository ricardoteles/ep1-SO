#include <readline/readline.h>

int main(int argc, char *argv[])
{
	if(argc == 4 || argc == 5) printf("Eu: %s %s %s %s. Acabou\n", argv[0], argv[1], argv[2], argv[3]);

	return 0;
}