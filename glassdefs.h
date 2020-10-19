#ifndef GLASS_DEFS_H
#define GLASS_DEFS_H

// instance type labels

enum val_type {NONE, FUNC, NUMB, NAME, STNG, CMDS};
enum token_type {NO_TOKEN, ASCII, NAME_IDX, NUMBER, STNG_IDX, STCK_IDX};

typedef struct val val;
typedef struct v_list v_list;
typedef struct glass_env glass_env;
typedef struct tok tok;

int find_name(char** all_n, char* n);
int add_name(char** all_n, char* n);

struct val {
	enum val_type type;

	union {
		int   numb;
		val*  func;
		char* name;
		char* stng;

		// functions are vals with command string
		char* cmds;
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
	char** names;    // all names defined in the program (for debug purposes)
	int* c_lookup;   // c_lookup[i] = n means the ith class has name n (in the name array)
	int** f_lookup;  // f_lookup[c][i] = n means the number ith function of the cth class has name n
	int***  f_locs;  // f_locs[c][f] points to the first token of the fth function of cth class
	token_t* tokens; // array of tokens forming the program. 

	char** strings;  // array of all string literals used in program
};

int find_name(char** all_n, char* n) {
	// finds name n (null-terminated) in all_n
	// returns -1 on failure
	for (int i = 0; all_n[i]; i++) {
		if (!strcmp(all_n[i], n)) return i;
	}
	return -1;
}

int add_name(char** all_n, char* n) {
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
			all_n[i] = (char*) malloc(strlen(n) * sizeof (char));
			strcpy(all_n[i], n);
			return i;
		}
		i++
	} while (i < MAX_NAMES)
	return -1;
}

#endif