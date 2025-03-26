#ifndef __TABLE_H__
#define __TABLE_H__
#include <pthread.h>
#include "proj2.h"

typedef struct table {
    char key[31];
    int  status;
    pthread_mutex_t table_mutex;
} table_t;

void init_table(table_t* table);
int  find_empty(table_t* table);
int  find_key(table_t* table, char* key);
void write_to_file(char* buffer,  int index);
int  read_from_file(char* buffer, char* buffer_out, int index);
void delete_key(table_t* table, int index);
int add_key(struct request* rq, table_t* table, char* buf);

#endif