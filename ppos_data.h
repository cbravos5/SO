//GRR20186313 Chrystopher Naves Bravos


// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   char type; //identificador do tipo de tarefa(u = user, s = sistem)
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   short int prio; //prioridade da tarefa
   short int prio_d;//prioridade dinamica da tarefa
   int exec_time; //tempo de execucao da tarefa
   int proc_time_in; //hora em que a tarefa iniciou o processamento (auxilia no calculo do processamento total)
   int proc_time; //tempo de processamento total
   int activate_count; //numero de ativacoes da tarefa
   struct task_t *waitingQueue; //fila de tarefas que estao aguardando esta terminar
   int exit_code; //campo para retorno do exitCode para as funcoes da fila join
   int sleep_out; //campo para definir quando a tarefa devera ser acordada
   char state; //campo pra definir estado da tarefa
   // ... (outros campos serão adicionados mais tarde)

} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int count; //contador do semaforo
  task_t *waitingQueue;//fila de tarefas esperando liberacao do semaforo
  int active; //variavel para saber se o semaforo esta ativo
  int exit_code; //variavel para gerar retorno de sem_down
  int lock;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

//estrutura para um buffer circular
typedef struct 
{
  int max;
  int read;
  int write;
  int size;
  int full;
  void *buffer;
}CircularBuffer;

// estrutura que define uma fila de mensagens
typedef struct
{
  semaphore_t s_item,s_vaga,s_buffer;
  CircularBuffer *buff;
  char state;
} mqueue_t ;

#endif

