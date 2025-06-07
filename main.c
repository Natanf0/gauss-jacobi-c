#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<pthread.h>
#include<math.h>

int MAX_THREADS = 20;
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

// sincronização entre cada iteração do algoritmo
void barreira() {
    static int bloqueadas = 0;
    pthread_mutex_lock(&mutex); // início da seção crítica

    bloqueadas++;
    if (bloqueadas == nthreads) {
        bloqueadas = 0;
        erroGlobal = 0;
        // copia os valores calculados de vetX para copyVetX
        for (int i = 0; i < linhasVet; i++) {
            copyVetX[i] = vetX[i];
        }
        pthread_cond_broadcast(&cond);
    } else {
        pthread_cond_wait(&cond, &mutex);
    }

    pthread_mutex_unlock(&mutex); // fim da seção crítica
}

void* gaussjacobi(void* arg){

    int id = (int) arg;
    
    int bloco = qtdLinhas/nthreads; //tamanho do bloco de dados de cada thread
    int inicio = id*bloco; //posicao inicial do vetor
    int fim = (id == nthreads - 1) ? qtdLinhas : inicio + bloco;

    double x_old, erroLocal = 0;
    printf("Thread %d começando\n", id);

    do{
        erroLocal = 0;
        for(int i = inicio; i<fim; i++){
            x_old = vetX[i];
            double p = 0; 
            for(int j = 0; j<qtdColunas; j++){
                if(j!=i){
                    p += matA[i * qtdColunas + j] * copyVetX[j];

                } // aproveitando o pipeline da CPU
            }
            vetX[i] = (vetB[i]-p)/matA[i*qtdColunas + i];
            erroLocal += fabs(vetX[i] - x_old); // erro acumulado entre as linhas do bloco da thread
            printf("X[%d] = %lf da thread %d com erro = %lf\n", i,vetX[i], id, erroLocal);          
        }
        pthread_mutex_lock(&mutexErr);
            erroGlobal += erroLocal;
        pthread_mutex_unlock(&mutexErr);
        printf("Thread %d entrando na barreira\n", id);
        barreira();
        printf("\nErro global: %lf\n", erroGlobal);
        if(erroGlobal < LIMIAR){printf("Thread %d saindo da barreira\n", id);pthread_exit(NULL);}
       
    }while(1) ;
}

//carrega a matriz/vetor de entrada, retorna a qtde de linhas e colunas e a matriz/vetor preenchido
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

int main(int argc, char* argv[]){
    if(argc < 2) {
        fprintf(stderr, "Digite: %s <#THREADS> (Máx.:20)\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    nthreads = ( n > 20) ? MAX_THREADS: n ;
    pthread_t *tid;
    tid = (pthread_t*) malloc(sizeof(pthread_t)*nthreads);
    FILE *descritorArquivoMatriz, *descritorArquivoVetorB, *descritorArquivoVetorX ; //descritores do arquivo de entrada
   
    //abre o arquivo com a matriz para leitura binaria
    descritorArquivoMatriz = fopen("matA.bin", "rb");
    if(!descritorArquivoMatriz) {
        fprintf(stderr, "Erro de abertura do arquivo com a matriz\n");
        return 2;
    }    
    //carrega a matriz
    if(carregaEntrada(descritorArquivoMatriz, &matA, &qtdLinhas, &qtdColunas)) //'mat' esta no escopo global
        return 3;

    //abre o arquivo com o vetor para leitura binaria
    descritorArquivoVetorB = fopen("vetB.bin", "rb");
    if(!descritorArquivoVetorB) {
        fprintf(stderr, "Erro de abertura do arquivo com o vetor\n");
        return 4;
    }
    //carrega o vetor
    if(carregaEntrada(descritorArquivoVetorB, &vetB, &linhasVet, &colunasVet)) //'vet' esta no escopo global
        return 5;

    //abre o arquivo com o vetor para leitura binaria
    descritorArquivoVetorX = fopen("vetX.bin", "rb");
    if(!descritorArquivoVetorX) {
        fprintf(stderr, "Erro de abertura do arquivo com o vetor\n");
        return 6;
    }

    if(carregaEntrada(descritorArquivoVetorX, &vetX, &linhasVet, &colunasVet)) //'vet' esta no escopo global
    return 7;
    //carrega o vetor
    copyVetX = (float*) malloc(sizeof(float) * linhasVet);
    if (!copyVetX) {
        fprintf(stderr, "Erro ao alocar memória para vetX\n");
        return 8;
    }
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexErr, NULL);
    pthread_cond_init (&cond, NULL);
    for(int i = 0; i<nthreads; i++){
        if(pthread_create(&tid[i], NULL, gaussjacobi, (void *) i)) {
            printf("Erro ao criar as threads!\n");
            exit(-1);
        }
    }
    
    //--espera todas as threads terminarem
    for (int i=0; i<nthreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Erro ao aguardar as threads\n"); 
            exit(-1); 
        } 
    } 
    printf("\nTerminou!!\n\n");

    for(int i = 0; i<qtdLinhas; i++){
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

  
    free(matA);
    free(vetB);
    free(vetX);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexErr);
    pthread_cond_destroy(&cond);
    
    return 0;
}