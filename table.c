#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// grow when is array 75% full
#define TABLE_MAX_LOAD 0.75

// constructor for new empty hash table
void initTable(Table *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

// deallocate items
void freeTable(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

// return a location for the key
static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;)
    {
        // todo: can we use pointer arithemetic?
        Entry *entry = &entries[index];

        // todo: refactor
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                if (tombstone == NULL)
                {
                    tombstone = entry;
                }
            }
        }
        // todo: fix string comparison
        else if (entry->key == key)
        {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

// grows the table
static void adjustCapacity(Table *table, int capacity)
{
    // make new space
    Entry *entries = ALLOCATE(Entry, capacity);

    // init items
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // copy every existing item
    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];

        if (entry->key == NULL)
        {
            continue;
        }

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;

        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

// return if item exists, set value pointer to this value
bool tableGet(Table *table, ObjString *key, Value *value)
{
    // empty table
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);

    // item not found
    if (entry->key == NULL)
        return false;

    // set and return entry
    *value = entry->value;
    return true;
}

// returns true if value already exists, always caches the value
bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        adjustCapacity(table, GROW_CAPACITY(table->capacity));
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value))
    {
        table->count++;
    }

    entry->key = key;
    entry->value = value;

    return isNewKey;
}

// use tombstoning to override items
bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);

    if (entry->key == NULL)
        return false;

    // mark value as skippable
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

// copies table values
void tableAddAll(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry *entry = &from->entries[i];

        if (entry->key != NULL)
        {
            tableSet(to, entry->key, entry->value);
        }
    }
}

// look for string in table
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash % table->capacity;

    for (;;)
    {
        Entry *entry = &table->entries[index];

        // todo: refactor
        if (entry->key == NULL)
        {
            // tombstone
            if (IS_NIL(entry->value))
                return NULL;
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            // found string
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}