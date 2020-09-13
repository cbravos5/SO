#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ppos.h"

task_t      p1, p2, p3, c1, c2;
semaphore_t s_item, s_buffer, s_vaga ;
int lower = 0, upper = 99;
typedef struct
{
	int *next,*prev;
	int item;
} int_queue;
int_queue *buffer = NULL;

void produtor(void * arg)
{
   int item;
   int_queue *aux = NULL;
   while (1)
   {
      task_sleep (1000);
      aux = malloc(sizeof(int_queue*));
      item = (rand() % (upper - lower + 1)) + lower; 
      sem_down (&s_vaga);

      sem_down (&s_buffer);
      aux->item = item;
      queue_append((queue_t**)&buffer,(queue_t*)aux);
      sem_up (&s_buffer);
      printf("Produtor %s produziu %d\n",(char *)arg,item);
      sem_up (&s_item);
   }
}


void consumidor(void * arg)
{
   int item;
   int_queue *aux = NULL;
   while (1)
   {
      sem_down (&s_item);

      sem_down (&s_buffer);
      aux = buffer;
      aux = (int_queue*)queue_remove((queue_t**)&buffer,(queue_t*)aux);
      sem_up (&s_buffer);

      sem_up (&s_vaga);
      item = aux->item;
      printf("                            Consumidor %s consumiu %d\n",(char *)arg,item );
      free(aux);
      task_sleep (1000);
   }
}


int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // cria semaforos
   sem_create (&s_item, 0) ;
   sem_create (&s_vaga, 5) ;
   sem_create (&s_buffer, 1);

   srand(time(0)); 

   // cria tarefas
   task_create (&p1, produtor, "P1") ;
   task_create (&p2, produtor, "P2") ;
   task_create (&p3, produtor, "P3") ;
   task_create (&c1, consumidor, "C1") ;
   task_create (&c2, consumidor, "C2") ;

   // aguarda a1 encerrar
   task_join (&p1) ;

   printf ("main: fim\n") ;
   task_exit (0) ;

   exit (0) ;
}