#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "glassdefs.h"

#ifndef PARSER_H
#define PARSER_H

void parse_error(char* err_text) {
	fprintf(stderr, "Error in parser.h: %s\n", err_text);
	exit(1);
}

void check_ptr(void* x) {
	if (!x) {
		fprintf(stderr, "NULL!\n");
		exit(1);
	} 
}

void check_par(char* start, char a, char b, char end) {
	// checks that parens/brackets/braces are matched
	// example: for checking parens, a=( and b=)
	// TODO: add handling for if a string has mismatched parens
	int ps = 0;
	for (int i = 0; start[i] != end; i++) {
		if (!start[i]) parse_error("overran string in check_par");
		if (start[i] == a) ps++;
		else if (start[i] == b) ps--;
		if (ps < 0) parse_error("mismatched");
	}
	if (ps != 0) parse_error("mismatched");
}

void read_name(char* buff, char* start, int lim) {
	// copies a name (length < lim-1) possibly in parens into buff
	if (*start == '(') {
		start++;
		for (int i = 0; start[i] != ')'; i++) {
			if (i >= (lim - 2)) parse_error("name too long");
			buff[i] = start[i];
			buff[i + 1] = 0;
		}
	}
	else {
		buff[0] = *start;
		buff[1] = 0;
	}
}

char* read_string(char* start) {
	// parses a string in ""s, allocates new memory for it
	// start shoud point to first delimiter
	char tmp[64] = {0};
	start++;
	for (int i = 0; start[i] != '"'; i++) {
		tmp[i] = start[i];
	}
	char* res = (char*) malloc(strlen(tmp) + 1);
	strcpy(res, tmp);
	return res;
}

int read_number(char* start) {
	// parses a number in <>s or ()s
	// start should point to first delimiter
	char tmp[64] = {0};
	start++;
	for (int i = 0; (start[i] != ')') && (start[i] != '>'); i++) {
		tmp[i] = start[i];
	}
	return atoi(tmp);
}

token_t make_token(glass_env* env, char* start) {
	// take pointed-to string and tokenize the most immediate chunk
	// adds any encountered names
	// this function assumes any syntax problems have been caught by now
	//   -- particularly paren mismatches
	token_t res;
	char name_buff[64] = {0};
	switch (*start) {
		case '(':
			if (isdigit(start[1])) {
				// it's a number in parens - a stack retrieve
				res.type = STCK_IDX;
				res.data = read_number(start);
				if (res.data < 0) parse_error("error reading number");
			}
			else {
				// it's a name in parens
				// TODO add check for illegal characters?
				read_name(name_buff, start, 64);
				int name_idx = add_name(env->names, name_buff);
				if (name_idx < 0) parse_error("couldn't add name in make_token");
				res.type = NAME_IDX;
				res.data = name_idx;
			}
			break;
		case '<':
			// it's a number in <>s - a number literal
			res.type = NUMBER;
			res.data = read_number(start);
			if (res.data < 0) parse_error("error reading number");
			break;
		case '"':
			// it's a string - allocate space, add to env->strings, fill out index
			// written as a block because it opens with a declaration TODO bad hack
		{
			char* new_str = read_string(start);
			int str_idx = 0;
			while (env->strings[str_idx] && str_idx < MAX_LITERALS) str_idx++;
			if (str_idx >= MAX_LITERALS) parse_error("MAX_LITERALS exceeded");
			env->strings[str_idx] = new_str;
			res.type = STNG_IDX;
			res.data = str_idx;
		}
			break;
		default:
		if (isalpha(*start)) {
			// single-character name
			// add the name to the name list and set token accordingly
			read_name(name_buff, start, 64);
			int name_idx = add_name(env->names, name_buff);
			if (name_idx < 0) parse_error("couldn't add name in make_token");
			res.type = NAME_IDX;
			res.data = name_idx;
		}
		else {
			// generic ascii - could be command, brackets, whatever
			// set the type and value
			res.type = ASCII;
			res.data = (int) *start;
		}
	}
	return res;
}

char* end_of_token(char* start) {
	// returns a pointer to the character after a chunk
	switch (*start) {
		case '<':
			while(*(start++) != '>');
			return start;
		case '"':
			start++;
			while(*(start++) != '"');
			return start;
		case '(':
			while(*(start++) != ')');
			return start;
		default:
		return start + 1;
	}
}

char* read_clean(char* loc) {
	// read the file, sans whitespace and comments
	char* buff = NULL;
	char* res = NULL;
	int len;

	FILE* f = fopen(loc, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		len = ftell (f);
  		fseek (f, 0, SEEK_SET);

  		buff = (char*) malloc(len + 1);
  		res  = (char*) malloc(len + 1);

  		if (res) memset(res, 0, len+1);
  		else parse_error("malloc error in read_clean");

  		if (buff) fread(buff, sizeof (char), len, f);
  		fclose(f);
	}

	if (!buff) parse_error("couldn't read file");

	int in_comment = 0;
	int in_string = 0;
	int res_i = 0;
	char c;

	for (int i = 0; i < len; i++) {
		// strip whitespace and commented text
		// likely to break - TODO test
		c = buff[i];

		// read the following lines if you DARE
		if (!in_comment && (c == '"')) in_string = !in_string;
		if (!in_string && (c == '\'') && !in_comment) in_comment = 1;
		if (!in_comment && !isspace(c)) res[res_i++] = c;
		if (!in_string && (c == '\'') && in_comment) in_comment = 0;
	}

	free(buff);
	return res;
}

void alloc_env(glass_env* env) {
	// allocate all the various arrays and nested arrays in an env
	// initialize everything to 0

	env->names = (char**) malloc(MAX_NAMES * sizeof (char*));
	memset(env->names, 0, MAX_NAMES * sizeof (char));
	env->names[0] = "~";
	// ^this is crucial; most subroutines depend on name_idx != 0 for valid names
	// TODO: needs fixing? Could initialize lookups with -1s but that's inconvenient

	env->c_lookup = (int*) malloc(MAX_CLASSES * sizeof (int));
	memset(env->c_lookup, 0, MAX_CLASSES * sizeof (int));

	env->f_lookup = (int**) malloc(MAX_CLASSES * sizeof (int*));
	for (int i = 0; i < MAX_CLASSES; i++) {
		env->f_lookup[i] = (int*) malloc(MAX_FUNCS * sizeof (int));
		for (int j = 0; j < MAX_FUNCS; j++) {
			env->f_lookup[i][j] = -1;
		}
	}

	env->f_locs = (int**) malloc(MAX_CLASSES * sizeof (int*));
	for (int i = 0; i < MAX_CLASSES; i++) {
		env->f_locs[i] = (int*) malloc(MAX_FUNCS * sizeof (int));
		memset(env->f_locs[i], 0, MAX_FUNCS * sizeof (int));
	}

	env->tokens = (token_t*) malloc(MAX_PROGRAM * sizeof (token_t));
	memset(env->tokens, 0, MAX_PROGRAM * sizeof (token_t));

	env->strings = (char**) malloc(MAX_LITERALS * sizeof (char*));
	memset(env->strings, 0, MAX_LITERALS * sizeof (char*));
}

void add_class(glass_env* env, char* name) {
	// adds a class to the env c_lookup
	// if (find_name(env->names, name) >= 0) parse_error("cannot overwrite existing class");
	// the previous line is commented because it always fails - the tokenizer adds all names
	// TODO add a proper check for if a class has already been defined
	// seek the end of class_lookup
	int i = 0;
	while (env->c_lookup[i]) i++;

	env->c_lookup[i] = add_name(env->names, name);
	if (env->c_lookup[i] < 0) parse_error("couldn't add class name");
}

void add_class_func(glass_env* env, char* c_name, char* f_name, int tok_idx) {
	// adds a function to the env f_lookup
	// fills out env->f_locs with tok_idx
	//     for built-in function, supply tok_idx=-1

	int c_name_i = find_name(env->names, c_name);
	// this should never happen
	if (c_name_i < 0) parse_error("no such class name");

	// get the class' index in f_lookup
	int c_idx = 0;
	if (!env->c_lookup[c_idx]) parse_error("no classes in environment");
	while(env->c_lookup[c_idx]) {
		if (env->c_lookup[c_idx] == c_name_i) break;
		c_idx++;
	}
	if (!env->c_lookup[c_idx]) parse_error("couldn't find class name");

	// find the first empty entry in f_lookup
	int empty_i = 0;
	while((env->f_lookup[c_idx][empty_i] > 0) && empty_i < MAX_FUNCS) empty_i++;
	if (empty_i >= MAX_FUNCS) parse_error("MAX_FUNCS exceeded");

	// add the function name, fill out entry
	int f_name_i = add_name(env->names, f_name);
	if (f_name_i < 0) parse_error("could not add function name");
	env->f_lookup[c_idx][empty_i] = f_name_i;
	// fill out the token index
	env->f_locs[c_idx][empty_i] = tok_idx;
}

void init_env(glass_env* env) {
	// allocate subarrays of an env and fill out the standard library
	
	alloc_env(env);

	// build out the standard classes and functions
	char* standard_names[] = {"A", "S", "V", "O", "I", NULL};

	// establish the standard functions (and their order, which is important for the interpreter)
	char* std_A_funcs[] = {"a", "s", "m", "d", "mod", "f", "e", "ne", "lt", "le", "gt", "ge", NULL};
	char* std_S_funcs[] = {"l", "i", "si", "a", "d", "e", "ns", "sn", NULL};
	char* std_V_funcs[] = {"n", "d", NULL};
	char* std_O_funcs[] = {"o", "on", NULL};
	char* std_I_funcs[] = {"l", "c", "e", NULL};

	char** stds[] = {std_A_funcs, std_S_funcs, std_V_funcs, std_O_funcs, std_I_funcs, NULL};

	// add the standard class names and their functions
	for (int i = 0; standard_names[i]; i++) {
		add_class(env, standard_names[i]);
	}
	for (int i = 0; stds[i]; i++) {
		for (int j = 0; stds[i][j]; j++) {
			add_class_func(env, standard_names[i], stds[i][j], -1);
		}
	}
}

glass_env parse_file(char* filename) {
	// reads in a file, returns parsed and tokenized data to the interpreter
	// max numbers of names, classes etc. are fixed for now.
	glass_env res;
	// intialize the name and lookup arrays with the standard classes and functions
	init_env(&res);

	char* file = read_clean(filename);
	char* file_pos = file;

	//check for matching braces, parens, etc.
	check_par(file_pos, '{', '}', '\0');
	check_par(file_pos, '(', ')', '\0');
	check_par(file_pos, '<', '>', '\0');

	// tokenize the program and read function starts into env

	int token_idx = 0;
	token_t cur_token;
	char* cur_class = NULL;
	// initialize various flags for the pass
	int next_is_class_name = 0;
	int next_is_func_name = 0;

	while (*file_pos) {
		// convert the current chunk to a token, add it
		cur_token = make_token(&res, file_pos);
		res.tokens[token_idx] = cur_token;
		token_idx++;
		if (token_idx >= MAX_PROGRAM) parse_error("parse_file: MAX_PROGRAM overrun");

		if (next_is_class_name) {
			next_is_class_name = 0;
			// the current token should be a name, add the class
			if (cur_token.type != NAME_IDX) parse_error("parse_file: { must be followed by name");
			add_class(&res, res.names[cur_token.data]);
			// set the current class
			cur_class = res.names[cur_token.data];
		}
		else if (next_is_func_name) {
			next_is_func_name = 0;
			// the current token should be a name, add function to current class
			if (cur_token.type != NAME_IDX) parse_error("parse_file: { must be followed by name");
			if (!cur_class) parse_error("parse_file: function definition must follow class definition");
			// fill out f_loc to point to the token after this name
			// since token_idx has already been incremented, that's the index to use.
			add_class_func(&res, cur_class, res.names[cur_token.data], token_idx);
		}

		if (*file_pos == '{') next_is_class_name = 1;
		if (*file_pos == '[') next_is_func_name = 1;

		// oh why not
		// this'll throw an error later on if syntax is bad
		if (*file_pos == '}') cur_class = NULL;

		// jump to after the processed chunk
		file_pos = end_of_token(file_pos);
	}

	// terminate with a NO_TOKEN
	res.tokens[token_idx] = (token_t) {NO_TOKEN, 0};

	free(file);
	return res;
}

#endif