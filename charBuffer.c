#include "charBuffer.h"
#include <stdlib.h>

void initBuffer(CharBuffer_t* buffer) {
	buffer->content = (char*) malloc(BUFF_INIT_SIZE);
	buffer->size = BUFF_INIT_SIZE;
}

void resizeBuffer(CharBuffer_t* buffer) {
	buffer->content = (char*) realloc(buffer->content, buffer->size * BUFF_MULT);
	buffer->size *= BUFF_MULT;
}

void freeBuffer(CharBuffer_t* buffer) {
	free(buffer->content);
	buffer->content = NULL;
	buffer->size = 0;

}
