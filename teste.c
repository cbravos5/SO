#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct 
{
	int max;
	int read;
	int write;
	int size;
	void *buffer;
}CircularBuffer;

CircularBuffer* Init_Buffer(int max,int size)
{
	CircularBuffer* p = malloc(sizeof(CircularBuffer*));
	p->max = max;
	p->write = p->read = 0;
	p->size = size;
	p->buffer = malloc(max*sizeof(size));
	return p;
}

void read_buffer(CircularBuffer* C_buffer, void*value)
{
	int aux = C_buffer->read;
	if (C_buffer->read == C_buffer->max-1) {C_buffer->read = 0;}
	else {C_buffer->read++;}
	memcpy(value,C_buffer->buffer + C_buffer->size*aux,sizeof(C_buffer->size));
}

void write_buffer(CircularBuffer* C_buffer,void*value)
{
	int aux = C_buffer->write;
	if (C_buffer->write == C_buffer->max-1) {C_buffer->write = 0;}
	else {C_buffer->write++;}
	memcpy(C_buffer->buffer+C_buffer->size*aux, value, sizeof(C_buffer->size));
}

int main(int argc, char const *argv[])
{
	int j;
	CircularBuffer* bff = Init_Buffer(10,sizeof(int));
	for (int i = 0; i < 10; ++i)
	{
		printf("escrevendo %d no buffer\n",i*2 );
		int aux = i*2;
		write_buffer(bff,&aux);
	}
	usleep(1000000);
	for (int i = 0; i < 10; ++i)
	{
		read_buffer(bff,&j);
		printf("lendo %d no buffer\n",j );
	}
	return 0;
}