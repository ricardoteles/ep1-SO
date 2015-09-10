#!/bin/bash

numTestes=2				# nao pode ter espaco entre a variavel e o igual
numEscalonadores=4      # nao pode ter espaco entre a variavel e o igual

rm dados.txt
make

for j in $(seq $numEscalonadores)
do
	printf "=====================\n" >> dados.txt
	printf "   Escalonador: $j   \n" >> dados.txt
	printf "=====================\n\n" >> dados.txt
	
	for i in $(seq $numTestes);
	do
		printf "Teste $i\n\n" >> dados.txt

		./ep1 $j traceEntrada.txt saida 
		cat saida >> dados.txt;
		printf "\n\n" >> dados.txt
	done
done