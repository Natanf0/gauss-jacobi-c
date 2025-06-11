/* Programa que cria arquivo com uma matriz/vetor de valores do tipo float, gerados aleatoriamente
 * Entrada: dimensoes da matriz (linhas e colunas) e nome do arquivo de saida
 * Saida: arquivo binario com as dimensoes (valores inteiros) da matriz (linhas e colunas),
 * seguido dos valores (float) de todas as celulas da matriz gerados aleatoriamente
 * */

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<math.h>

float *matriz; //matriz ( ou vetor ) que será gerada (o)

void gerarMatrizA(int linhas, int colunas){
   for (int i = 0; i < linhas; i++) {
      float soma = 0.0f;
      for (int j = 0; j < colunas; j++) {
         long int idx = i * colunas + j;
         if (i != j) {
            float r;
            if (((float)rand() / RAND_MAX) < 0.5f) {
               r = 0.0f; // garante que a matriz será esparsa
            } else {
               r = ((float)rand() / RAND_MAX) * 2.1f;
            }
            matriz[idx] = r;
            soma += fabs(r);
         }
      }
      // garante dominância estrita na diagonal
      matriz[i * colunas + i] = soma + ((float)rand() / RAND_MAX) + 10.1f;
   }
}

void gerarVetorB(int linhas){
   // vetor B com valores floats entre 0 e 102.1
   for (int i = 0; i < linhas; i++) {
       matriz[i] = ((float)rand() / RAND_MAX) * 102.1f; 
   }  
}

void gerarVetorX(int linhas){
   // o vetor de chute inicial será o vetor nulo.
   for (int i = 0; i < linhas; i++) { matriz[i] = 0.f; }
}

int main(int argc, char*argv[]) {
   
   int linhas, colunas; //dimensoes da matriz
   long long int tam; //qtde de elementos na matriz
   FILE * descritorArquivo; //descritor do arquivo de saida
   size_t ret; //retorno da funcao de escrita no arquivo de saida
   
   //recebe os argumentos de entrada
   if(argc < 4) {
      fprintf(stderr, "Digite: %s <#linhas> <#colunas> <arquivo saida>\n", argv[0]);
      return 1;
   }
   linhas = atoi(argv[1]);
   colunas = atoi(argv[2]);
   tam = linhas * colunas;
 
   //aloca memoria para a matriz
   matriz = (float*) malloc(sizeof(float) * tam);
   if(!matriz) {
      fprintf(stderr, "Erro de alocação de memória da matriz\n");
      return 2;
   }
 
   srand((unsigned int)time(NULL) ^ (unsigned int)clock());
   if(strcmp(argv[3], "matA.bin") == 0){
      gerarMatrizA(linhas, colunas);
   }else if(strcmp(argv[3], "vetB.bin") == 0){
      gerarVetorB(linhas);
   }else{
      gerarVetorX(linhas);
   }
  
   //escreve a matriz no arquivo
   //abre o arquivo para escrita binaria
   descritorArquivo = fopen(argv[3], "wb");
   if(!descritorArquivo) {
      fprintf(stderr, "Erro de abertura do arquivo\n");
      return 3;
   }
   //escreve numero de linhas e de colunas
   ret = fwrite(&linhas, sizeof(int), 1, descritorArquivo);
   ret = fwrite(&colunas, sizeof(int), 1, descritorArquivo);
   //escreve os elementos da matriz
   ret = fwrite(matriz, sizeof(float), tam, descritorArquivo);
   if(ret < tam) {
      fprintf(stderr, "Erro de escrita no arquivo\n");
      return 4;
   }
 
   //finaliza o uso das variaveis
   fclose(descritorArquivo);
   free(matriz);
   return 0;
}