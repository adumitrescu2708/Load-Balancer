/* Copyright 2021 Dumitrescu Alexandra */
// NUME: Dumitrescu Alexandra
// GRUPA: 313CA
// TEMA2: Load Balancer

#ifndef SERVER_H_
#define SERVER_H_

#include "LinkedList.h"

typedef struct server_memory server_memory;

/**
 * Fiecare server va retine un hashtable unde sunt introduse
 * obiecte si cheile lor (vezi: struct info). In plus, adaugam un identificator
 * (un ID) pentru fiecare server.
 *
 * Hashtable-ul are bucket-urile implementate prin liste simplu inlantuite,
 * un camp hmax, menit sa retina numarul maxim de bucketuri pe care le poate
 * avea, un camp size care memoreaza numarul de bucketuri si pointeri la 2
 * functii. (vezi si: load_balancer.c)
 * 
**/

struct info {
	void *key;
	void *value;
};

struct server_memory {
	int ID;
	linked_list_t **buckets;
	int hmax;
	unsigned int size;
	unsigned int (*hash_function)(void*);
	int (*compare_function)(char*, char*);
};

server_memory* init_server_memory();

void free_server_memory(server_memory* server);

/**
 * server_store() - Stores a key-value pair to the server.
 * @arg1: Server which performs the task.
 * @arg2: Key represented as a string.
 * @arg3: Value represented as a string.
 */
void server_store(server_memory* server, char* key, char* value);

/**
 * server_remove() - Removes a key-pair value from the server.
 * @arg1: Server which performs the task.
 * @arg2: Key represented as a string.
 */
void server_remove(server_memory* server, char* key);

/**
 * server_remove() - Gets the value associated with the key.
 * @arg1: Server which performs the task.
 * @arg2: Key represented as a string.
 *
 * Return: String value associated with the key
 *         or NULL (in case the key does not exist).
 */
char* server_retrieve(server_memory* server, char* key);

#endif  // SERVER_H_
