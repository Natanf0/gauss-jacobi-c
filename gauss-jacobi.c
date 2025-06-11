#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h> 
#include "timer.h"

#define MAX_THREADS 20
#define LIMIAR 0.0001

int nthreads;
float *matA;
float *vetB;
float *vetX;
float *copyVetX;
double erroGlobal = 0;

int qtdLinhas, qtdColunas;
int linhasVet, colunasVet;
int convergiu;

pthread_mutex_t mutex;
pthread_mutex_t mutexErr;
pthread_cond_t cond;

void barreira() {
    // Função que realiza a sincronização do tipo barreira entre as threads.
    static int bloqueadas = 0;
    pthread_mutex_lock(&mutex);
    bloqueadas++;
    if (bloqueadas == nthreads) {
        bloqueadas = 0;
        pthread_cond_broadcast(&cond);
    } else {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}

void* copyX(void* arg){
    // função que realiza a cópia do vetor X para o vetor copyVetX
    // é necessária pois cada iteração de cada thread utiliza todos os valores do vetor X, então a escrita teria que esperar poder escrever antes de calcular o próximo. Deadlock
    // a função foi implementada de forma concorrente também.

    int id = (intptr_t) arg;
    int bloco = qtdLinhas / nthreads;
    int inicio = id * bloco;
    int fim = (id == nthreads - 1) ? qtdLinhas : inicio + bloco;
    
    for (int i = inicio; i < fim; i++) {
        copyVetX[i] = vetX[i];
    }
}

void* gaussjacobi(void* arg) {
    int id = (intptr_t) arg;

    int bloco = qtdLinhas / nthreads;
    int inicio = id * bloco;
    int fim = (id == nthreads - 1) ? qtdLinhas : inicio + bloco;

    double x_old, erroLocal;
    do {
        erroLocal = 0;
        for (int i = inicio; i < fim; i++) {
            x_old = vetX[i];
            double p = 0;
            for (int j = 0; j < qtdColunas; j++) {
                if (j != i) {
                    p += matA[i * qtdColunas + j] * copyVetX[j];
                }
            }
            vetX[i] = (vetB[i] - p) / matA[i * qtdColunas + i];
            erroLocal += fabs(vetX[i] - x_old);
        }

        pthread_mutex_lock(&mutexErr);
        erroGlobal += erroLocal;
        convergiu = (erroGlobal <= LIMIAR); 
        pthread_mutex_unlock(&mutexErr);
        
        barreira(); // o boleano 'convergiu' será determinado pela última thread que incrementar erroTotal.

        if (convergiu) break;

        copyX(arg);
    
        if (id == 0) {
            //printf("\nErro global: %.10lf\n", erroGlobal);
            erroGlobal=0;
        }
        barreira();
    } while (1);
}

// Carrega matriz ou vetor de arquivo binário
int carregaEntrada(FILE *arquivo, float **entrada, int *linhas, int *colunas) {
    size_t ret;

    ret = fread(linhas, sizeof(int), 1, arquivo);
    if (!ret) return 1;


    ret = fread(colunas, sizeof(int), 1, arquivo);
    if (!ret) return 2;


    int tam = (*linhas) * (*colunas);
    *entrada = (float *) malloc(sizeof(float) * tam);
    if (!*entrada) return 3;


    ret = fread(*entrada, sizeof(float), tam, arquivo);
    if (ret < tam) return 4;


    return 0;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Digite: %s <matriz A> <vetor B> <vetor X> <# threads> (Máx.: %d)\n", argv[0], MAX_THREADS);
        return 1;
    }
    double inicio, fim, duracao, total=0;

    int n = atoi(argv[4]);
    nthreads = (n > MAX_THREADS) ? MAX_THREADS : n;
    pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * nthreads);

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexErr, NULL);
    pthread_cond_init(&cond, NULL);
    

    FILE *descritorArquivoMatriz, *descritorArquivoVetorB, *descritorArquivoVetorX;

    // Carregando as entradas:
    GET_TIME(inicio);

    descritorArquivoMatriz = fopen(argv[1], "rb");
    if (!descritorArquivoMatriz) {
        fprintf(stderr, "Erro de abertura do arquivo com a matriz\n");
        return 2;
    }
    if (carregaEntrada(descritorArquivoMatriz, &matA, &qtdLinhas, &qtdColunas)) return 3;


    descritorArquivoVetorB = fopen(argv[2], "rb");
    if (!descritorArquivoVetorB) {
        fprintf(stderr, "Erro de abertura do arquivo com o vetor B\n");
        return 4;
    }
    if (carregaEntrada(descritorArquivoVetorB, &vetB, &linhasVet, &colunasVet)) return 5;


    descritorArquivoVetorX = fopen(argv[3], "rb");
    if (!descritorArquivoVetorX) {
        fprintf(stderr, "Erro de abertura do arquivo com o vetor X0\n");
        return 6;
    }
    if (carregaEntrada(descritorArquivoVetorX, &vetX, &linhasVet, &colunasVet)) return 7;

    copyVetX = (float*) malloc(sizeof(float) * linhasVet);
    if (!copyVetX) {
        fprintf(stderr, "Erro ao alocar memória para copyVetX\n");
        return 8;
    }
    for (int i = 0; i < linhasVet; i++) {
        copyVetX[i] = vetX[i]; 
    }

    GET_TIME(fim);
    duracao = fim - inicio;
    total += duracao;

    printf("%d, %d, ", qtdLinhas, nthreads);
    // tempo de carregamento das entradas
    printf(" %lf, ",duracao );


    GET_TIME(inicio);
    // Criando e disparando as threads:
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tid[i], NULL, gaussjacobi, (void*)(intptr_t)i)) {
            printf("Erro ao criar a thread %d\n", i);
            exit(-1);
        }
    }
    

    // Aguardando as threads
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Erro ao aguardar a thread %d\n", i);
            exit(-1);
        }
    }

    GET_TIME(fim);
    duracao = fim - inicio;
    total += duracao;

    // tempo de processamento
    printf(" %lf, ",duracao );


    GET_TIME(inicio);
    FILE * descritorArquivo; //descritor do arquivo de saida
    size_t ret; //retorno da funcao de escrita no arquivo de saida
   
     //abre o arquivo para escrita binaria
     descritorArquivo = fopen("newVetX.bin", "wb");
     if(!descritorArquivo) {
        fprintf(stderr, "Erro de abertura do arquivo\n");
        return 3;
     }
     //escreve numero de linhas e de colunas
     ret = fwrite(&linhasVet, sizeof(int), 1, descritorArquivo);
     ret = fwrite(&colunasVet, sizeof(int), 1, descritorArquivo);
     //escreve os elementos da matriz
     ret = fwrite(vetX, sizeof(float), linhasVet, descritorArquivo);
     if(ret < linhasVet) {
        fprintf(stderr, "Erro de escrita no  arquivo\n");
        return 4;
     }

    // Liberando os recursos
    free(matA);
    free(vetB);
    free(vetX);
    free(copyVetX);
    free(tid);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexErr);
    pthread_cond_destroy(&cond);

    GET_TIME(fim);
    duracao = fim - inicio;
    total += duracao;

    // tempo de gravação do resultado no arquivo newVetX.bin e desalocação das estruturas
    printf(" %lf, ",duracao );
    printf(" %lf, ",total );
    return 0;
}



