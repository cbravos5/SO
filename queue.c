#include "queue.h"
#include <stdio.h>


void queue_append (queue_t **queue, queue_t *elem)
{
	if(queue == NULL)
		fprintf(stderr, "%s", "ERRO\nFila nao existe\n");
	else if(elem == NULL)
		fprintf(stderr, "%s", "ERRO\nElemento nao existe\n");
	else if(elem->next != NULL)
		fprintf(stderr, "%s", "ERRO\nElemento ja pertence a uma fila\n");
	else
	{
		if(*queue == NULL)//fila vazia
		{
			*queue = elem;
			elem->prev = elem->next = elem;
		}	
		else
		{
			queue_t *aux = *queue;
			while(aux->next != *queue)
				aux = aux->next;
			aux->next = elem;
			elem->prev = aux;
			elem->next = *queue;
			*queue->prev = elem;
		}
	}
}


int queue_size (queue_t *queue)
{
	if(*queue == NULL)
		return 0;
	queue_t *aux = *queue->next;
	int i = 1;
	while(aux != *queue)
	{	
		i += 1;
		aux = aux->next;
	}
	return i; 
}


queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
	if(queue == NULL){
		fprintf(stderr, "%s", "ERRO\nFila nao existe\n");
		return NULL;
	}
	else if(*queue == NULL){
		fprintf(stderr, "%s", "ERRO\nFila vazia\n");
		return NULL;
	}
	else if(elem == NULL){
		fprintf(stderr, "%s", "ERRO\nElemento nao existe\n");
		return NULL;
	}
	else //verificar se pertence a fila
	{
		int tamFila = queue_size(*queue);
		int i;
		queue_t *aux = *queue;
		for(i = 1; i <= tamFila; i++)//ao saber o tamanho da fila pode-se tratar os casos especificos
		{							 //baseado na posicao em que se encontra o 'i'					
			if(aux == elem)
				break;
			aux = aux->next;		
		}
		if(i == tamFila + 1){
			fprintf(stderr, "%s", "ERRO\nElemento nao pertence a fila\n");
			return NULL;	
		}
		if(tamFila == 1)
			*queue = NULL;
		else{
			elem->prev->next = elem->next;
			elem->next->prev = elem->prev;
			if(i == 1)//caso elem esteja na primeira posicao
			*queue = elem->next;
		}	
		elem->next = elem->prev = NULL;
		return elem;
	}								
}


void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
	printf("%s\n", name);
	if(*queue == NULL)
		printf("%s\n", "Fila vazia");
	else
	{
		queue_t *aux = *queue;
		do{
			print_elem(aux);
			aux = aux->next;	
		}while(aux != *queue)
	}	
}










