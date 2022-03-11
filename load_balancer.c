/* Copyright 2021 Dumitrescu Alexandra */
/* NUME: Dumitrescu Alexandra
   GRUPA: 313CA
   TEMA2: Load Balancer */

#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "utils.h"

#define MAX_SERVERS 350

/**
 * Load Balancer-ul cuprinde:
 *	-- servers: vector de pointeri la serverele din sistem
 *	-- hashring: vector de etichete, circular si sortat crescator
 *	-- servers_count: retine cate etichete sunt introduse in hashring
 *
 * Pentru fiecare server cu un ID se introduc in hashring 3 etichete
 * astfel:
 *
 * EXEMPLU:												(B)
 *		Date retinute 				Date retinute
 *			in server_memory:			in hashring:
 *
 *
 *					--->			ETICHETA #1: 100000 * 0 + 1
 *		(ID: 1)		--->			ETICHETA #2: 100000 * 1 + 1
 *					--->			ETICHETA #3: 100000 * 2 + 1
 *
 *									Toate cele 3 etichete fac referire
 *									la serverul identificat prin:
 *
 *									ID = ETICHETA #N % 100000
 **/

struct load_balancer {
    int *hashring;
    server_memory **servers;
    int servers_count;
};


/**
 * Functii folosite pentru a calcula hash-uri.
 *
 * Hashtable-ul din fiecare
 * server va avea o functie de calcul a hash-ului -- in cazul nostru
 * hash_function_key, si o functie de compare -- compare_strings
 *
 * Hashtable-ul este generic implementat, putand fi introduse diverse
 * tipuri de date, dar pentru sistemul propus, se cunoaste ca se introduc
 * doar stringuri --> functia de compare este strcmp().
 *
**/

unsigned int hash_function_servers(void *a)
{
    unsigned int uint_a = *((unsigned int *)a);
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}


unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;
    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

int
compare_strings(char *a, char *b)
{
    return strcmp(a, b);
}


/**
 * Functia de CREATE:
 * 		-- Returneaza o structura load_balancer careia i-au fost
 *		alocate si initializate campurile. Se verifica succesul operatiilor
 *		prin functia DIE(), implementata in utils.h
 *
**/

load_balancer* init_load_balancer() {
    load_balancer *loader = malloc(sizeof(load_balancer));
    DIE(loader == NULL, "Malloc Failed");

    loader->hashring = malloc(sizeof(int));
    loader->servers_count = 0;
    loader->servers = malloc(sizeof(server_memory *));
    return loader;
}


/**
 * Functie care cauta server_id in vectorul de servere din main si
 * returneaza pozitia pe care a fost gasit.
**/

void search_server(load_balancer *main, int *server_id, int *position)
{
	if (main == NULL)
		return;

	for (int i = 0; i <= main->servers_count / 3; i++) {
		if (main->servers[i]->ID == *server_id) {
			*position = i;
			return;
		}
	}
}

/**
 * Functii de RESIZE:
 *		resize_hashring(main, n) --> redimensioneaza hashring-ul din main
 *									 intr-un vector de dimensiune n
 *		resize_server_array(main, n) --> redimensioneaza vectorul de servere
 *										 intr-un vector de dimensiune n
 *
 * Ambele functii urmeaza aceeasi pasi:
 *
 * -- Se copiaza intr-un vector auxilar, alocat de dimensinea
 * dorita (n), informatiile din hashring/vector de servere.
 * -- Se elibereaza memoria ocupata de hashring/vector de servere,
 * se realoca de dimensiune n si se re-copiaza informatiile din aux
 * -- Se elibereaza memoria ocupata de aux.
 *
**/

void resize_hashring(load_balancer *main, int n)
{
	if (main == NULL)
		return;

	int *hashring_copy = malloc(n * sizeof(int));
	DIE(hashring_copy == NULL, "Malloc Failed");

	int minim;
	if (n == main->servers_count + 1)
		minim = main->servers_count;
	else
		minim = n;

	for (int i = 0; i < minim; i++)
		hashring_copy[i] = main->hashring[i];

	free(main->hashring);
	main->hashring = malloc(n * sizeof(int));
	DIE(main->hashring == NULL, "Malloc Failed");

	for (int i = 0; i < n; i++)
		main->hashring[i] = hashring_copy[i];

	free(hashring_copy);
}

void resize_server_array(load_balancer *main, int n)
{
	if (main == NULL)
		return;

	server_memory **servers_copy = malloc(n * sizeof(server_memory *));
	DIE(servers_copy == NULL, "Malloc Failed");

	int minim;
	if (n == main->servers_count/3 + 1)
		minim = main->servers_count/3;
	else
		minim = n;

	for (int i = 0; i < minim; i++)
		servers_copy[i] = main->servers[i];

	free(main->servers);
	main->servers = malloc(n * sizeof(server_memory *));
	DIE(main->servers == NULL, "Malloc Failed");

	for (int i = 0; i < n; i++)
		main->servers[i] = servers_copy[i];

	free(servers_copy);
}


/**
 * BINARY_SEARCH
 * Cautarea binara este folosita in 3 contexte:
 *		(1): Pentru cautarea eficienta a unei informatii in hashring
 *		(2): Pentru returnarea indexului unde trebuie adaugata o eticheta
 *			noua in hashring -- vectorul este construit sortat crescator
 *		(3): Pentru returnarea poztiei unde trebuie adaugat un nou obiect
 *			in functia de store
 *	
 *		Exemplu pentru (3) folosit in multe din functiile ce urmeaza:	(A)
 *			main->hashring: 10000 20000 30000 40000
 *			hash(new_object->key) = 22000
 *			binary_search_object (main, key, position)
 *				--> position = 2;
 *				--> ETICHETA serverului unde trebuie adaugat new_object
 *							= main->hashring[position] ( = 30000)
 *				--> ID server-ului unde trebuie adaugat new_object
 *							= ETICHETA % 10000 = 0;
 *				--> prin server_search(main, ID, position) position va retine
 *					index-ul in vectorul de pointeri la servere care duce
 *					la server-ul unde va fi adaugat obiectul 
 *
 * Se disting doua functii: binary_search_object() si binary_search_label()
 * Ambele functioneaza pe acelasi principiu, difera doar functia de hash
 * folosita.
 *
**/

void binary_search_object(load_balancer* main, char *key, int *position)
{
    if (main == NULL)
        return;

    int left =  0, right = main->servers_count - 1, middle = 0;
    unsigned int hash_server = 0, hash_key = 0;

    while (left <= right) {
        middle = (left + right) / 2;

        hash_server = hash_function_servers(&(main->hashring[middle]));
        hash_key = hash_function_key(key);

        if (hash_server > hash_key)
            right = middle - 1;
        else
            left = middle + 1;
    }
    *position = left;
}

void binary_search(load_balancer* main, int server_id, int *position)
{
    if (main == NULL)
        return;

    int left =  0, right = main->servers_count - 1, middle = 0;
    unsigned int hash_server = 0, hash_key = 0;

    while (left <= right) {
        middle = (left + right) / 2;

        hash_server = hash_function_servers(&(main->hashring[middle]));
        hash_key = hash_function_servers(&server_id);

        if (hash_server > hash_key)
            right = middle - 1;
        else
            left = middle + 1;
    }
    *position = left;
}


/**
 * redirect_objects()
 * Functia are 2 utilitati:
 *		(1): Calculeaza server-ul unde trebuie mutat un obiect.
 *		(2): Verifica daca trebuie mutat obiectul respectiv intr-un nou server
 *		in momentul in care se adauga un nou server in sistem.(*) In caz
 *		afirmativ, muta obiectul prin functia de store.
 *
 *			(*) In momentul adaugarii unui nou server in sistem, se cauta cel
 *			mai apropiat server si se redistribuie obiectele, daca este nevoie
 *				Se verifica sa nu se redistribuie obiecte din server-ul cu ID
 *			#N in acelasi server.
 *
 *		Functia parcurge fiecare bucket din server-ul primit ca parametru.
 *		Se obtine pentru fiecare obiect serverul unde ar trebui adaugat.
 *		(vezi: (A)). Se verifica (*).
 *
**/

void redirect_objects(load_balancer *main, server_memory *server) {
	if(main == NULL)
		return;

    ll_node_t *current_node = NULL;
    int i = 0, new_id = 0, index = 0;
	struct info *data = NULL;
    server_memory *new_server = NULL;

    for (i = 0; i < server->hmax; i++) {
        if (server->buckets[i]->size != 0) {
            current_node = server->buckets[i]->head;
            while (current_node != NULL) {
            	data = (struct info*)current_node->data;

                binary_search_object(main, (char *)data->key, &index);
                index = index%main->servers_count;
                new_id = main->hashring[index] % 100000;
                search_server(main, &new_id, &index);
                new_server = main->servers[index];

                if (server->ID != main->servers[index]->ID)
                    server_store(new_server, (char *)data->key,
                    				(char *)data->value);

                current_node = current_node->next;
            }
        }
    }
}

/**
 * check_objects()
 * Functia este folosita in momentul adaugarii unui nou server in sistem.
 * O data adaugat un server nou (#1), trebuie verificat daca obiectele aflate
 * in hashtableul vecinului sau (#2) nu trebuie mutate in serverul
 * nou adaugat (#1).
 *
 * Functia calculeaza pozitia si ID-ul serverului in care trebuie efectuata
 * verificarea (#2) (check_position, check_ID) pe principiul descris la (A).
 *
 * Se apeleaza functia de redirect_objects pentru serverul (#2)
 *
**/

void check_objects(load_balancer *main, int position)
{
	if (main == NULL)
		return;

	int check_position = (position + 1) % main->servers_count;
	int check_ID = main->hashring[check_position] % 100000;
	search_server(main, &check_ID, &position);

	redirect_objects(main, main->servers[position]);
}


/**
 * Functia de ADD_SERVER
 * loader_add_server()
 * Pentru a adauga un nou server in sistem sunt necesare 2 operatii:
 *			(1): Server: Se redimensioneaza vectorul de servere si se adauga
 *			pe utlima pozitie un nou server, initializat cu ID-ul primit ca
 *			parametru.
 *			(2): Hashring: Se cauta prin cautare binara indexul din hashring
 *			unde trebuie adaugata noua eticheta. Se adauga prin functia
 *			add_label_hashring() eticheta in hashring. Se verifica daca
 *			trebuie mutate obiectele.
 * 							Operatia se repeta de 3 ori, pentru fiecare
 *			server se adauga in hashring 3 etichete (vezi: (B))
 *
 * 
 * add_label_hashring()
 *			Redimensioneaza vectorul de servere si se produce o shiftare la
 *			dreapta a sa, pentru a face loc unei noi etichete.
 *
 * EXEMPLU right_shift():
 * Se urmareste adaugarea etichetei 2002 pe pozitia 2:
 *
 *	hashring (inainte): 1000 2000 3000 4000
 *	hashring (dupa PAS #1): 1000 2000 2000 3000 4000 -->
 *		-->	 (dupa PAS #2): 1000 2000 2002 3000 4000
**/

void right_shift(load_balancer *main, int position)
{
	if(main == NULL)
		return;
	for (int i = main->servers_count - 1; i >= position; i--)
        main->hashring[i + 1] = main->hashring[i];
}

void add_label_hashring(load_balancer *main, int label, int position)
{
    if (main == NULL)
        return;

    resize_hashring(main, main->servers_count + 1);
    right_shift(main, position);

    main->hashring[position] = label;
    return;
}

void loader_add_server(load_balancer* main, int server_id)
{
    if (main == NULL)
        return;

    int position;

    resize_server_array(main, main->servers_count/3 + 1);
    main->servers[main->servers_count/3] = init_server_memory();
    main->servers[main->servers_count/3]->hash_function = hash_function_key;
    main->servers[main->servers_count/3]->compare_function = compare_strings;
    main->servers[main->servers_count/3]->ID = server_id;

    binary_search(main, server_id, &position);
    add_label_hashring(main, server_id, position);
    main->servers_count++;
    check_objects(main, position);

    binary_search(main, 100000 + server_id, &position);
    add_label_hashring(main, 100000 + server_id, position);
    main->servers_count++;
    check_objects(main, position);

    binary_search(main, 2 * 100000 + server_id, &position);
    add_label_hashring(main, 2 * 100000 + server_id, position);
    main->servers_count++;
    check_objects(main, position);
}


/**
 * Functia de STORE:
 *   Se aplica iar procedeul descris la (A) pentru a extrage serverul unde
 * urmeaza sa fie adaugat un nou obiect
 *   Se apeleaza functia server_store() pentru a introduce o noua pereche
 * key-value in hashtableul serverului descoperit anterior.
 *
**/

void loader_store(load_balancer* main, char* key, char* value, int* server_id) {
    if (main == NULL)
        return;

    int position;
    binary_search_object(main, key, &position);
    position = position%main->servers_count;
    *server_id = main->hashring[position] % 100000;
    search_server(main, server_id, &position);

    server_store(main->servers[position], key, value);
}

/**
 * Functia de RETRIEVE:
 *   Se aplica iar procedeul descris la (A) pentru a extrage serverul unde
 * trebuie cautat obiectul.
 *   Se apeleaza functia de server_retrieve() pentru a cauta o cheie in
 * hashtable-ul serverului descoperit anterior.
**/

char *loader_retrieve(load_balancer* main, char* key, int* server_id) {
    if (main == NULL)
        return NULL;

    int position;
    binary_search_object(main, key, &position);
    position = position%main->servers_count;
    *server_id = main->hashring[position] % 100000;
    search_server(main, server_id, &position);

    char *object = server_retrieve(main->servers[position], key);
    return object;
}


/**
 * Functia de REMOVE SERVER
 * Pentru a elimina un server din sistem sunt necesare 2 operatii:
 *		(1): Hashring: Se redimensioneaza si se elimina etichetele din
 *		hashring printr-o shiftare la stanga (functia left_shift()). 
 *
 *	EXEMPLU left_shift():
 *	Se urmareste eiminarea etichetei 2002 de pe pozitia 2
 *		hashring (inainte): 1000 2000 2002 3000 4000
 *		hashring (dupa PAS #1): 1000 2000 3000 4000 4000 -->
 *			-->  (dupa PAS #2): 1000 2000 3000 4000
 *
 *		(2) Server: Se redimensioneaza vectorul de servere si se
 *		apeleaza functia de free_server_memory(#server) care elibereaza memoria
 *		ocupata de serverul pe care vrem sa il stergem (#server).
 *
 *
 *	remove_hashring_entries()
 *		--> functia este apelata in pasul (1)
 *		--> cauta in hashring toate etichetele asociate serverului pe care
 *		trebuie sa il stergem (vezi: (B))
 *		--> apeleaza functia de left_shift()
 *
 *	Pentru pasul (2):
 * 	Se extrage pointer la serverul ce urmeaza sa fie sters
 *	O data sterse intrarile din hashring, se redistribuie obiectele din
 * 	hashtable-ul serverului gasit anterior. Se elibereaza memoria apoi se
 *	shifteaza vectorul de servere.
 *
**/

void left_shift(load_balancer *main, int position)
{
	if(main == NULL)
		return;

    for (int i = position; i < main->servers_count - 1; i++)
        main->hashring[i] = main->hashring[i + 1];

    resize_hashring(main, main->servers_count - 1);
    main->servers_count--;
    return;
}

void left_shift_servers(load_balancer *main, int position)
{
	if(main == NULL)
		return;

	for (int i = position; i< main->servers_count/3; i++)
		main->servers[i] = main->servers[i + 1];
}

void remove_hashring_entries(load_balancer *main, int server_id)
{
    if (main  == NULL)
        return;

    for (int i = 0; i < main->servers_count; i++) {
        if (main->hashring[i] % 100000 == server_id) {
            left_shift(main, i);
            i--;
        }
    }
}

void loader_remove_server(load_balancer* main, int server_id)
{
	if (main == NULL)
        return;

    remove_hashring_entries(main, server_id);

    int position;
    search_server(main, &server_id, &position);
    redirect_objects(main, main->servers[position]);

    free_server_memory(main->servers[position]);

    main->servers[position] = NULL;
    left_shift_servers(main, position);
    resize_server_array(main, main->servers_count/3);
}


/**
 * Functia de DESTROY:
 * Elibereaza memoria ocupata de fiecare server din vectorul de servere
 * Elibereaza memoria ocupata de hashring si vector de servere si in final
 * memoria ocupata de load balancer
**/

void free_load_balancer(load_balancer* main)
{
	if(main == NULL)
		return;

    for (int i = 0; i < main->servers_count/3; i++)
    	free_server_memory(main->servers[i]);
    free(main->hashring);
    free(main->servers);
    free(main);
}
