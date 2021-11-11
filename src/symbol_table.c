#include "symbol_table.h"
#include <string.h>
#include <assert.h>

table* table_create(void) {
  table *new_table = malloc(sizeof(table));
  fail_if(!new_table, "Failed to allocate memory");
  new_table->start = NULL;
  return new_table;
}

void table_free(table *t) {
  node *curr = t->start;
  while(curr) {
    node *next = curr->next;
    free(curr->key);
    free(curr);
    curr = next;
  }
  free(t);
}

void table_insert(table *t, const char *key, WORD value) {
  node *prev = NULL;
  node *curr = t->start;
  int bigger = curr? strcmp(key, curr->key): -1;
  while(bigger > 0) {
    prev = curr;
    curr = curr->next;
    bigger = curr? strcmp(key, curr->key): -1;
  }
  if (bigger == 0) {
    curr->value = value;
    return;
  }

  node *new_node = malloc(sizeof(node));
  fail_if(!new_node, "Failed to allocate memory");
  new_node->key = calloc(strlen(key) + 1, sizeof(char));
  fail_if(!new_node->key, "Failed to allocate memory");

  strcpy(new_node->key, key);
  new_node->value = value;
  new_node->next = curr;
  if (prev) {
    prev->next = new_node;
  } else {
    t->start = new_node;
  }
}

bool table_get(table *t, const char *key, WORD *value) {
  node *curr = t->start;
  int bigger = curr? strcmp(key, curr->key): -1;
  while(bigger > 0) {
    curr = curr->next;
    bigger = curr? strcmp(key, curr->key): -1;
  }
  if (bigger == 0) {
    *value = curr->value;
    return true;
  }
  return false;
}

ftable* ftable_create(void) {
  ftable *new_table = malloc(sizeof(ftable));
  fail_if(!new_table, "Failed to allocate memory");
  new_table->start = NULL;
  return new_table;
}

void ftable_free(ftable *t) {
  fnode *curr = t->start;
  while(curr) {
    fnode *next = curr->next;
    free(curr->key);
    free(curr);
    curr = next;
  }
  free(t);
}

void ftable_insert(ftable *t, const char *key, word_func func) {
  fnode *prev = NULL;
  fnode *curr = t->start;

  int bigger = curr? strcmp(key, curr->key): -1;
  while(bigger > 0) {
    prev = curr;
    curr = curr->next;
    bigger = curr? strcmp(key, curr->key): -1;
  }
  if (bigger == 0) {
    curr->func = func;
    return;
  }

  fnode *new_node = malloc(sizeof(node));
  fail_if(!new_node, "Failed to allocate memory");
  new_node->key = calloc(strlen(key) + 1, sizeof(char));
  fail_if(!new_node->key, "Failed to allocate memory");

  strcpy(new_node->key, key);
  new_node->func = func;
  new_node->next = curr;
  if (prev) {
    prev->next = new_node;
  } else {
    t->start = new_node;
  }
}

bool ftable_get(ftable *t, const char *key, word_func *func) {
  fnode *curr = t->start;
  int bigger = curr? strcmp(key, curr->key): -1;
  while(bigger > 0) {
    curr = curr->next;
    bigger = curr? strcmp(key, curr->key): -1;
  }
  if (bigger == 0) {
    *func = curr->func;
    return true;
  }
  return false;
}
