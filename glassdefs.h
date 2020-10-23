#ifndef GLASS_DEFS_H
#define GLASS_DEFS_H

#include <string.h>
#include <ctype.h>

#define MAX_NAMES 256
#define MAX_CLASSES 256
#define MAX_FUNCS 256
#define MAX_LITERALS 256
#define MAX_PROGRAM 1024
#define MAX_LOOP_DEPTH 64

#define STD_LIBS 5 // number of standard classes

enum val_type {NO_VAL=0, FUNC, OBJT, NUMB, NAME, STNG, CMDS};
enum token_type {NO_TOKEN, ASCII, NAME_IDX, NUMBER, STNG_IDX, STCK_IDX};
enum scope_type {NO_SCOPE=0, GLOBAL_SCOPE, OBJECT_SCOPE, FUNCTION_SCOPE};

typedef struct val val;
typedef struct v_list v_list;
typedef struct glass_env glass_env;
typedef struct token_t token_t;

// objects and functions can be on the stack, so we need structs for the relevant attributes
// object structs have persistent state, function structs just have class and function names
typedef struct object_t object_t;
typedef struct func_t func_t;

void glassdefs_error(char* error_text);

void free_env(glass_env env);

int find_name(char** all_n, char* n);
int add_name(char** all_n, enum scope_type* scopes, char* n);

int get_class_idx(glass_env env, int class_name_idx);
int get_func_idx(glass_env env, int class_name_idx, int func_name_idx);

void print_tokens(token_t* toks);

struct func_t {
	int       class_i;
	int       func_i;
	object_t* obj;
};

struct val {
	enum val_type type;

	union {
		int       numb;
		int       name;
		char*     stng;
		object_t* objt;
		func_t    func;
	};
};

// data structure for the stack
struct v_list {
	int        last_i;
	size_t     alloc;
	val*       vs;
};

// data structure for parser output

struct token_t {
	enum token_type type;
	int data;
};

struct glass_env {
	char**   names;     // all names defined in the program (for debug purposes)
	enum scope_type* scopes; // each name has a scope (depends on first letter of name)
	int*     c_lookup;    // c_lookup[i] = n means the ith class has name n (in the name array)
	int**    f_lookup;   // f_lookup[c][i] = n means the number ith function of the cth class has name n
	int**    f_locs;    // f_locs[c][f] is index of first token of the fth function of cth class (after name)
	token_t* tokens;  // array of tokens forming the program. 

	char** strings;   // array of all string literals used in program
	val* global_vars; // for use during runtime
};

struct object_t {
	int class_i; // index of the class of which this is an instance
	val vars[MAX_NAMES]; // object variables. For now, storage allocated for all variables
	// TODO reduce overhead by only storing variables for names with scope=OBJECT_SCOPE
};

void glassdefs_error(char* error_text) {
	fprintf(stderr, "Error in glassdefs.h: %s\n", error_text);
	exit(1);
}

void free_env(glass_env env) {
	// free all the referenced memory in an env

	// can't free all names :( some are const char* from initialization
	// TODO fix this leak
	//for (int i = 0; env.names[i] && i < MAX_NAMES; i++) free(env.names[i]);
	//free(env.names);

	free(env.c_lookup);

	for (int i = 0; (env.f_lookup[i] >= 0) && i < MAX_CLASSES; i++) {
		free(env.f_lookup[i]);
		free(env.f_locs[i]);
	}
	free(env.f_lookup);
	free(env.f_locs);

	free(env.tokens);
	
	for (int i = 0; env.strings[i] && i < MAX_LITERALS; i++) free(env.strings[i]);
	free(env.strings);
	
	free(env.global_vars);
}

enum scope_type name_scope(char* n) {
	if (isupper(*n)) return GLOBAL_SCOPE;
	if (islower(*n)) return OBJECT_SCOPE;
	if (*n == '_') return FUNCTION_SCOPE;
	glassdefs_error("name_scope: bad name");
	return NO_SCOPE;
}

int find_name(char** all_n, char* n) {
	// finds name n (null-terminated) in all_n (NULL-terminated)
	// returns -1 on failure
	for (int i = 0; all_n[i]; i++) {
		if (!strcmp(all_n[i], n)) return i;
	}
	return -1;
}

int add_name(char** all_n, enum scope_type* scopes, char* n) {
	// adds the name to the end of all_n, returns the place it was added
	// checks if name is already there, if so returns existing location
	// allocates new memory for the name, sets all_n[i] pointer
	// returns -1 on failure
	int loc = find_name(all_n, n);
	if (loc >= 0) return loc;
	int i = 0;
	do {
		if (!all_n[i]) {
			// allocate space for the new name and 
			all_n[i] = (char*) malloc((strlen(n) * sizeof (char)) + 1);
			scopes[i] = name_scope(n);
			strcpy(all_n[i], n);
			return i;
		}
		i++;
	} while (i < MAX_NAMES);
	return -1;
}

int get_class_idx(glass_env env, int class_name_idx) {
	// given the name index of a suspected class, return the class' index in the env lookup
	// returns -1 on failure
	if (class_name_idx < 0) glassdefs_error("get_class_idx: bad class index input");
	int res = -1;
	for (int i = 0; (i < MAX_CLASSES) && (env.c_lookup[i]); i++) {
		if (env.c_lookup[i] == class_name_idx) res = i;
	}
	return res;
}

int get_func_idx(glass_env env, int class_name_idx, int func_name_idx) {
	// given name idx of a function, return the function's index in the lookup
	// returns -1 on failure
	if (class_name_idx < 0) glassdefs_error("get_func_idx: bad class index input");
	if (func_name_idx < 0) glassdefs_error("get_func_idx: bad function index input");
	int c_idx = get_class_idx(env, class_name_idx);
	if (c_idx < 0) glassdefs_error("get_func_idx: no such class");
	int res = -1;
	for (int i = 0; (i < MAX_FUNCS) && (env.f_lookup[c_idx][i]); i++) {
		if (env.f_lookup[c_idx][i] == func_name_idx) res = i;
	}
	return res;
}

void print_tok(token_t t) {
	if (t.type == ASCII) {
		putchar(t.data);
	}
	else {
		printf("\033[0;31m");
		switch (t.type) {
			case NAME_IDX:
				putchar('N');
			break;
			case NUMBER:
				putchar('#');
			break;
			case STNG_IDX:
				putchar('A');
			break;
			case STCK_IDX:
				putchar('T');
			break;
			default:
			putchar('?');
		}
		printf("\033[0m");
	}
}

void print_tokens(token_t* t) {
	// print a NO_TOKEN terminated array of tokens
	int i = 0;
	while(t[i].type != NO_TOKEN && i < MAX_PROGRAM) {
		print_tok(t[i]);
		i++;
	}
	putchar('\n');
}

void print_val(val v) {
	char* type_names[] = {"NO_VAL", "FUNC", "OBJT", "NUMB", "NAME", "STNG", "CMDS"};
	printf("type-%s-val-", type_names[v.type]);
	if ((v.type == NUMB) || (v.type == NAME)) {
		printf("%d\n", v.numb);
	}
	else if (v.type == STNG) {
		printf("%s\n", v.stng);
	}
	else if (v.type == FUNC) {
		printf("%d-%d\n", v.func.class_i, v.func.func_i);
	}
	else if (v.type == OBJT) {
		printf("%p\n", (void *) v.objt);
	}
	else printf("none\n");
}

#endif