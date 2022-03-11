/* Copyright 2021 Dumitrescu Alexandra */
/* NUME: Dumitrescu Alexandra
   GRUPA: 313CA
   TEMA2: Load Balancer */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LinkedList.h"
#include "utils.h"

/**
 * ll_create()
 * Returneaza un pointer la o lista noua, avand campurile alocate si
 * initializate. Se verifica succesul operatiilor prin DIE().
**/

linked_list_t *ll_create(unsigned int data_size) {
    linked_list_t *list = malloc(sizeof(linked_list_t));
    DIE(list == NULL, "Malloc Failed");
    list->size = 0;
    list->head = NULL;
    list->data_size = data_size;
    return list;
}


/**
 * ll_add_nth_node()
 *  Adauga un nou nod in lista pe pozitia n.
 *  Daca lista nu exista -> eroare.
 *  Daca n este mai mare decat dimensiunea listei -> se va adauga nodul la
 * finalul listei.
 *  Aloca memorie pentru un nou nod si copiaza datele primite ca parametru.
 *  Se trateaza 3 cazuri: (1): nodul este primul adaugat si izolat
 *                        (2): nodul este primul adaugat si mai urmeaza un nod
 *                        (3): nodul este adaugat la final, sau in interior
**/

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	ll_node_t *current_node;
	ll_node_t *new_node;
	unsigned int i;

	if (list == NULL)
		return;

	if (n > list->size)
		n = list->size;

    current_node = list->head;
    new_node = malloc(sizeof(ll_node_t));
    DIE(new_node == NULL, "Malloc Failed");
    new_node->data = malloc(list->data_size);
    DIE(new_node->data == NULL, "Malloc Failed");

	if (n == 0) {
		memcpy(new_node->data, new_data, list->data_size);
		if (list->size == 0)  // (1)
			new_node->next = NULL;
		else
			new_node->next = list->head;  // (2)
		list->head = new_node;
	} else {
        // (3)
		for (i = 1; i < n; i++)
			current_node = current_node->next;
		memcpy(new_node->data, new_data, list->data_size);
		new_node->next = current_node->next;
		current_node->next = new_node;
	}
	list->size++;
	return;
}

/**
 * ll_remove_nth_node()
 * Se elimina nodul aflat pe pozitia n in lista primita ca parametru.
 * Daca lista nu exista -> eroare.
 * Daca n este mai mare decat size-ul listei, se va elimina ultimul nod.
 *
**/

ll_node_t* ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *current_node = list->head;
    ll_node_t *remove;

	unsigned int i;
    if (list == NULL)
    	return NULL;
    if (n >= list->size)
    	n = list->size - 1;

    if (n == 0) {
    	list->head = current_node->next;
    	list->size--;
    	return current_node;
    } else {
    	for (i = 1; i < n; i++) {
    		current_node = current_node->next;
    	}
    	remove = current_node->next;
    	current_node->next = current_node->next->next;
    	list->size--;
    	return remove;
    }
}


/**
 * ll_free()
 * Parcurge lista si elibereaza memoria ocupata
 * de fiecare nod si in final de lista.
**/

void ll_free(linked_list_t **pp_list)
{
    ll_node_t *current_node = (*pp_list)->head;
    ll_node_t *last_node = (*pp_list)->head;
    while (current_node != NULL) {
    	current_node = last_node->next;
    	free(last_node);
    	last_node = current_node;
    }
    free(*pp_list);
}
