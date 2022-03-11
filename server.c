/* Copyright 2021 Dumitrescu Alexandra */
// NUME: Dumitrescu Alexandra
// GRUPA: 313CA
// TEMA2: Load Balancer

#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "utils.h"
#include "LinkedList.h"
#define MAX_BUCKETS 350

/**
 * Vezi si: server.h
 *
 * init_server_memory():
 * Functia returneaza un pointer la un nou server caruia i-au
 * fost alocate si initializate campurile. Se verifica succesul
 * functiilor prin DIE().
**/

server_memory* init_server_memory()
{
	server_memory *server = malloc(sizeof(server_memory));
	DIE(server == NULL, "Malloc Failed");

	server->size = 0;
	server->hmax = MAX_BUCKETS;
	server->ID = 0;

	server->buckets = malloc(MAX_BUCKETS * sizeof(linked_list_t *));
	for (int i = 0; i < MAX_BUCKETS; i++)
		server->buckets[i] = ll_create(sizeof(struct info));

	return server;
}


/**
 * server_store():
 * Se introduce o noua pereche cheie-valoare in hashtableul asociat
 * serverului trimis ca parametru.
 *
 * In pasul (1) se verifica sa nu existe deja cheia pe care o cautam.
 * In caz afirmativ, se face update la valoarea asociata.
 *
 * In pasul (2) ne folosim de o noua variabila de tip struct info
 * pentru care sunt cunoscute cheia si valoare (primite ca parametrii)
 *
 * In pasul (3) se adauga la inceputul bucketului perechea cheie-valoare
 *
**/

void server_store(server_memory* server, char* key, char* value)
{
	if (server == NULL)
		return;

	unsigned int value_size = strlen(value) + 1;
	unsigned int key_size = strlen(key) + 1;

	unsigned int hash_index = server->hash_function(key) % server->hmax;
	linked_list_t *hash_bucket = (server->buckets)[hash_index];
	ll_node_t *current_node = hash_bucket->head;

	// (1)
	while (current_node != NULL) {
		struct info *data = ((struct info *)current_node->data);
		if (server->compare_function(data->key, key) == 0) {
			memcpy(data->value, value, value_size * sizeof(char));
			return;
		}
		current_node = current_node->next;
	}

	// (2)
	struct info new_data;
	new_data.key = malloc(key_size * sizeof(char));
	DIE(new_data.key == NULL, "Malloc Failed");
	new_data.value = malloc(value_size * sizeof(char));
	DIE(new_data.value == NULL, "Malloc Failed");

	memcpy(new_data.key, key, key_size * sizeof(char));
	memcpy(new_data.value, value, value_size * sizeof(char));

	// (3)
	ll_add_nth_node(hash_bucket, 0, &new_data);

	server->size++;
}

/**
 * server_remove()
 * Functia sterge intrarile din hashtable asociate cheii trimise ca parametru.
 *
 * In pasul (1) se afla bucketul in care se afla cheia, se parcurge lista,
 * iar de fiecare data cand functia compare are succes se iese din pas.
 *		- parametrul count - retine pozitia din lista pe care se afla
 *							 nodul pe care vrem sa il eliminam
 *
 * In pasul (2) eliminam din lista gasita anterior nodul de pe pozitia count
 * si eliberam memoria ocupata.
**/

void server_remove(server_memory* server, char* key)
{
	if (server == NULL)
		return;

	// (1)
	unsigned int hash_index = server->hash_function(key) % server->hmax;
	linked_list_t *hash_bucket = (server->buckets)[hash_index];
	ll_node_t *current_node = hash_bucket->head;
	int count = 0;

	while (current_node != NULL) {
		struct info *data = (struct info *)current_node->data;
		if (server->compare_function(data->key, key) == 0)
			break;
		current_node = current_node->next;
		count++;
	}

	// (2)
	server->size--;
	current_node = ll_remove_nth_node(hash_bucket, count);

	free(((struct info *)current_node->data)->key);
	free(((struct info *)current_node->data)->value);
	free(current_node->data);
	free(current_node);
}

/**
 * server_retrieve()
 * Functia returneaza valoarea asociata cheii trimise ca parametru.
 *
 * Se gaseste bucketul asociat cheii.
 * Se parcurge lista, iar atunci cand functia de compare are succes
 * se returneaza valoarea gasita.
 *
**/

char* server_retrieve(server_memory* server, char* key)
{
	if (server == NULL)
		return NULL;

	unsigned int hash_index = server->hash_function(key) % server->hmax;
	linked_list_t *hash_bucket = (server->buckets)[hash_index];
	ll_node_t *current_node = hash_bucket->head;

	while (current_node != NULL) {
		struct info *data = (struct info *)current_node->data;
		if (strcmp((char *)data->key, key) == 0)
			return (char *)data->value;
		current_node = current_node->next;
	}
	return NULL;
}

/**
 * free_server_memory()
 * Functia elibereaza memoria ocupata de serverul trimis ca parametru.
 *
 * Se parcurge fiecare bucket din server si se apeleaza server_remove pentru
 * fiecare nod descoperit. In final, se elibereaza bucketurile si serverul.
**/

void free_server_memory(server_memory* server)
{
	int i;
	ll_node_t *current_node, *next_node;
	linked_list_t *hash_bucket;
	if (server == NULL)
		return;

	for (i = 0; i < server->hmax; i++) {
		hash_bucket = (server->buckets)[i];

		if (server->buckets[i] != NULL) {
			current_node = hash_bucket->head;
			while (current_node != NULL) {
				next_node = current_node->next;
				server_remove(server, ((struct info *)current_node->data)->key);
				current_node = next_node;
			}
		}
		ll_free(&hash_bucket);
	}
	free(server->buckets);
	free(server);
}
