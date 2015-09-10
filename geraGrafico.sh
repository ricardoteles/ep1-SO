#!/bin/bash

numTestes=2				# nao pode ter espaco entre a variavel e o igual
numEscalonadores=3      # nao pode ter espaco entre a variavel e o igual
arquivos=("Pouco" "Medio" "Muito")

rm dados.txt
make

for item in "${arquivos[@]}"
do
	printf "=====================\n" >>  dados.txt
	printf "    Arquivo: $item   \n" >>  dados.txt
	printf "=====================\n\n" >>  dados.txt

	for escalonador in $(seq $numEscalonadores)
	do
		printf "(Escalonador:$escalonador)-> [" >>  dados.txt
		
		for teste in $(seq $numTestes);
		do

			./ep1 $escalonador traceEntrada$item.txt saida 
			cat mudancaContexto.txt >>  dados.txt;
			printf "," >>  dados.txt
		done
		printf "]\n\n" >>  dados.txt
	done
done