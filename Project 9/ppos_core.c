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
task_t *task_q = NULL; 

//ponteiro para a fila de tarefas prontas
task_t *prontas = NULL;

//ponteiro para fila de tarefas dormindo
task_t *sleeping_queue = NULL;

//variavel que gera id's em sequencia(sem repetir)
int id_counter = 1;

//contador de tarefas de usuario
int user_tasks = 0; 

//calcula o tempo do sitema baseado nos ticks
int system_time = 0;

//variavel global para controlar numero de ticks das tarefas de usuario
int ticks = 20;

//estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

//estrutura de inicialização to timer
struct itimerval default_timer ;

//estrutura para gerar um temporizador nulo
struct itimerval zero_timer = {0};

void tick_handler(int signal);

void verify_sleeping();



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
			next_task->state = 'r';
			ticks = 20;
			task_switch(next_task);
			switch(next_task->state)
			{
				//tarefa terminada; remove da fila de prontas e libera memoria.
				case 't': queue_remove((queue_t**)&prontas,(queue_t*)next_task);
						  free(next_task->context.uc_stack.ss_sp);
						  next_task = NULL; break;
				//tarefa volta para o final da fila de prontas.
				case 'p': queue_remove((queue_t**)&prontas,(queue_t*)next_task);
						  queue_append((queue_t**)&prontas,(queue_t*)next_task);
						  break;
			}
		}
		verify_sleeping();
	}
	task_exit(0);
}

void ppos_init ()
{
	task_create(&disp, dispatcher, 0);//cria dispatcher

	setvbuf (stdout, 0, _IONBF, 0);
	mainTask.prev = mainTask.next = NULL;
	mainTask.id = 0;
	mainTask.type = 'u';
	task_setprio(&mainTask,0);
	mainTask.exec_time = system_time; 
	mainTask.proc_time = 0;
	mainTask.activate_count = 0;
	mainTask.waitingQueue = NULL;
	mainTask.exit_code = 0;
	mainTask.sleep_out = 0;
	mainTask.state = 'p';
	user_tasks++;
	task_act = &mainTask;

	queue_append((queue_t**)&prontas,(queue_t*)&mainTask);//adiciona tarefa main a fila
	#ifdef DEBUG
	printf("ppos_init: fila de tarefas iniciada\n"); 
	#endif
	

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
	default_timer.it_value.tv_usec = TIME_TICK;      // primeiro disparo, em micro-segundos
  	default_timer.it_value.tv_sec  = 0 ;      	 	// primeiro disparo, em segundos
  	default_timer.it_interval.tv_usec = TIME_TICK;   // disparos subsequentes, em micro-segundos
  	default_timer.it_interval.tv_sec  = 0 ;   	 	// disparos subsequentes, em segundos

  	if (setitimer (ITIMER_REAL, &default_timer, 0) < 0)
  	{
    	perror ("Erro em setitimer: ") ;
    	exit (1) ;
  	}
  	task_yield();
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
	task->waitingQueue = NULL;
	task->exit_code = 0;
	task->sleep_out = 0;
	task->state = 'p';
	//
	#ifdef DEBUG
	printf("task_create: criou tarefa %d\n", task->id); 
	#endif
	//
	id_counter += 1;
	if (task != &disp)//Verificar se a tarefa criada eh uma tarefa de usuario, se for, incrementa user_tasks
	{				  // e adiciona a fila de prontas
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
	//incializacao variaveis de tempo
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
	//libera tarefas que estao aguardando termino
	task_t* aux = NULL;
	while(queue_size((queue_t*)task_act->waitingQueue) > 0)
	{
		aux = task_act->waitingQueue;
		aux = (task_t*)queue_remove((queue_t**)&task_act->waitingQueue,(queue_t*)aux);
		aux->state = 'p';
		queue_append((queue_t**)&prontas,(queue_t*)aux);
	}
	task_act->exit_code = exitCode;

	/////////////////////////////////////////////////////////////////////////////////
	//Impressao conteudo de temporizacao
	/////////////////////////////////////////////////////////////////////////////////
	task_act->proc_time = task_act->proc_time + (system_time - task_act->proc_time_in);
	task_act->exec_time = system_time - task_act->exec_time;
	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
				task_act->id, task_act->exec_time, task_act->proc_time, task_act->activate_count);
	/////////////////////////////////////////////////////////////////////////////////

	if (task_act->type != 's')//Verificar se a tarefa terminada eh uma tarefa de usuario, se for, 
	{						  //decrementa user_tasks e volta pro dispatcher
		task_act->state = 't';			
		user_tasks--;
		task_switch(&disp);	
	}	
	else
	{
		queue_remove((queue_t**)&task_q,(queue_t*)task_act);
		exit (0);
	}	
}


int task_switch (task_t *task)
{
	//calcula tempo de processando = hora atual - hora entrada
	task->proc_time_in = system_time;
	task->activate_count++;
	task_act->proc_time = task_act->proc_time + (system_time - task_act->proc_time_in); 

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

void task_yield()
{	
	if(task_act->type != 's')
		task_act->state = 'p';
	task_switch(&disp);
}

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


int task_join (task_t *task)
{
	if(task->state == 't')
		return -1;
	//tarefa retirada da fila de pronts e colocada na fila de espera com estado atualizado
	setitimer (ITIMER_REAL, &zero_timer, 0);//temporizador nulo para evitar preempcao
	queue_remove((queue_t**)&prontas,(queue_t*)task_act);
	task_act->state = 's';
	queue_append((queue_t**)&task->waitingQueue,(queue_t*)task_act);
	setitimer (ITIMER_REAL, &default_timer, 0);//retorna ao temporizador original

	task_switch(&disp);
	return task->exit_code;
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

void task_sleep (int t)
{
	task_act->sleep_out = systime() + t;
	queue_remove((queue_t**)&prontas,(queue_t*)task_act);
	task_act->state = 's';
	queue_append((queue_t**)&sleeping_queue,(queue_t*)task_act);
	task_switch(&disp);
}

void verify_sleeping()
{
	task_t *aux_in = sleeping_queue; //possivel tarefa a ser acordada
	task_t *aux_out = NULL; //tarefa a ser colocada na fial de prontas
	int size = queue_size((queue_t*)sleeping_queue);
	for (int i = 0; i < size; ++i) 
	{
		if (aux_in->sleep_out <= systime())
		{
			aux_out = aux_in;
			aux_in = aux_in->next;
			aux_out = (task_t*)queue_remove((queue_t**)&sleeping_queue,(queue_t*)aux_out);
			aux_out->state = 'p';
			queue_append((queue_t**)&prontas,(queue_t*)aux_out);
		}
		else
			aux_in = aux_in->next;
	}
}

unsigned int systime()
{
	return system_time;
}

