#include <stdio.h>
#include <stdlib.h>
#include "glassdefs.h"

void runtime_error(char* error_text) {
	fprintf(stderr, "runtime error:\n%s\n", error_text);
	exit(1);
}

// stack operations

v_list init_stack() {
	// initialize an empty stack
	v_list stack;
	val* vs = (val*) malloc(1000 * sizeof (val));
	if (!stack) runtime_error("could not malloc stack in init_stack");
	if (!vs) runtime_error("could not malloc vs in init_stack");
	// initialize a stack with last_i -1, alloc 1000 vals, and val pointer vs
	stack = (v_list) {-1, 1000 * sizeof (val), vs};
	return stack;
}

void push(v_list* stack, val x) {
	// push a value to stack, adding memory as necessary
	stack->last_i++;

	if ((stack->last_i * sizeof (val)) >= stack->alloc) {
		stack->alloc += 1000 * sizeof (val);
		stack->vs = (val*) realloc(stack->vs, stack->alloc);
		if (!stack->vs) runtime_error("could not realloc stack memory in push");
	}

	stack->vs[stack->last_i] = x;
}

val pop(nv_list* stack) {
	// pop a value from stack (LIFO), freeing memory as necessary
	if (stack->last_i < 0) runtime_error("cannot pop from empty stack");
	
	val res;
	res = stack->vs[stack->last_i];
	stack->last_i--;

	// if there are 1500 fewer vals than there is allocated space for
	if((stack->last_i * sizeof (val)) < (stack->alloc - (1500 * sizeof (val)))) {
		stack->alloc -= 1500 * sizeof (val);
		stack->vs = realloc(stack->vs, stack->alloc);
	}

	return res;
}