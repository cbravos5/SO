#include "ppos.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#define STACKSIZE 32768

task_t mainTask;//variavel para controlar tarefa da main
task_t *task_act;//ponteiro para controlar o cotexto atual (facilita a implementacao de funcoes)
task_t *task_q; //ponteiro para fila de tarefas
int id_counter = 1; //variavel que gera id's em sequencia(sem repetir)


void ppos_init ()
{
	task_q = NULL;//inicializa fila
	setvbuf (stdout, 0, _IONBF, 0);
	mainTask.prev = mainTask.next = NULL;
	mainTask.id = 0;
	queue_append((queue_t**)&task_q,(queue_t*)&mainTask);//adiciona tarefa main a fila
	#ifdef DEBUG
	printf("ppos_init: fila de tarefas iniciada\n"); 
	#endif
	task_act = &mainTask;
}


int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	getcontext(&(task->context));
	char *stack;
	stack = malloc(STACKSIZE) ;
    if (stack)
    {
       task->context.uc_stack.ss_sp = stack ;
       task->context.uc_stack.ss_size = STACKSIZE ;
       task->context.uc_stack.ss_flags = 0 ;
       task->context.uc_link = 0 ;
    }	
    else
    {
       return -1;
    }
	makecontext(&(task->context),(void*)start_func,1,arg);
	task->next = task->prev = NULL;
	task->id = id_counter;
	//
	#ifdef DEBUG
	printf("task_create: criou tarefa %d\n", task->id); 
	#endif
	//
	id_counter += 1;
	queue_append((queue_t**)&task_q, (queue_t*)task);
	return task->id;
}	


void task_exit (int exitCode)
{
	#ifdef DEBUG
	printf("task_exit: terminando tarefa %d\n", task_act->id); 
	#endif
	//
	queue_remove((queue_t**)&task_q,(queue_t*)task_act);
	task_switch(&mainTask);
}


int task_switch (task_t *task)
{
	#ifdef DEBUG
	printf("task_switch: mudando da tarefa %d para a tarefa %d\n", task_act->id,task->id); 
	#endif
	//
	task_t* swap_task = task_act; //DUVIDA
	task_act = task;
	swapcontext(&(swap_task->context),&(task_act->context));
	
	return 0;
}


int task_id ()
{
	return task_act->id;
}