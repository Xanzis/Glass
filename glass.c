#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "glassdefs.h"
#include "parser.h"

void glass_error(char* err_text) {
	fprintf(stderr, "Error in glass.c: %s\n", err_text);
	exit(0);
}


int main(int argc, char *argv[] ) {
	if (argc != 2) glass_error("glass takes exactly two arguments");

	class* class_defs = parse_file(argv[1]);

	printf("First class name is: %s\n", class_defs[0].name);
	printf("First class' first function name is: %s\n", class_defs[0].fncs[0].name);
	printf("That function's content is: %s\n", class_defs[0].fncs[0].v.cmds);
	
	
	
	return 1;
}