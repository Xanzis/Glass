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
		for (int i = 0; i != ')'; i++) {
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
	strpcy(res, tmp);
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

cmd* build_func(char* start, int len, char** all_names, int* name_idx) {
	// given a pointer to the opening [ of a function, parses the function and returns it
	// returns as an array of commands
	cmd* res;

	// quick pass to check that parens are intact
	check_par(start, '(', ')', ']');
	// also check that <>s match
	check_par(start, '<', '>', ']');
	// also check there's an even number of ""s
	int qcount = 0;
	for (int i = 0; start[i] != ']'; i++) {
		if (start[i] == '"') qcount++;
	}
	if (qcount % 2) parse_error("mismatched quotes in function");

	assert(*start = '[');
	start++;

	// read the function name, set name_idx accordingly
	char tmp[64] = {0};
	read_name(tmp, start, 64);
	*name_idx = find_name(all_names, tmp);
	if (name_idx < 0) name_idx = add_name(all_names, tmp);

	// Move start to after the name
	if (*start == '(') while(!(*(start++) == ')'));
	else start++;

	// read in to res
	// adds names to the name list as it goes
	// TODO 1000 command limit per function for now
	int lim = 1000;
	res = (cmd*) malloc(lim * sizeof (cmd));
	check_ptr(res);
	int i = 0;
	int cmd_idx = 0;
	char tmp_name[64];
	while(start[i] != ']') {
		if (isalpha(start[i])) {
			read_name(tmp_name, start + i, 64);
			int name_idx = find_name(all_names, tmp_name);
			if (name_idx == -1) name_idx = add_name(all_names, tmp_name);
			// add a new name-type command with the name index
			res[cmd_idx].type = NAMECMD;
			res[cmd_idx].name = name_idx;

			i++;
		}
		else if (start[i] == '(') {
			if (isdigit(start[i])) {
				// this is a command to pull a stack element up
				res[cmd_idx].type = STACK;
				res[cmd_idx].numb = read_number(start + i);
				while (start[i++] != ')'); // this should move to after the )
			}
			else {
				// this is a parenthesized name - read and process
				read_name(tmp_name, start + i, 64);
				int name_idx = find_name(all_names, tmp_name);
				if (name_idx == -1) name_idx = add_name(all_names, tmp_name);
				// add a new name-type command with the name index
				res[cmd_idx].type = NAME;
				res[cmd_idx].name = name_idx;

				// skip to after the end of the name (including parens)
				i += 2 + strlen(tmp_name);
			}
		}
		else if (start[i] == '<') {
			res[cmd_idx].type = NUMBER;
			res[cmd_idx].numb = read_number(start + i);
			while (start[i++] != ')'); // this should move to after the )
		}
		else if (start[i] == '"') {
			res[cmd_idx].type = STRING;
			res[cmd_idx].stng = read_string(start + i);
			i++;
			while(start[i++] != '"'); // this should move to after the '"'
		}
		else {
			res[cmd_idx].type = NOTNAME;
			res[cmd_idx].c = start[i];
			i++;
		}
		cmd_idx++;
	}

	res[cmd_idx].type = NOTCMD;

	return res;
}

glass_class build_class(char* start, int len, char** all_names, int* name_idx) {
	// given a pointer to the opening { of a class, parses the class and returns it
	// modifies name_idx to be the name's index in all_names
	// adds to the end of all_names if necessary (plenty of space already allocated)

	class res;
	res.fncs = NULL;
	res.fnc_idxs = (int*) malloc(len * sizeof (int));
	int n_funcs = 0;

	// quick pass to check that brackets are intact
	assert(*start == '{');
	check_par(start, '[', ']', '}');
	start++;

	// find the class name, modify name_idx, add to all_names if needed
	// SIDE EFFECT! name_idx is a pointer
	char tmp[64] = {0};
	read_name(tmp, start, 64);
	*name_idx = find_name(all_names, tmp).idx;
	if (name_idx < 0) name_idx = add_name(all_names, tmp).idx;

	// Move start to after the name
	if (*start == '(') while(!(*(start++) == ')'));
	else start++;

	val temp;
	for (int i = 0; start[i] != '}'; i++) {
		if (start[i] == '[') {
			n_funcs++;
			// build the function and fill out the name indices
			int fnc_name_idx
			temp = build_func(start + i, len, all_names, &fnc_name_idx);
			res.fnc_idxs[fnc_name_idx] = n_funcs - 1;

			// allocate space for the new function and insert it
			res.fncs = (val*) realloc(res.fncs, (n_funcs + 1) * sizeof (named_val));
			check_ptr(res.fncs);
			res.fncs[n_funcs - 1] = temp;
		}
	}

	res.fncs[n_funcs].v.type = NONE;

	return res;
}

glass_env parse_file(char* filename) {
	// reads in a file, returns a struct containing:
	// number of names, number of classes
	// array of classes, array of names
	// array of class indices for each name (most -1)
	glass_env env = NULL;
	env.classes = NULL
	env.n_classes = 0;
	env.n_names = 0;

	char* file = read_clean(filename);
	char* start = file;
	int len = strlen(file);

	// set up the name array and the first few names
	// there are strictly fewer names than characters in the source
	env.names = (char**) malloc(len * sizeof (char*));
	env.class_idxs = (int*) malloc(len * sizeof (int));
	for (int i = 0; i < len; i++) env.class_idxs[i] = -1;
	env.names[0] = "c__";
	env.names[1] = "d__";
	env.names[2] = "M";
	env.names[3] = "m";
	env.n_names = 4;

	//check for matching braces
	check_par(start, '{', '}', '\0');
	start--; //TODO: this is bad? the pointer's incremented before it's dereferenced...
	while(*(++start)) {
		if (*start == '{') {
			env.n_classes++;
			// add the 
			env.classes = (class*) realloc(env.classes, (env.n_classes + 1) * sizeof (class));
			check_ptr(env.classes);
			// add the class to the env class array and retrieve the class' name index
			// build_class and its subfunctions update env.names with new names
			int name_idx;
			env.classes[n_classes - 1] = build_class(start, len, env.names, &name_idx);
			env.class_idxs[name_idx] = n_classes - 1;
		}
	}

	free(file);

	return env;
}

#endif