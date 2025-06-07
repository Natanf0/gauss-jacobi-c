#!/bin/bash
gcc -o gerarDados gerarDados.c -O5
gcc -o gaussjacobi ma.c -lm -lpthread
gcc -o matvet matvet.c
clear

echo "Gerando a matriz A"
./gerarDados 5 5 matA.bin
echo ""

echo "Gerando o vetor B"
./gerarDados 5 1 vetB.bin
echo ""

echo "Gerando o vetor X0 (Chute inicial)"
./gerarDados 5 1 vetX.bin
echo ""
echo "Executando gauss jacobi
"
echo ""
./gaussjacobi 5
echo ""
echo "Executando matvet"
echo ""
./matvet matA.bin newVetX.bin 5