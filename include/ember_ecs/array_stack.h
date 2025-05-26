#ifndef ARRAY_STACK_H_INCLUDED
#define ARRAY_STACK_H_INCLUDED
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * A fake FILO array stack. It does not shrink, it just reduces it's
 * count when as_pop is called.
 */
typedef struct {
	size_t size;
	uint32_t count;
	uint32_t cap;
	void* data;
} ArrayStack;

/**
 * Create an ArrayStack on the heap and return a pointer to it.
 */
ArrayStack * as_create(size_t size);

/**
 * Destroy an ArrayStack, removing it from heap.
 */
void as_destroy(ArrayStack *as);

/**
 * Pop the first item from the stack.
 */
void * as_pop(ArrayStack *as);

/**
 * Push data onto the stack.
 */
void as_push(ArrayStack *as, void* data);

#endif
