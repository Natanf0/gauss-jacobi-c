/*
Programa que avalia a corretude do gauss-jacobi
Ideia: Realizar a multiplicação A*x, onde x é o vetor retornado do gauss jacobi
E então subtrair B
A*x - b
Por fim, calcular a norma deste vetor
erro = || A*x - b || (Calculado pela funrção concorrente calculaErro())

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

float *matA; // matriz de entrada
float *vetB; // vetor de entrada
float *vetX; // vetor de entrada

double erroTotal; // erro global
pthread_mutex_t mutex;

typedef struct {
   int id;       // id da thread
   int dim;      // dimensão matriz/vetor
   int nthreads; // número de threads
} tArgs;

// função executada pelas threads para calcular o erro total (||Ax - b||)
void *calculaErro(void *arg) {
   tArgs *args = (tArgs*) arg;
   for (int i = args->id; i < args->dim; i += args->nthreads) {
      double s = 0.0;  // <<< zera a soma a cada linha
      for (int j = 0; j < args->dim; j++) {
         s += matA[i * (args->dim) + j] * vetX[j];
      }
      pthread_mutex_lock(&mutex);
      erroTotal += fabs(s - vetB[i]);
      pthread_mutex_unlock(&mutex);
   }
   pthread_exit(NULL);
}

// carrega matriz ou vetor do arquivo binário
int carregaEntrada(FILE *descritorArquivo, float **entrada, int *linhas, int *colunas) {
   size_t ret;
   int tam;

   // lê dimensões
   ret = fread(linhas, sizeof(int), 1, descritorArquivo);
   if (!ret) {
      fprintf(stderr, "Erro de leitura das dimensoes no arquivo \n");
      return 1;
   }
   ret = fread(colunas, sizeof(int), 1, descritorArquivo);
   if (!ret) {
      fprintf(stderr, "Erro de leitura das dimensoes no arquivo \n");
      return 2;
   }
   tam = (*linhas) * (*colunas);

   *entrada = (float*) malloc(sizeof(float) * tam);
   if (!*entrada) {
      fprintf(stderr, "Erro de alocacao da memoria\n");
      return 3;
   }

   ret = fread(*entrada, sizeof(float), tam, descritorArquivo);
   if (ret < tam) {
      fprintf(stderr, "Erro de leitura dos elementos da estrutura de entrada\n");
      return 4;
   }
   return 0;
}

int main(int argc, char* argv[]) {
   int nthreads;
   pthread_t *tid;
   tArgs *args;
   FILE *descritorArquivoMatriz, *descritorArquivoVetorX, *descritorArquivoVetorB;
   int linhasMat, colunasMat;
   int linhasVetX, colunasVetX;
   int linhasVetB, colunasVetB;

   pthread_mutex_init(&mutex, NULL);

   if (argc < 5) {
      printf("Uso: %s <matriz A> <vetor new X> <vetor B> <num threads>\n", argv[0]);
      return 1;
   }

   // lê matriz A
   descritorArquivoMatriz = fopen(argv[1], "rb");
   if (!descritorArquivoMatriz) {
      fprintf(stderr, "Erro ao abrir arquivo da matriz A\n");
      return 2;
   }
   if (carregaEntrada(descritorArquivoMatriz, &matA, &linhasMat, &colunasMat)) {
      fclose(descritorArquivoMatriz);
      return 3;
   }
   fclose(descritorArquivoMatriz);

   // lê vetor X (resultado do Gauss-Jacobi)
   descritorArquivoVetorX = fopen(argv[2], "rb");
   if (!descritorArquivoVetorX) {
      fprintf(stderr, "Erro ao abrir arquivo do vetor X\n");
      free(matA);
      return 2;
   }
   if (carregaEntrada(descritorArquivoVetorX, &vetX, &linhasVetX, &colunasVetX)) {
      fclose(descritorArquivoVetorX);
      free(matA);
      return 3;
   }
   fclose(descritorArquivoVetorX);

   // lê vetor B
   descritorArquivoVetorB = fopen(argv[3], "rb");
   if (!descritorArquivoVetorB) {
      fprintf(stderr, "Erro ao abrir arquivo do vetor B\n");
      free(matA);
      free(vetX);
      return 2;
   }
   if (carregaEntrada(descritorArquivoVetorB, &vetB, &linhasVetB, &colunasVetB)) {
      fclose(descritorArquivoVetorB);
      free(matA);
      free(vetX);
      return 3;
   }
   fclose(descritorArquivoVetorB);

   // verifica compatibilidade das dimensões
   if (colunasMat != linhasVetX || linhasMat != linhasVetB || colunasVetX != 1 || colunasVetB != 1) {
      fprintf(stderr, "Dimensoes incompatíveis entre A, X e B\n");
      free(matA);
      free(vetX);
      free(vetB);
      return 4;
   }

   nthreads = atoi(argv[4]);
   if (nthreads > linhasMat) nthreads = linhasMat;

   tid = (pthread_t*) malloc(sizeof(pthread_t) * nthreads);
   args = (tArgs*) malloc(sizeof(tArgs) * nthreads);
   if (!tid || !args) {
      fprintf(stderr, "Erro de malloc\n");
      free(matA);
      free(vetX);
      free(vetB);
      return 5;
   }

   erroTotal = 0.0;

   for (int i = 0; i < nthreads; i++) {
      args[i].id = i;
      args[i].dim = linhasMat;
      args[i].nthreads = nthreads;
      if (pthread_create(&tid[i], NULL, calculaErro, (void*) &args[i])) {
         fprintf(stderr, "Erro pthread_create\n");
         free(matA);
         free(vetX);
         free(vetB);
         free(tid);
         free(args);
         return 6;
      }
   }

   for (int i = 0; i < nthreads; i++) {
      pthread_join(tid[i], NULL);
   }

   printf("%lf\n", erroTotal);

   pthread_mutex_destroy(&mutex);
   free(matA);
   free(vetX);
   free(vetB);
   free(tid);
   free(args);
   return 0;
}
