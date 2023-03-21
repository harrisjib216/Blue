#ifndef blue_table_h
#define blue_table_h

#include "common.h"
#include "value.h"

typedef struct
{
    ObjString *key;
    Value value;
} Entry;

typedef struct
{
    // number of key/value pairs
    int count;

    // allocated size, count & capacity = load factor
    int capacity;

    // array of entries
    Entry *entries;
} Table;

// constructor to create a new hash table
void initTable(Table *table);

// return memory
void freeTable(Table *table);

// return if exists, place into value pointer
bool tableGet(Table *table, ObjString *key, Value *value);

// insert into table
bool tableSet(Table *table, ObjString *key, Value value);

// delete items
bool tableDelete(Table *table, ObjString *key);

// copy entries over
void tableAddAll(Table *from, Table *to);

// finds a string
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash);

#endif