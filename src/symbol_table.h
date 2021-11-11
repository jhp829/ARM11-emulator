#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE
#include "utils.h"
#include <stdbool.h>

typedef struct node {
  char *key;
  WORD value;
  struct node *next;
} node;

typedef struct {
  node *start;
} table;

typedef WORD (*word_func) ();

typedef struct fnode {
  char *key;
  word_func func;
  struct fnode *next;
} fnode;

typedef struct {
  fnode *start;
} ftable;

// Creates a new heap-allocated symbol table
table* table_create(void);

// Frees the table recursively at the given pointer
void table_free(table *);

// Inserts a key-value pair into the table
void table_insert(table *, const char *key, WORD value);

// If the key exists in the table, store the corresponding value
// at the given pointer, and return true. Otherwise, return false.
bool table_get(table *, const char *key, WORD *value);

// Creates a new heap-allocated symbol table
ftable* ftable_create(void);

// Frees the table recursively at the given pointer
void ftable_free(ftable *);

// Inserts a key-value pair into the table
void ftable_insert(ftable *, const char *key, word_func func);

// If the key exists in the table, store the corresponding value
// at the given pointer, and return true. Otherwise, return false.
bool ftable_get(ftable *, const char *key, word_func *func);

#endif
