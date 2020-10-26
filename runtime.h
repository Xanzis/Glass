#ifndef RUNTIME_H
#define RUNTIME_H

#define STACK_INC 1000
#define STACK_DEC 1500

#define is_func_end(tok) ((tok.type == ASCII)&&(tok.data==']'))
#define is_loop_end(tok) ((tok.type == ASCII)&&(tok.data=='\\'))

#include <stdio.h>
#include <stdlib.h>
#include "glassdefs.h"

void runtime_error(char* error_text);
void runtime_error_verbose(glass_env* env, v_list* stack, int t_i, char* error_text);

v_list init_stack();
void push(v_list* stack, val x);
val pop(v_list* stack);

void print_stack(v_list* stack);
void print_func(glass_env* env, func_t f);
void print_loc(glass_env* env, int t_i);

void execute_A_function(int func_i, v_list* stack);
void execute_O_function(glass_env* env, int func_i, v_list* stack);
void execute_std_function(glass_env* env, func_t func, v_list* stack);

object_t* init_object(glass_env* env, int class_i, v_list* stack);
val* get_name_target(glass_env* env, val* obj_vals, val* locals, val n);
int execute_token(glass_env* env, object_t* obj, v_list* stack, val* lcl_vars, int t_i);
void execute_function(glass_env* env, func_t func, v_list* stack);

void runtime_error(char* error_text) {
	fprintf(stderr, "runtime error:\n%s\n", error_text);
	exit(1);
}

void runtime_error_verbose(glass_env* env, v_list* stack, int t_i, char* error_text) {
	printf("runtime error:\n%s\n", error_text);
	printf("state info follows:\n");
	print_loc(env, t_i);
	print_stack(stack);
	exit(1);
}

v_list init_stack() {
	// initialize an empty stack
	v_list stack;
	val* vs = (val*) malloc(STACK_INC * sizeof (val));
	if (!vs) runtime_error("could not malloc vs in init_stack");
	// initialize a stack with last_i -1, alloc STACK_INC vals, and val pointer vs
	stack = (v_list) {-1, STACK_INC * sizeof (val), vs};
	return stack;
}

void push(v_list* stack, val x) {
	// push a value to stack, adding memory as necessary
	stack->last_i++;

	if ((stack->last_i * sizeof (val)) >= stack->alloc) {
		stack->alloc += STACK_INC * sizeof (val);
		stack->vs = (val*) realloc(stack->vs, stack->alloc);
		if (!stack->vs) runtime_error("could not realloc stack memory in push");
	}

	if (x.type == STNG) {
		// any string pushed to the stack gets copied and a new alloc
		// TODO figure out when to properly free this (woo leaks) (hint: can't do it on pop())
		char* x_copy = (char*) malloc((strlen(x.stng) + 1) * sizeof (char));
		strcpy(x_copy, x.stng);
		stack->vs[stack->last_i] = (val) {STNG, .stng = x_copy};
	}
	else {
		stack->vs[stack->last_i] = x;
	}
}

val pop(v_list* stack) {
	// pop a value from stack (LIFO), freeing memory as necessary
	if (stack->last_i < 0) runtime_error("cannot pop from empty stack");
	
	val res;
	res = stack->vs[stack->last_i];
	stack->last_i--;

	// if there are STACK_DEC fewer vals than there is allocated space for
	if((int) (stack->last_i * sizeof (val)) < (int) (stack->alloc - (STACK_DEC * sizeof (val)))) {
		stack->alloc -= STACK_INC * sizeof (val);
		stack->vs = realloc(stack->vs, stack->alloc);
		if (!stack->vs) runtime_error("could not realloc stack");
	}

	return res;
}

void print_stack(v_list* stack) {
	printf("stack:\n");
	for (int i = stack->last_i; i >= 0; i--) {
		print_val(stack->vs[i]);
	}
	printf("end stack\n");
}

void print_func(glass_env* env, func_t f) {
	printf("function c:%s f:%s, obj c:%s\n", 
		env->names[env->c_lookup[f.class_i]],
		env->names[env->f_lookup[f.class_i][f.func_i]],
		env->names[env->c_lookup[f.obj->class_i]]);
}

void print_loc(glass_env* env, int t_i) {
	print_tokens(env->tokens);
	for (int i = 0; i < t_i; i++) printf(" ");
	printf("|\n");
}

void execute_A_function(int func_i, v_list* stack) {
	// execute a function of class A, with func_i indexing into the canonical function ordering
	// TODO: determine what the deal is with numbers - are they floating point? add handling
	// std_A_funcs[] = {"a", "s", "m", "d", "mod", "f", "e", "ne", "lt", "le", "gt", "ge", NULL};
	val x, y;
	y = pop(stack);
	if (func_i != 5) {
		// floor doesn't use two operands
		x = pop(stack);
		if (x.type != NUMB) runtime_error("arithmetic operands must be numbers");
	}
	if (y.type != NUMB) runtime_error("arithmetic operands must be numbers");

	switch (func_i) {
		case 0:
			push(stack, (val) {NUMB, (int) (x.numb + y.numb)});
		break;
		case 1:
			push(stack, (val) {NUMB, (int) (x.numb - y.numb)});
		break;
		case 2:
			push(stack, (val) {NUMB, (int) (x.numb * y.numb)});
		break;
		case 3:
			push(stack, (val) {NUMB, (int) (x.numb / y.numb)});
		break;
		case 4:
			push(stack, (val) {NUMB, (int) (x.numb % y.numb)});
		break;
		case 5:
			push(stack, (val) {NUMB, (int) (y.numb)});
		break;
		case 6:
			push(stack, (val) {NUMB, (int) (x.numb == y.numb)});
		break;
		case 7:
			push(stack, (val) {NUMB, (int) (x.numb != y.numb)});
		break;
		case 8:
			push(stack, (val) {NUMB, (int) (x.numb < y.numb)});
		break;
		case 9:
			push(stack, (val) {NUMB, (int) (x.numb <= y.numb)});
		break;
		case 10:
			push(stack, (val) {NUMB, (int) (x.numb > y.numb)});
		break;
		case 11:
			push(stack, (val) {NUMB, (int) (x.numb >= y.numb)});
		break;
		default:
		// this should be an unreachable state
		runtime_error("execute_A_function: bad func_i");
	}
}

void execute_O_function(glass_env* env, int func_i, v_list* stack) {
	// execute a function of class O, with func_i indexing into the canonical function ordering
	// {"o", "on", NULL};
	val x = pop(stack);
	switch (func_i) {
		case 0:
			if (x.type == NAME) {
				printf("%s\n", env->names[x.name]);
			}
			else if (x.type == STNG) {
				printf("%s\n", x.stng);
			}
			else runtime_error("output operand must be string or name");
		break;
		case 1:
			if (x.type != NUMB) runtime_error("output number operand must be number");
			printf("%d\n", x.numb);
		break;
		default:
		runtime_error("execute_O_function: bad func_i");
	}
}

void execute_std_function(glass_env* env, func_t func, v_list* stack) {
	// order of standard functions: (from parser.h:)
	// 1: "A", "S", "V", "O", "I"
	//std_A_funcs[] = {"a", "s", "m", "d", "mod", "f", "e", "ne", "lt", "le", "gt", "ge", NULL};
	//char* std_S_funcs[] = {"l", "i", "si", "a", "d", "e", "ns", "sn", NULL};
	//char* std_V_funcs[] = {"n", "d", NULL};
	//char* std_O_funcs[] = {"o", "on", NULL};
	//char* std_I_funcs[] = {"l", "c", "e", NULL};

	// i sincerely apologize for the appearance of this function.
	switch (func.class_i) {
		case 0:
			execute_A_function(func.func_i, stack);
		break;
		case 1:
			runtime_error("S class not yet supported");
		break;
		case 2:
			runtime_error("V class not yet supported");
		break;
		case 3:
			execute_O_function(env, func.func_i, stack);
		break;
		case 4:
			runtime_error("I class not yet supported");
		break;
		default:
		runtime_error("execute_std_function: bad class input");
	}
}

object_t* init_object(glass_env* env, int class_i, v_list* stack) {
	// allocate memory for an object, run its initializer if it exists, return a pointer
	object_t* res = (object_t*) malloc(sizeof (object_t));
	res->class_i = class_i;

	// a little backwards but this is how the other lookup function goes
	// TODO probably fix this
	if (class_i < 0) runtime_error("init_object: bad class index");
	int class_name_i = env->c_lookup[class_i];
	if (class_name_i < 0) runtime_error("init_object: bad class name");
	int func_name_i = find_name(env->names, "c__");
	if (func_name_i >= 0) {
		// find the constructor and run it (if the class has one)
		int f_i = get_func_idx(*env, class_name_i, func_name_i);
		if (f_i >= 0) {
#ifdef DEBUG
			printf("running constructor\n");
#endif
			execute_function(env, (func_t) {class_i, f_i, res}, stack);
		}
	}

	return res;
}

val* get_name_target(glass_env* env, val* obj_vals, val* locals, val n) {
	// given a name n, determine scope and return a pointer to the appropriate val to reference
	// TODO: this fails when looking up an object name? 
#ifdef DEBUG
	printf("get_name_target request: %s scope ", env->names[n.name]);
#endif
	if (n.type != NAME) runtime_error("get_name_target: name must be name");
	val* res;
	switch (env->scopes[n.name]) {
		case GLOBAL_SCOPE:
#ifdef DEBUG
		printf("GLOBAL_SCOPE\n");
#endif
		res = env->global_vars + n.name;
		break;
		case OBJECT_SCOPE:
#ifdef DEBUG
		printf("OBJECT_SCOPE\n");
#endif
		res = obj_vals + n.name;
		break;
		case FUNCTION_SCOPE:
#ifdef DEBUG
		printf("FUNCTION_SCOPE\n");
#endif
		res = locals + n.name;
		break;
		default:
		runtime_error("bad scope on attempted = assignment");
		return NULL;
	}
#ifdef DEBUG
	val ref = *res;
	if (ref.type == NO_VAL) printf("warning: no_val referenced (fine as assignment target)\n");
#endif
	return res;
}

int execute_token(glass_env* env, object_t* obj, v_list* stack, val* lcl_vars, int t_i) {
	// execute the t_i token of env. returns 1 if function should return on execution of the token
	// do not pass / tokens - these should be handled in execute_function
	token_t t = env->tokens[t_i];
	switch (t.type) {
		case NAME_IDX:
			push(stack, (val) {NAME, t.data});
		break;
		case STNG_IDX:
			push(stack, (val) {STNG, .stng = env->strings[t.data]});
		break;
		case STCK_IDX:
			if (stack->last_i < t.data) runtime_error("duplicate call overshoots stack");
			push(stack, stack->vs[t.data]);
		break;
		case NUMBER:
			push(stack, (val) {NUMB, t.data});
		break;
		case ASCII:
			// it's a generic command - lots to do here ...
			switch (t.data) {
				case ',':
					pop(stack);
				break;
				case '^':
					return 1;
				break;
				case '=':
				{
					// assign a value to a name
					val v = pop(stack);
					val n = pop(stack);
					*get_name_target(env, obj->vars, lcl_vars, n) = v;
				}
				break;
				case '!':
				{
					// initialize an object, assign to variable
					val c = pop(stack);
					val n = pop(stack);
					if ((n.type != NAME) || (c.type != NAME)) runtime_error("both ! operands must be names");
					val new_obj = (val) {OBJT, .objt = init_object(env, get_class_idx(*env, c.name), stack)};
					*get_name_target(env, obj->vars, lcl_vars, n) = new_obj;
				}
				break;
				case '.':
				{
					// pop an object name and a function name, push the function
					val f = pop(stack);
					val o = pop(stack);
					if ((o.type != NAME) || (f.type != NAME)) runtime_error("both . operands must be names");
					val obj_var = *get_name_target(env, obj->vars, lcl_vars, o);
					if (obj_var.type != OBJT) {
						print_val(o);
						print_val(obj_var);
						runtime_error_verbose(env, stack, t_i, "first . operand must be name of object variable");
					}
					// resolve the various mappings and push the function. TODO this is horribly clunky
					int class_i = obj_var.objt->class_i;
					int c_name_i = env->c_lookup[class_i];
					int func_i = get_func_idx(*env, c_name_i, f.name);
					func_t new_func = (func_t) {class_i, func_i, obj_var.objt};
					push(stack, (val) {FUNC, .func = new_func});
				}
				break;
				case '?':
				{
					// pop a function and run it
					val f = pop(stack);
					if (f.type != FUNC) runtime_error("operand of ? must be a function");
#ifdef DEBUG
					printf("running a new function!\n");
					print_loc(env, t_i);
					print_func(env, f.func);
					print_stack(stack);
#endif
					func_t func = f.func;
					// down the rabbit hole we go
					execute_function(env, func, stack);
				}
				break;
				case '*':
				{
					// pop a name, push a (scope-dependent) value
					val n = pop(stack);
					val res = *get_name_target(env, obj->vars, lcl_vars, n);
					// check that res has an assigned value
					if (res.type == NO_VAL) runtime_error("variable undefined in the current scope");
					push(stack, res);
				}
				break;
				case '$':
				{
					// pop a name, assign the current object to it
					val n = pop(stack);
					val o = (val) {OBJT, .objt = obj};
					*get_name_target(env, obj->vars, lcl_vars, n) = o;
				}
				break;
				default:
				runtime_error("execute_token: bad ascii token");
			}
		break;
		default:
		runtime_error("execute_token: bad token");
	}
	return 0;
}

void execute_function(glass_env* env, func_t func, v_list* stack) {
	// execute the function specified by func
	// handle loops internally
	object_t* obj = func.obj;

	// array of indices of / token of loops
	// loop_begins[0] is always the location of the current loop
	// note a valid loop will never begin at 0, because tokens[0] is { always
	int loop_begins[MAX_LOOP_DEPTH] = {0};


	if (func.class_i < 5) {
		// the class is one of the standard classes
		execute_std_function(env, func, stack);
	}

	else {
		val* locals = (val*) malloc(MAX_NAMES * sizeof (val));
		for (int i = 0; i < MAX_NAMES; i++) locals[i] = (val) {NO_VAL, 0};

		int t_i = env->f_locs[func.class_i][func.func_i];
		token_t cur_token;

		//print_tokens(env->tokens + t_i);

		cur_token = env->tokens[t_i];
		// main loop
		while (!is_func_end(cur_token)) {
			// handle things
			if ((cur_token.type == ASCII) && (cur_token.data == '/')) {
				// check if this is the first encounter with this loop
				// if so, update loop_begins
				if (loop_begins[0] != t_i) {
					// this is the first encounter of this loop begin
					// shuffle the loop begins down
					for (int i = MAX_LOOP_DEPTH - 1; i >= 1; i--) {
						loop_begins[i] = loop_begins[i - 1];
					}
					loop_begins[0] = t_i;
				}
				// handle rest of the loop logic
				// check the value of the following name
				t_i++;
				cur_token = env->tokens[t_i];
				if (cur_token.type != NAME_IDX) runtime_error("/ must be followed by name");
				val condition = *get_name_target(env, func.obj->vars, locals, (val) {NAME, cur_token.data});
				if (condition.type != NUMB) runtime_error("for now, only numbers supported as loop conditions");
				if (condition.numb) {
					t_i++;
				}
				else {
					// jump to after the loop
					cur_token = env->tokens[t_i];
					while(!is_loop_end(cur_token)) {
						t_i++;
						cur_token = env->tokens[t_i];
					}
					t_i++;
					// shuffle loop_begins over on exit of the loop
					for (int i = 0; loop_begins[i] && (i < (MAX_LOOP_DEPTH - 1)); i++) {
						loop_begins[i] = loop_begins[i + 1];
					}
				}
			}

			else if ((cur_token.type == ASCII) && (cur_token.data == '\\')) {
				// end of loop
				// hop back to the first /, let the next iteration take over the rest
				t_i = loop_begins[0];
			}

			else {
				// standard token
				//print_tok(cur_token);
				int should_return = execute_token(env, func.obj, stack, locals, t_i);
				if (should_return) {
					free(locals);
					return;
				}

				// just increment to next token on a standard operation
				t_i++;
			}
			cur_token = env->tokens[t_i];
		}
		// function ends naturally
		free(locals);
	}
}

#endif