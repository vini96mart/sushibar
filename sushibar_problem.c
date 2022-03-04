#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>

#define THREADS 20

/* ------- SEMAFORO ------- */ 

//Variável cond do semáforo
typedef pthread_cond_t Cond;

//Declaração da variável mutex
typedef pthread_mutex_t Mutex;

//struct de tipo "Semaforo"
typedef struct
{
	int valor, wakeup; //valor atual e de despertar
	Mutex *mutex;      //mutex para gerenciar recurso compartilhado
	Cond *cond;        //condicional (como declarado acima)
}Semaforo;


//Iniciando o mutex
/*Na função abaixo, estaremos inicializando o mutex com uma
  alocação dinâmica. Se a função retornar 0, ela foi iniciada
  corretamente. Como foi passado o valor NULL no segundo
  argumento, os valores padrão da função que inicia pthread
  serão utilizados no lugar.*/

Mutex* make_mutex(void)
{
	int n;
	Mutex* mutex = (Mutex*)malloc(sizeof(Mutex));
	n = pthread_mutex_init(mutex, NULL);
	if(n != 0)
		perror("Mutex falhou!");
	return(mutex);
}

//Inicializador da cond (variável auxiliar do mutex)
Cond* make_cond(void)
{
	int n;
	Cond* cond = (Cond*)malloc(sizeof(Cond));
	n = pthread_cond_init(cond, NULL);
	if(n!=0)
		perror("Make falhou!");
	return(cond);
}

//Dá o sinal de execução em cond
void cond_signal(Cond* cond)
{
	int n = pthread_cond_signal(cond);
	if(n != 0)
		perror("post falhou!");
}

//Coloca cond em estado de espera
void cond_wait(Cond* cond, Mutex* mutex)
{
	int n = pthread_cond_wait(cond, mutex);
	if(n != 0)
		perror("Wait falhou!");
} 

//Construtor do semáforo
Semaforo* make_Semaforo(int valor)
{
	Semaforo* semaforo = (Semaforo*)malloc(sizeof(Semaforo)); //aloca a memória necessária para o tipo semaforo
	semaforo->valor = valor; //atribui o valor do semáforo de acordo com o valor que foi chamado na função
	semaforo->wakeup = 0; //wakeup vale 0 porque quando chega nesse valor, libera mais um espaço no semáforo
	semaforo->mutex = make_mutex(); //inicializa o mutex
	semaforo->cond = make_cond();  //inicializa o cond
  return(semaforo);
}

//Coloca o semáforo em estado de espera
void sem_wait(Semaforo *Semaforo)
{
	pthread_mutex_lock(Semaforo->mutex);
 
	Semaforo->valor--;
	if(Semaforo->valor < 0)
	{
		do
		{
			cond_wait(Semaforo->cond, Semaforo->mutex);
		}while(Semaforo->wakeup < 1);
		Semaforo->wakeup--;
	}
 
	pthread_mutex_unlock(Semaforo->mutex);
}

//Altera o valor do semáforo
void sem_signal(Semaforo* Semaforo)
{
	pthread_mutex_lock(Semaforo->mutex);
 
	Semaforo->valor++;
	if(Semaforo->valor <= 0)
	{
    Semaforo->wakeup++;
    cond_signal(Semaforo->cond);
	}
 
	pthread_mutex_unlock(Semaforo->mutex);
}


/* ---- MONTAGEM DO SUSHIBAR PROBLEM ---- */

Semaforo *chiclete;
Mutex *banana;
int esperar = 0; // status se as pessoas devem esperar (se a mesa estiver vazia = 0, quando ela fica cheia = 1)
int fila = 0; // numero de pessoas esperando
int qtd_comendo = 0; // quantidade de pessoas comendo

void* Sushibar(void *arg)
{     
	    pthread_mutex_lock(banana); //entra na região crítica
      
      //sleep(1);
      if (esperar == 1)        // se estiver cheio entra aqui
      {
   		  fila++; // mais uma pessoa esperando
        printf ("alguem acabou de entrar na fila \n");
        printf("quantidade de pessoas esperando : %d     quantidade de pessoas comendo : %d sinal de aguarde : %d \n",fila, qtd_comendo,esperar);
	  	  pthread_mutex_unlock(banana);
        sem_wait(chiclete);   //força a espera das pessoas (sai da região crítica e espera)
 	  	  fila--;
        printf ("alguem acabou de sair da fila \n");
       }
 

      qtd_comendo++;    //mais uma pessoa comendo
      
 	    if (qtd_comendo == 5)
      {
 		    esperar = 1;
        printf("alterou o esperar para 1 \n");
	    }
      
      printf ("alguem acabou de comecar a comer \n");
      printf("quantidade de pessoas esperando : %d     quantidade de pessoas comendo : %d sinal de aguarde : %d \n",fila, qtd_comendo,esperar);
	   
       if ((fila != 0) && (esperar == 0))   //se tiver pessoas esperando e nao precisar esperar, ele libera o semaforo
	 	    sem_signal(chiclete);
 	    else
     		pthread_mutex_unlock(banana);
	
      sleep((rand() % 5)); // valor aleatorio de sleep para criar aleatoriedade no tempo que as pessoas demoram para comer
      

      //come o sushi


      pthread_mutex_lock(banana);         //entra na região crítica de novo
 
 	    qtd_comendo--; // terminam de comer
      printf ("alguem acabou de comer \n");
      printf("quantidade de pessoas esperando : %d     quantidade de pessoas comendo : %d sinal de aguarde : %d \n",fila, qtd_comendo,esperar);
 	    
       if (qtd_comendo == 0)   // se todas as pessoas sairem da mesa esperar = 0
 		  {
        esperar = 0;
        printf("alterou o esperar para 0 \n");
 	    }

 	    if ((fila != 0) && (esperar == 0))    //verifica novamente  se tem pessoas esperando e se nao eh precisar esperar, ele libera o semaforo
	 	    sem_signal(chiclete);
 	    else
 		    pthread_mutex_unlock(banana); //sai da região crítica
	    
      //sleep(1);
}

/* --------- MAIN --------- */

int main(){ 

  int i = 0;
  void *arg;
  banana = make_mutex();                    //inicializa o mutex
	chiclete = make_Semaforo(0);              //inicializa o semaforo chiclete
  pthread_t thread_Clientes[THREADS];

	for (i = 0; i <= THREADS-1; i++)
	{
		sleep(random() % 2);
		pthread_create(&thread_Clientes[i], 0, Sushibar, arg);
	}
    
	for (i=0; i < THREADS-1; i++)
	{
		pthread_join(thread_Clientes[i], NULL);
	}
	sleep(1);
	return 1;
}