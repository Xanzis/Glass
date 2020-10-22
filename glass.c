#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "glassdefs.h"
#include "parser.h"
#include "runtime.h"

void glass_error(char* err_text) {
	fprintf(stderr, "Error in glass.c: %s\n", err_text);
	exit(0);
}

void interpret(glass_env env) {
	v_list* stack = init_stack();

	int main_idx = get_class_idx(env, find_name(env.names, "M"));
	int m_idx = get_func_idx(env, find_name(env.names, "M"), find_name(env.names, "m"));
	if (m_idx < 0) glass_error("cannot find M.m");

	object_t* main_obj = init_object(env, main_idx, stack);

	func_t main_func = (func_t) {main_idx, m_idx, main_obj};

	execute_function(env, main_func, stack);
}

int main(int argc, char *argv[] ) {
	if (argc != 2) glass_error("glass takes exactly two arguments");

	glass_env env = parse_file(argv[1]);
	
	int main_idx = get_class_idx(env, find_name(env.names, "M"));
	printf("Class M is stored as class #%d\n", main_idx);
	printf("Class M has the following functions:\n");
	for (int i = 0; env.f_lookup[main_idx][i] >= 0; i++) {
		printf("%s ", env.names[env.f_lookup[main_idx][i]]);
	}
	printf("\n");

	printf("Program tokens:\n");
	print_tokens(env.tokens);

	printf("Here goes ...\n");
	interpret(env);

	free_env(env);

	return 1;
}