#ifndef CHARBUFFER_H
#define CHARBUFFER_H

typedef struct{
	char* content;
	int size;
}CharBuffer_t;

#define BUFF_INIT_SIZE 20
#define BUFF_MULT 2

void initBuffer(CharBuffer_t* buffer);

void resizeBuffer(CharBuffer_t* buffer);

void freeBuffer(CharBuffer_t* buffer);


#endif //CHARBUFFER_H
