#ifndef GLASS_DEFS_H
#define GLASS_DEFS_H

// instance type labels

enum val_type {NONE, FUNC, NUMB, NAME, STNG, CMDS};
enum class_type {NOTCLASS, BUILTIN, USERDEF};

typedef struct val val;
typedef struct named_val named_val;
typedef struct class class;
typedef struct instance instance;
typedef struct v_list v_list;

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

struct named_val {
	char* name;
	val   v;
};


struct class {
	char*      name;
	val        init_func;
	val        dest_func;
	named_val* fncs;

	named_val* vars;
};

struct instance {
	enum class_type type;
	class*     clss;
	named_val* vars;
};

// data structure for the stack
struct v_list {
	int        last_i;
	size_t     alloc;
	val*       vs;
};

#endif