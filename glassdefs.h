#ifndef GLASS_DEFS_H
#define GLASS_DEFS_H

// val type labels
#define NONE 0
#define FUNC 1
#define NUMB 2
#define NAME 3
#define STNG 4
#define CMDS 5

// instance type labels

typedef struct val val;
struct val {
	int type;

	union {
		int   numb;
		val*  func;
		char* name;
		char* stng;

		// functions are vals with command string
		char* cmds;
	};
};

typedef struct named_val named_val;
struct named_val {
	char* name;
	val   v;
};

typedef struct class class;
struct class {
	char*      name;
	val        init_func;
	val        dest_func;
	named_val* fncs;

	named_val* vars;
};

typedef struct instance instance;
struct instance {
	class*     clss;
	named_val* vars;
};

typedef struct func_instance func_instance;
struct func_instance {
	named_val*     vars;
	func_instance* child;
};

#endif