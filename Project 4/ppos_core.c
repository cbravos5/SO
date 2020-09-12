//GRR20186313 Chrystopher Naves Bravos

#include "ppos.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#define STACKSIZE 32768

task_t mainTask;//variavel para controlar tarefa da main
task_t *task_act;//ponteiro para controlar o cotexto atual (facilita a implementacao de funcoes)
task_t *task_q; //ponteiro para fila de tarefas
task_t disp; //ponteiro para o dispatcher
task_t *prontas = NULL; //ponteiro para a fila de tarefas prontas
int id_counter = 1; //variavel que gera id's em sequencia(sem repetir)
int user_tasks; //contador de tarefas de usuario


void task_setprio (task_t *task, int prio) 
{
	task->prio = prio;
	task->prio_d = prio;
}


int task_getprio (task_t *task)
{
	if(task == NULL)
		return task_act->prio;
	return task->prio;
}

void task_yield()
{	
	if(task_act != &mainTask)
		queue_append((queue_t **)&prontas,(queue_t *)task_act);
	task_switch(&disp);
}

task_t* scheduler()
{
	//Primeiro verifica-se o tamanho da fila de prontas para evitar problemas
	int size = queue_size((queue_t *)prontas);
	/*Apos a verificacao eh feito um loop com duas variaveis: a com menor valor de prioridade e outra para auxiliar.
	Assim que a tarefa com maior prioridade for descoberta, ela eh removida da fila de prontas*/
	if(size > 0)
	{
		task_t *task_ready = prontas;
		task_t *aux = prontas;
		for(int i = 0; i < size; i++)
		{
			if (aux->prio_d < task_ready->prio_d)
				task_ready = aux;
			aux = aux->next;
		}

		//
		task_ready->prio_d = task_getprio(task_ready);
		//
		//Loop para envelhecer as tarefas que nao foram escolhidas
		aux = prontas;
		for(int i = 0; i < size; i++)
		{
			aux->prio_d = (aux->prio_d - 1);
			aux = aux->next;
		}
		return task_ready;
	}	
	else
		return NULL;
}

void dispatcher()
{
	task_t *next_task;
	while(user_tasks > 0)
	{
		next_task = scheduler();
		if (next_task != NULL)
		{
			queue_remove((queue_t**)&prontas,(queue_t*)next_task);
			task_switch(next_task);
		}
	}
	task_exit(0);
}

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
	task_create(&disp, dispatcher, 0);
	task_act = &mainTask;
}


int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	char *stack;
	getcontext(&(task->context));
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
	if (task != &disp && task != &mainTask)//Verificar se a tarefa criada eh uma tarefa de usuario, se for, incrementa user_tasks
	{										// e adiciona a fila de prontas
		user_tasks++;
		task_setprio(task,0);
		queue_append((queue_t**)&prontas, (queue_t*)task);
	}
	else
		queue_append((queue_t**)&task_q, (queue_t*)task);

	return task->id;
}	


void task_exit (int exitCode)
{
	#ifdef DEBUG
	printf("task_exit: terminando tarefa %d\n", task_act->id); 
	#endif
	//

	if (task_act != &disp)//Verificar se a tarefa terminada eh uma tarefa de usuario, se for, 
	{					//decrementa user_tasks e volta pro dispatcher
		user_tasks--;
		task_switch(&disp);	
	}	
	else
	{
		queue_remove((queue_t**)&task_q,(queue_t*)task_act);
		task_switch(&mainTask);
	}	
}


int task_switch (task_t *task)
{
	#ifdef DEBUG
	printf("task_switch: mudando da tarefa %d para a tarefa %d\n", task_act->id,task->id); 
	#endif
	//
	task_t* swap_task = task_act;
	task_act = task;
	if((swapcontext(&(swap_task->context),&(task_act->context))) == -1)
		return -1;
	return 0;
}


int task_id ()
{
	return task_act->id;
}