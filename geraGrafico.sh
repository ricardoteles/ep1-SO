#!/bin/bash

numTestes=3					# nao pode ter espaco entre a variavel e o igual
escalonador=3				    # nao pode ter espaco entre a variavel e o igual
arquivos="Pouco"

rm dados.txt
make

printf "(Escalonador:$escalonador / mudancaContexto)-> [" >>  dadosMud.txt
printf "(Escalonador:$escalonador / deadline)-> [" >>  dadosDead.txt
printf "(Escalonador:$escalonador / justica)-> [" >>  dadosJust.txt
		
for teste in $(seq $numTestes);
do
	./ep1 $escalonador traceEntrada$arquivos.txt saida 
	cat mudancaContexto.txt >>  dadosMud.txt;
	cat deadline.txt >>  dadosDead.txt;
	cat justica.txt >>  dadosJust.txt;

	printf "," >>  dadosMud.txt
	printf "," >>  dadosDead.txt
	printf "," >>  dadosJust.txt
done
printf "]\n\n" >>  dadosMud.txt
printf "]\n\n" >>  dadosDead.txt
printf "]\n\n" >>  dadosJust.txt

cat dadosDead.txt >> dadosMud.txt
cat dadosJust.txt >> dadosMud.txt

mv dadosMud.txt dados.txt

rm dadosDead.txt
rm dadosJust.txt