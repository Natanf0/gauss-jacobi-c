#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h> // Para intptr_t

#define MAX_THREADS 20
#define LIMIAR 0.00001

int nthreads;
float *matA;
float *vetB;
float *vetX;
float *copyVetX;
double erroGlobal = 0;

int qtdLinhas, qtdColunas;
int linhasVet, colunasVet;

pthread_mutex_t mutex;
pthread_mutex_t mutexErr;
pthread_cond_t cond;

// Barreira de sincronização
void barreira() {
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

// Thread principal de Gauss-Jacobi
void* gaussjacobi(void* arg) {
    int id = (intptr_t) arg;

    int bloco = qtdLinhas / nthreads;
    int inicio = id * bloco;
    int fim = (id == nthreads - 1) ? qtdLinhas : inicio + bloco;

    double x_old, erroLocal;

    printf("Thread %d começando\n", id);

    do {
        barreira();
        if (id == 0) erroGlobal = 0;
        barreira();

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
        pthread_mutex_unlock(&mutexErr);

        barreira();

        if (id == 0) {
            for (int i = 0; i < linhasVet; i++) {
                copyVetX[i] = vetX[i];
            }
            printf("\nErro global: %lf\n", erroGlobal);
        }

        barreira();

        if (erroGlobal < LIMIAR) {
            if (id == 0) printf("Convergiu!\n");
            pthread_exit(NULL);
        }
        barreira();
        if (id==1){
            erroGlobal = 0;
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
        fprintf(stderr, "Digite: %s <#THREADS> (Máx.: %d)\n", argv[0], MAX_THREADS);
        return 1;
    }

    int n = atoi(argv[1]);
    nthreads = (n > MAX_THREADS) ? MAX_THREADS : n;
    pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * nthreads);

    FILE *descritorArquivoMatriz, *descritorArquivoVetorB, *descritorArquivoVetorX;

    descritorArquivoMatriz = fopen("matA.bin", "rb");
    if (!descritorArquivoMatriz) {
        fprintf(stderr, "Erro de abertura do arquivo com a matriz\n");
        return 2;
    }
    if (carregaEntrada(descritorArquivoMatriz, &matA, &qtdLinhas, &qtdColunas)) return 3;

    descritorArquivoVetorB = fopen("vetB.bin", "rb");
    if (!descritorArquivoVetorB) {
        fprintf(stderr, "Erro de abertura do arquivo com o vetor B\n");
        return 4;
    }
    if (carregaEntrada(descritorArquivoVetorB, &vetB, &linhasVet, &colunasVet)) return 5;

    descritorArquivoVetorX = fopen("vetX.bin", "rb");
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

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexErr, NULL);
    pthread_cond_init(&cond, NULL);

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tid[i], NULL, gaussjacobi, (void*)(intptr_t)i)) {
            printf("Erro ao criar a thread %d\n", i);
            exit(-1);
        }
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Erro ao aguardar a thread %d\n", i);
            exit(-1);
        }
    }

    printf("\nResultado final:\n");
    for (int i = 0; i < qtdLinhas; i++) {
        printf("%f\n", vetX[i]);
    }

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
    // Libera recursos
    free(matA);
    free(vetB);
    free(vetX);
    free(copyVetX);
    free(tid);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexErr);
    pthread_cond_destroy(&cond);

    return 0;
}
