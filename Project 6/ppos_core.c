//GRR20186313 Chrystopher Naves Bravos

#include "ppos.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#define STACKSIZE 32768
#define TIME_TICK 1000

//variavel para controlar tarefa da main
task_t mainTask;

//ponteiro para o dispatcher
task_t disp;

//ponteiro para controlar o cotexto atual (facilita a implementacao de funcoes)
task_t *task_act;

//ponteiro para fila de tarefas
task_t *task_q; 

//ponteiro para a fila de tarefas prontas
task_t *prontas = NULL;

//ponteiro pra uma tarefa que foi terminada e memoria precisa ser liberada
task_t *terminate = NULL; 

//variavel que gera id's em sequencia(sem repetir)
int id_counter = 1;

//contador de tarefas de usuario
int user_tasks; 

//calcula o tempo do sitema baseado nos ticks
int system_time = 0;

//variavel global para controlar numero de ticks das tarefas de usuario
int ticks = 20;

//estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

//estrutura de inicialização to timer
struct itimerval timer ;


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
	if(task_act->type != 's')
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
		//
		//Loop para envelhecer as tarefas que nao foram escolhidas
		aux = prontas;
		for(int i = 0; i < size; i++)
		{
			aux->prio_d = (aux->prio_d - 1);
			aux = aux->next;
		}
		task_ready->prio_d = task_getprio(task_ready);
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
			ticks = 20;
			task_switch(next_task);
			if(terminate != NULL)
			{
				free(terminate->context.uc_stack.ss_sp);
				terminate = NULL;
			}
		}
	}
	task_exit(0);
}

unsigned int systime()
{
	return system_time;
}

void tick_handler(int signal)
{
	system_time++;
	if(task_act->type != 's')
	{
		ticks -= 1;
		if(ticks == 0)
			task_yield();
	}
}

void ppos_init ()
{
	task_q = NULL;//inicializa fila
	setvbuf (stdout, 0, _IONBF, 0);
	mainTask.prev = mainTask.next = NULL;
	mainTask.id = 0;
	mainTask.type = 's';
	queue_append((queue_t**)&task_q,(queue_t*)&mainTask);//adiciona tarefa main a fila
	#ifdef DEBUG
	printf("ppos_init: fila de tarefas iniciada\n"); 
	#endif
	
	task_create(&disp, dispatcher, 0);//cria dispatcher
	task_act = &mainTask;

	//act handler
	action.sa_handler = tick_handler;
  	sigemptyset (&action.sa_mask) ;
  	action.sa_flags = 0 ;
  	if (sigaction (SIGALRM, &action, 0) < 0)
  	{
    	perror ("Erro em sigaction: ") ;
    	exit (1) ;
  	}

	//tick set
	timer.it_value.tv_usec = TIME_TICK;      // primeiro disparo, em micro-segundos
  	timer.it_value.tv_sec  = 0 ;      	 	// primeiro disparo, em segundos
  	timer.it_interval.tv_usec = TIME_TICK;   // disparos subsequentes, em micro-segundos
  	timer.it_interval.tv_sec  = 0 ;   	 	// disparos subsequentes, em segundos

  	if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  	{
    	perror ("Erro em setitimer: ") ;
    	exit (1) ;
  	}
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
		task->type = 'u';
		queue_append((queue_t**)&prontas, (queue_t*)task);
	}
	else
	{
		task->type = 's';
		queue_append((queue_t**)&task_q, (queue_t*)task);
	}
	//incializaocao variaveis de tempo
	task->exec_time = system_time; 
	task->proc_time = 0;
	task->activate_count = 0;
	return task->id;
}	


void task_exit (int exitCode)
{
	#ifdef DEBUG
	printf("task_exit: terminando tarefa %d\n", task_act->id); 
	#endif
	//
	task_act->proc_time = task_act->proc_time + (system_time - task_act->proc_time_in);
	task_act->exec_time = system_time - task_act->exec_time;
	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
				task_act->id, task_act->exec_time, task_act->proc_time, task_act->activate_count);
	if (task_act->type != 's')//Verificar se a tarefa terminada eh uma tarefa de usuario, se for, 
	{						//decrementa user_tasks e volta pro dispatcher
		user_tasks--;
		terminate = task_act; //tarefa sera liberada memoria no dispatcher
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
	//caso nao seja um switch para a main, a tarefa deve ser incializada com uma hora de entrada no processamento
	//e deve ser adicionado mais uma "posse" do processamento
	if(task != &mainTask)
	{
		task->proc_time_in = system_time;
		task->activate_count++;
	}
	if(task_act != &mainTask)
		task_act->proc_time = task_act->proc_time + (system_time - task_act->proc_time_in); //calcula tempo de processando = hora atual - hora entrada

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