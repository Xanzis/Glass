#ifndef GLASS_DEFS_H
#define GLASS_DEFS_H

// instance type labels

enum val_type {NONE, FUNC, NUMB, NAME, STNG, OBJT};
enum cmd_type {NOTCMD, NAMECMD, NOTNAME, STRING, NUMBER, STACK}
enum class_type {NO_CLASS, BUILTIN, USERDEF};
enum namespace {NOSCOPE, GLOBAL, INSTANCE, FUNCTION};

typedef struct val val;
typedef struct glass_class glass_class;
typedef struct instance instance;
typedef struct v_list v_list;
typedef struct glass_name glass_name;
typedef struct glass_env glass_env;
typedef struct cmd cmd;

int find_name(char** all_n, char* n);
int add_name(char** all_n, char* n);

struct val {
	enum val_type type;

	union {
		int   numb;
		val*  func;
		glass_name name;
		char* stng;

		// objects are vals with an instance pointer
		instance* objt;
	};
};

// data structure for a single command in a function
struct cmd {
	enum cmd_type type;

	union {
		char       c;
		int        numb;
		char*      stng;
		glass_name name;
	}
}

struct glass_class {
	val        init_func;
	val        dest_func;
	val*       fncs;

	// namespace mapping
	int* fnc_idxs
};

struct instance {
	enum class_type type;
	glass_class*    clss;
	val* vars;
};

// data structure for the stack
struct v_list {
	int        last_i;
	size_t     alloc;
	val*       vs;
};

struct glass_name {
	enum namespace space;
	int idx;
}

struct glass_env {
	int n_names;
	int n_classes;
	glass_class* classes;

	// namespace mapping
	char** names;
	int* class_idxs;
}

enum namespace find_scope(char c) {
	// determines the scope of a name (from glass spec)
	if (isupper(c)) {
		return GLOBAL;
	}
	if (islower(c)) {
		return INSTANCE;
	}
	if (c == '_') {
		return FUNCTION;
	}
	return NOSCOPE;
}

glass_name find_name(char** all_n, char* n) {
	// Finds name n (null-terminated) in all_n
	// infers namespace from first character of n
	for (int i = 0; all_n[i]; i++) {
		if (!strcmp(all_n[i], n)) return i;
	}
	enum namespace space = find_scope(n[0]);
	return (glass_name) {space, -1};
}

glass_name add_name(char** all_n, char* n) {
	// adds the name, returns the place it was added
	// infers namespace from first character of n
	int i = 0;
	do {
		if (!all_n[i]) {
			all_n[i] = n;
			return i;
		}
		i++
	} while (all_n[i])
	enum namespace space = find_scope(n[0]);
	return (glass_name) {space, -1};
}

#endif