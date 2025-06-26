#!/bin/bash
gcc -o gerarDados gerarDados.c -O5
gcc -o gaussjacobi gauss-jacobi.c -lm -lpthread
gcc -o corretude corretude.c
touch newVetX.bin # Criando o arquivo binário que irá armazenar a saída do programa gauss-jacobi
clear

# Arquivos de entrada
arquivoMatA="matA.bin"  
arquivoVetB="vetB.bin"   
arquivoVetX="vetX.bin"
arquivoNewVetX="newVetX.bin"


threads=(1 2 3 5 10 20)
dimensao=(20 100 1000 10000 20000 40000)
#dimensao=(20 100 1000 10000 20000 40000 45000)

echo "N, T, Dc, Dp, Dg, Dt, E:"
for d in ${dimensao[@]}; do
    for t in ${threads[@]}; do
        ./gerarDados $d $d $arquivoMatA
        ./gerarDados $d 1 $arquivoVetB
        ./gerarDados $d 1 $arquivoVetX
        ./gaussjacobi $arquivoMatA $arquivoVetB $arquivoVetX $t
        ./corretude $arquivoMatA $arquivoNewVetX $arquivoVetB $t
   done
   echo ""
done


