
/* compile commands
linux: cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
windows: cc -std=c99 -Wall parsing.c mpc.c -o parsing
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mpc.h"

// If compiling on windows
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}
// Fake add_history function
void add_history(char* unused) {}

// Else, include editline headers
#else
#include <editline/readline.h>
// mac X does not have history file
//#include <editline/history.h>
#endif
// Error condition
#define LASSERT(args, cond, err) \
	if (!(cond)) { \
		lval_del(args); \
		return lval_err(err); \
	}

// lisp value types
enum {
	LVAL_ERR,
	LVAL_NUM,
	LVAL_SYM,
	LVAL_SEXPR,
	LVAL_QEXPR
};
// lisp value error types
enum {
	LERR_DIV_ZERO,
	LERR_BAD_OP,
	LERR_BAD_NUM
};
// lisp value
typedef struct lval {
	int type;
	long num;
	// Error and symbol type string data
	char* err;
	char* sym;
	// a list of lval pointers with its count
	int count;
	struct lval** cell;
} lval;
/*
Constructor for number lval pointer.
Converts long to lval number
*/
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}
/*
Constructor for error lval pointer
Converts int long to lval error
*/
lval* lval_err(char* m) {
	// Alocate memory for lval pointer
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	// Allocate strlen + 1 for null terminator
	v->err = malloc(strlen(m) + 1);
	// Copies the error message string to v->err
	strcpy(v->err, m);
	return v;
}
/*
Constructor for symbol lval pointer
Converts a string symbol to a lval symbol
*/
lval* lval_sym(char* s) {
	// Alocate memory for lval pointer
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	// Copies the string s to v->sym
	strcpy(v->sym, s);
	return v;
}
/*
Constructor for s-expression lval
*/
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
/*
Constructor for q-expression lval
*/
lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
// Free up memory from lval pointer
void lval_del(lval* v) {
	switch (v->type) {
		// Do nothing for numbers
		case LVAL_NUM: break;
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			// Free all elements in s expression
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			// And the container of the pointer
			free(v->cell);
			break;
	}
	free(v);
}

// Adds an lval element to a cell
lval* lval_add(lval* v, lval* x) {
	v->count++;
	// Allocate extra space for new lval.
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}
lval* lval_read_num(mpc_ast_t* tree) {
	lval* v;
	errno = 0;
	long x = strtol(tree->contents, NULL, 10);
	// If not invalid number
	if (errno != ERANGE) {
		v = lval_num(x);
	} else {
		v = lval_err("invalid number");
	}
	return v;
}
lval* lval_read(mpc_ast_t* tree) {
	if (strstr(tree->tag, "number")) { return lval_read_num(tree); }
	if (strstr(tree->tag, "symbol")) { return lval_sym(tree->contents); }
	
	// If root (>) or sexpr then create empty list
	lval* x = NULL;
	if (strcmp(tree->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(tree->tag, "sexpr")) { x = lval_sexpr(); }
	if (strstr(tree->tag, "qexpr")) { x = lval_qexpr(); }
	
	// Fill list with any valid expression contained within
	for (int i = 0; i < tree->children_num; i++) {
		// Don't read (, ), {, }, and regex
		if (strcmp(tree->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, "{") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(tree->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(tree->children[i]));
	}
	return x;
}

lval* lval_eval(lval* v);
lval* lval_take(lval* v, int i);
lval* lval_pop(lval* v, int i);
lval* builtin_op(lval* a, char* operation);

lval* builtin(lval* arg, char* func);

lval* lval_eval_sexpr(lval* v) {
	// Evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}
	// Check for errors
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take (v,i);
		}
	}
	// If expression is empty
	if (v->count == 0) {
		return v;
	}
	// If expression is single
	if (v->count == 1) {
		return lval_take(v,0);
	}
	// Check if first element is Symbol
	lval* firstElement = lval_pop(v, 0);
	if (firstElement->type != LVAL_SYM) {
		// Delete first element and its expression
		lval_del(firstElement);
		lval_del(v);
		return lval_err("s-epression does not start with a symbol!");
	}
	lval* result = builtin(v, firstElement->sym);
	lval_del(firstElement);
	return result;
}

lval* lval_eval(lval* v) {
	lval* result;
	// Evaluate s-expression
	if (v->type == LVAL_SEXPR) {
		result = lval_eval_sexpr(v);
	} else {
		// Evaluate others directly
		result = v;
	}
	return result;
}

// Pops a lval from a s-expression
lval* lval_pop(lval* v, int i) {
	lval* popped_lval = v->cell[i];
	
	// Decrease amount of items in s-expression
	v->count--;
	
	// Shift memory after the item at i over the top
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i));
	
	// Reallocate memory
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	
	return popped_lval;
}
// Takes a lval from a s-expression and deletes the s-expression
lval* lval_take(lval* v, int i) {
	lval* result = lval_pop(v, i);
	lval_del(v);
	return result;
}
// Moves all lvals from y to x
lval* lval_join(lval* x, lval* y) {
	// Each cell in y is added to x
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}
	lval_del(y);
	return x;
}

bool isAddition(char* operatation) {
	return strcmp(operatation, "+") == 0 || strcmp(operatation, "add") == 0;
}
bool isSubtraction(char* operatation) {
	return strcmp(operatation, "-") == 0 || strcmp(operatation, "sub") == 0;
}
bool isMultiplication(char* operatation) {
	return strcmp(operatation, "*") == 0 || strcmp(operatation, "mul") == 0;
}
bool isDivision(char* operatation) {
	return strcmp(operatation, "/") == 0 || strcmp(operatation, "div") == 0;
}

lval* builtin_head(lval* arg) {
	// Error checking conditions
	LASSERT(arg, arg->count == 1,
		"'head' function has too many arguments!");
	LASSERT(arg, arg->cell[0]->type == LVAL_QEXPR,
		"'head' function passed the incorrect type!");
	LASSERT(arg, arg->cell[0]->count != 0,
		"'head' function passed {}!");
	
	// Else, take first arguement / head
	lval* v = lval_take(arg, 0);
	// Delete everything except for head and return
	while (v->count > 1) {
		lval_del(lval_pop(v,1));
	}
	return v;
}
lval* builtin_tail(lval* arg) {
	// Error checking conditions
	LASSERT(arg, arg->count == 1,
		"'tail' function has too many arguments!");
	LASSERT(arg, arg->cell[0]->type == LVAL_QEXPR,
		"'tail' function passed the incorrect type!");
	LASSERT(arg, arg->cell[0]->count != 0,
		"'tail' function passed {}!");
	
	// Else, take first arguement
	lval* v = lval_take(arg, 0);
	// Delete first element and return
	lval_del(lval_pop(v, 0));
	return v;
}
// Converts a q-expression to a s-expression
lval* builtin_list(lval* arg) {
	arg->type = LVAL_QEXPR;
	return arg;
}
// Converts a q-expression to a s-expression and evaluates it
lval* builtin_eval(lval* arg) {
	LASSERT(arg, arg->count == 1,
		"'eval' function has too many arguments!");
	LASSERT(arg, arg->cell[0]->type == LVAL_QEXPR,
		"'eval' function passed the incorrect type!");
	// Take first arguement and convert to s-expression
	lval* x = lval_take(arg, 0);
	x->type = LVAL_SEXPR;
	// Evaluate
	lval* result = lval_eval(x);
	return result;
}

lval* builtin_join(lval* arg) {
	// Check that all arguments are q-expressions
	for (int i = 0; i < arg->count; i++) {
		LASSERT(arg, arg->cell[i]->type == LVAL_QEXPR,
			"'join' function passed the incorrect type!");
	}
	lval* x = lval_pop(arg, 0);
	
	// Do until the arg is empty
	while (arg->count) {
		// Pop the first value from arg and put them into x 
		lval* y = lval_pop(arg, 0);
		x = lval_join(x, y);
	}
	lval_del(arg);
	return x;
	
}
lval* builtin(lval* arg, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(arg); }
	if (strcmp("head", func) == 0) { return builtin_head(arg); }
	if (strcmp("tail", func) == 0) { return builtin_tail(arg); }
	if (strcmp("join", func) == 0) { return builtin_join(arg); }
	if (strcmp("eval", func) == 0) { return builtin_eval(arg); }
	if (strstr("+-/*", func)) { return builtin_op(arg, func); }
	lval_del(arg);
	return lval_err("Unknown function");
}
lval* builtin_op(lval* a, char* operation) {
	// Check if all aruments are numbers
	for (int i = 0; i < a->count; i++) {
		if (a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Can't operate on a non-number");
		}
	}
	// Get the first element
	lval* x = lval_pop(a, 0);
	
	// If there are no other elements and operator is -
	if (a->count == 0 && isSubtraction(operation)) {
		// negate number
		x->num *= -1;
	}
	
	while (a->count > 0) {
		lval* y = lval_pop(a, 0);
		if (isAddition(operation)) { x->num += y->num ; }
		if (isSubtraction(operation)) { x->num  -= y->num ; }
		if (isMultiplication(operation)) { x->num *= y->num; }
		if (isDivision(operation)) { 
			// if denominator is 0
			if (y->num == 0) {
				lval_del(x);
				lval_del(y);
				x = lval_err("Can't divide by zero");
				break;
			} else {
				x->num /= y->num;
			}
		}
		lval_del(y);
	}
	return x;
}


// Forward declarations
void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	//printf("%d", v->count);
	for (int i = 0; i < v->count; i++) {
		// Print value inside
		lval_print(v->cell[i]);
		// Put whitespace for all except for last element
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

// prints value or error of lisp value
void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
	}
}
// print lisp value with newline
void lval_println(lval* v) {
	lval_print(v);
	putchar('\n');
}


int main(int argc, char** argv) {
	// Parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Qexpr = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	const char* language = "                               \
		number   : /-?[0-9]+/;                             \
		symbol : '+' | '-' | '*' | '/' |                   \
		 \"add\" | \"sub\" | \"mul\" | \"div\" |           \
		 \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\";  \
		sexpr : '(' <expr>* ')';                    \
		qexpr : '{' <expr>* '}';                    \
		expr     : <number> | <symbol> | <sexpr> | <qexpr>;  \
		lispy    : /^/ <expr>+ /$/ ;             \
	  ";
	// Define the language
	mpca_lang(MPCA_LANG_DEFAULT, language,
	  Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	puts("Lispy Version 0.0.1");
	puts("Press Ctrl+c to Exit\n");
	
	// Infinite loop until Ctrl+c
	while (1) {
		// Get user input
		char* input = readline("lispee> ");
		// Add command history
		add_history(input);
		
		// Try to parse user input
		mpc_result_t r;
		
		// If parsing sucessful
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
		} else {
			// Else, print error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
	}
	
	// Undefine and delete parsers 
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	
	return 0;
}

