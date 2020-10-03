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
	// copies an [a-Z] name (length < lim-1) in parens into buff
	if (*start == '(') {
		start++;
		for (int i = 0; i != ')'; i++) {
			if (!isalpha(start[i])) parse_error("bad name");
			if (i >= (lim - 2)) parse_error("name too long");
			buff[i] = start[i];
			buff[i + 1] = 0;
		}
	}
	else {
		if (!isalpha(*start)) parse_error("bad name");
		buff[0] = *start;
		buff[1] = 0;
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

named_val build_func(char* start) {
	// given a pointer to the opening [ of a function, parses the function and returns it
	named_val res;
	res.v.type = CMDS;

	// quick pass to check that parens are intact
	check_par(start, '(', ')', ']');
	// also check that <>s match
	check_par(start, '<', '>', ']');

	assert(*start = '[');
	start++;

	res.name = (char*) malloc(64);
	read_name(res.name, start, 64);

	// Move start to after the name
	if (*start == '(') while(!(*(start++) == ')'));
	else start++;

	int lim = 1000;
	res.v.cmds = (char*) malloc(lim);
	check_ptr(res.v.cmds);
	for (int i = 0; start[i] != ']'; i++) {
		if (i >= (lim - 3)) {
			lim += 1000;
			res.v.cmds = (char*) realloc(res.v.cmds, lim);
			check_ptr(res.v.cmds);
		}
		res.v.cmds[i] = start[i];
		res.v.cmds[i+1] = 0;
	}

	return res;
}

class build_class(char* start) {
	// given a pointer to the opening { of a class, parses the class and returns it
	class res;
	res.fncs = NULL;
	res.vars = NULL;
	res.init_func.type = NONE;
	res.dest_func.type = NONE;
	int n_funcs = 0;

	// quick pass to check that brackets are intact
	assert(*start == '{');
	check_par(start, '[', ']', '}');
	start++;

	res.name = (char*) malloc(64);
	read_name(res.name, start, 64);

	// Move start to after the name
	if (*start == '(') while(!(*(start++) == ')'));
	else start++;

	named_val temp;
	for (int i = 0; start[i] != '}'; i++) {
		if (start[i] == '[') {
			temp = build_func(start + i);

			if (!strcmp(temp.name, "c__"))      res.init_func = temp.v;
			else if (!strcmp(temp.name, "d__")) res.dest_func = temp.v;
			else {
				n_funcs++;
				res.fncs = (named_val*) realloc(res.fncs, (n_funcs + 1) * sizeof (named_val));
				check_ptr(res.fncs);
				res.fncs[n_funcs - 1] = temp;
			}
		}
	}

	res.fncs[n_funcs].v.type = NONE;

	return res;
}

class* parse_file(char* filename) {
	// reads in a file, returns an array of the defined classes
	class* res = NULL;
	int n_classes = 0;

	char* file = read_clean(filename);
	char* start = file;

	//check for matching braces
	check_par(start, '{', '}', '\0');
	start--; //TODO: hack
	while(*(++start)) {
		if (*start == '{') {
			n_classes++;
			res = (class*) realloc(res, (n_classes + 1) * sizeof (class));
			check_ptr(res);
			res[n_classes - 1] = build_class(start);
		}
	}

	res[n_classes].name = NULL;

	free(file);

	return res;
}

#endif