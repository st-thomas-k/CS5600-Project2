/**
 * table.c - functions needed to handle table struct
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "table.h"

#define MAX_ENTRIES 200


void init_table(table_t *table) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        table[i].key[0] = '\0';
        table[i].status = 2;
    }

    pthread_mutex_init(&table->table_mutex, NULL);
}


int find_empty(table_t* table) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (table[i].key[0] == '\0') {
            return i;
        }
    }

    return -1;
}


int find_key(table_t* table, char* key) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (table[i].status == 2 && strcmp(table[i].key, key) == 0) {
            pthread_mutex_unlock(&table->table_mutex);
            return i;
        }
    }

    return -1;
}


void write_to_file(char* buffer, int index) {
    char filename[50];
    sprintf(filename, "tmp/data.%d.txt", index);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        perror("cannot open file");
        return;
    }

    write(fd, buffer, strlen(buffer));
    close(fd);
}


int read_from_file(char* buffer, char* buffer_out, int index) {
    char filename[50];
    memset(filename, 0, sizeof(filename));

    sprintf(filename, "tmp/data.%d.txt", index);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("cannot open file\n");
        exit(1);
    }

    int len = read(fd, buffer_out, 4095);
    buffer_out[len] = '\0';
    close(fd);

    return len;
}


void delete_key(table_t* table, int index) {
    char command[50];
    sprintf(command, "rm -f tmp/data.%d.txt", index);
    table[index].status = 1;

    system(command);
    memset(&table[index].key, 0, sizeof(table[index].key));
    table[index].key[0] = '\0';

    table[index].status = 2;
}


int add_key(struct request* rq, table_t* table, char* buf) {
    int key_exists = 0;
    int empty_index;
    pthread_mutex_lock(&table->table_mutex);

    // check if key exists. if not, find first empty space
    int index = find_key(table, rq->name);
    if (index == -1) {
        empty_index = find_empty(table);

        if (empty_index == -1) {
            return -1;
        }
        index = empty_index;
    }
    else {
        // if it exists, replace it
        delete_key(table, index);
        key_exists = 1;
    }

    strcpy(table[index].key, rq->name);
    table[index].status = 1;
    write_to_file(buf, index);
    table[index].status = 2;

    pthread_mutex_unlock(&table->table_mutex);

    return key_exists;
}
