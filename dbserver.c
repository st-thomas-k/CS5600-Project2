#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#include "proj2.h"
#include "table.h"
#include "queue.h"

#define NUM_WORKERS 4
#define MAX_ENTRIES 200

table_t table[MAX_ENTRIES];
queue_t queue;

_Atomic int running = 1;
// 0: writes, 1: reads, 2: deletes, 3: object count, 4: fails, 5: requests
//_Atomic int counts[6];
int counts[6];

void print_stats() {
    printf("\n=== Server Statistics ===\n");
    printf("\nWrites: %d\nReads: %d\nDeletes: %d\nObject Count: %d\nFails: %d\nRequests: %d\n\n",
         counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
    printf("=========================\n\n");
}

void handle_keyboard() {
	 char line[128];

     while (1) {
     	 fgets(line, sizeof(line), stdin);

         line[strcspn(line, "\n")] = '\0';

         if (strcmp(line, "quit") == 0) {
             atomic_store(&running, 0);
             pthread_cond_broadcast(&queue.cond);
             printf("\n=== Closing Server ===\n");
         	 break;
         }
         else if (strcmp(line, "stats") == 0) {
             print_stats();
         }
     }
}

void handle_work(int fd) {
    atomic_fetch_add(&counts[5], 1);
    int index, length;
    struct request rq, response;
    read(fd, &rq, sizeof(rq));

    memset(&response, 0, sizeof(response));

    switch (rq.op_status) {
        case 'W':
            char buf[4096];
            memset(buf, 0, sizeof(buf));

            length = atoi(rq.len);
            read(fd, &buf, length * sizeof(char));

            strcpy(response.name, rq.name);
            sprintf(response.len, "%d", 0);

            if (atomic_load(&counts[3]) < MAX_ENTRIES) {
                add_key(&rq, table, buf);                  // lock in function
                response.op_status = 'K';
                counts[0]++;
                counts[3]++;
                //atomic_fetch_add(&counts[0], 1);
                //atomic_fetch_add(&counts[3], 1);
            }
            else {
                response.op_status = 'X';
                counts[4]++;
                //atomic_fetch_add(&counts[4], 1);
            }

            write(fd, &response, sizeof(response));
            break;

        case 'R':
            char output[4096];
            pthread_mutex_lock(&table->table_mutex);
            memset(output, 0, sizeof(output));
            index = find_key(table, rq.name);

            if (index != -1) {

                length = read_from_file(rq.name, output, index);
                response.op_status = 'K';
                strcpy(response.name, rq.name);
                sprintf(response.len, "%d", length);

                write(fd, &response, sizeof(response));
                write(fd, &output, length * sizeof(char));
                counts[1]++;
                //atomic_fetch_add(&counts[1], 1);
            }
            else {
                response.op_status = 'X';
                strcpy(response.name, rq.name);
                sprintf(response.len, "%d", 0);
                write(fd, &response, sizeof(response));
                counts[4]++;
                //atomic_fetch_add(&counts[4], 1);
            }
            pthread_mutex_unlock(&table->table_mutex);
            break;

        case 'D':
            pthread_mutex_lock(&table->table_mutex);
            index = find_key(table, rq.name);
            if (index != -1) {
                delete_key(table, index);
                response.op_status = 'K';
            	strcpy(response.name, rq.name);
            	sprintf(response.len, "%d", 0);
            	write(fd, &response, sizeof(response));
                counts[2]++;
                counts[3]--;
                //atomic_fetch_add(&counts[2], 1);
                //atomic_fetch_sub(&counts[3], 1);
            }
            else {
             	response.op_status = 'X';
            	strcpy(response.name, rq.name);
            	sprintf(response.len, "%d", 0);
            	write(fd, &response, sizeof(response));
                counts[4]++;
                //atomic_fetch_add(&counts[4], 1);
            }
            pthread_mutex_unlock(&table->table_mutex);
            break;
        default:
            break;
    }
    close(fd);


}

void* listener(void* arg) {
    int fd;
    int port = *(int*)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    fcntl(sock, F_SETFL, O_NONBLOCK);

    // to avoid waiting while testing
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = 0;

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("cannot bind");
        close(sock);
        exit(1);
    }

    if (listen(sock, 5) < 0) {
        perror("listen failed");
        exit(1);
    }
    printf("Listening on port %d. . .\n", port);
    printf("Database server started. Type 'stats' for statistics or 'quit' to exit.\n\n");

    while (running) {
        fd = accept(sock, NULL, NULL);

        if (fd < 0) {
            // make sure listener thread isn't blocking other threads
          	if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
          	}
            perror("accept failed");
            continue;
        }

        enqueue(&queue, fd);
    }

    close(sock);
    return NULL;
}


int get_work(queue_t *q) {
    pthread_mutex_lock(&q->mutex);

    // thread idles if there is no work in the queue
    while (q->head == NULL) {
        if (!running) {                          // if main thread stops running
            pthread_mutex_unlock(&q->mutex);
            return -1;
        }
        pthread_cond_wait(&q->cond, &q->mutex);  // signal that thread is waiting
    }

    work_t* tmp = q->head;
    int fd = tmp->fd;
    q->head = tmp->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }

    free(tmp);
    pthread_mutex_unlock(&(q->mutex));

    return fd;
}


void* worker(void* arg) {
    while (running) {
        int fd = get_work(&queue);
        if (fd == -1) {
            break;  // kill thread if there is no work
        }

        handle_work(fd);
        close(fd);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    int port = 5000;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

  	pthread_t listener_thread;
    pthread_t worker_thread[NUM_WORKERS];

    init_table(table);
    init_queue(&queue);
    memset(counts, 0, sizeof(counts));
	system("rm -f tmp/data.*.txt");

    // listener thread
    if (pthread_create(&listener_thread, NULL, listener, &port) != 0) {
      perror("could not create listener thread");
      return 1;
    }

    // four worker threads
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pthread_create(&worker_thread[i], NULL, worker, NULL) != 0) {
            perror("could not create worker thread");
            return 1;
        }
    }

    // main thread
	handle_keyboard();

    pthread_join(listener_thread, NULL);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(worker_thread[i], NULL);
    }

    return 0;
}