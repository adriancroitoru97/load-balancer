/* Copyright 2021 Adrian Croitoru */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "load_balancer.h"
#include "server.h"
#include "cdll.h"
#include "LinkedList.h"
#include "utils.h"

struct load_balancer {
	doubly_linked_list_t *h_ring;  /* hashring-ul circular */
	doubly_linked_list_t *servers;  /* lista adreselor serverelor */
	doubly_linked_list_t *servers_ids;  /* lista id-urilor serverelor */
};

unsigned int hash_function_servers(void *a) {
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

load_balancer* init_load_balancer() {
	load_balancer *load_balancer1 = malloc(sizeof(*load_balancer1));
	DIE(!load_balancer1, "load_balancer malloc error");

	load_balancer1->servers = dll_create(sizeof(server_memory*));
	load_balancer1->h_ring = dll_create(sizeof(unsigned int));
	load_balancer1->servers_ids = dll_create(sizeof(unsigned int));

    return load_balancer1;
}

void loader_store(load_balancer* main, char* key, char* value, int* server_id) {
	/* se obtine serverul pe care trebuie stocat obiectul dat ca parametru */
	server_memory *current_server = find_server(main, key, server_id);

	/* se verifica daca obiectul nu se afla deja pe server */
	if (!ht_has_key(current_server->hashtable, key)) {
		server_store(current_server, key, value);
	}
}


char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
	/* se obtine serverul pe care se afla obiectul dat ca parametru */
	server_memory *current_server = find_server(main, key, server_id);

	return server_retrieve(current_server, key);
}

void loader_add_server(load_balancer* main, int server_id) {
	/* se adauga 3 pozitii pe hashring - cele 3 tag-uri ale noului server */
	for (int i = 0; i < 3; i++) {
		add_server_h_ring(main, server_id);
		server_id += 100000;
	}
	server_id %= 100000;

	/* Noul server este initializat, se adauga adresa noului server in lista
	   main->servers si id-ul acestuia in lista main->servers_ids */
	server_memory *new_server = init_server_memory();
	dll_add_nth_node(main->servers, 0, &new_server);
	dll_add_nth_node(main->servers_ids, 0, &server_id);

	/* se rebalanseaza obiectele din sistem conform cu noul server adaugat */
	add_server_rebalance(main, server_id);
}


void loader_remove_server(load_balancer* main, int server_id) {
	/* se elimina cele 3 pozitii de pe hashring - tag-urile serverului scos */
	for (int i = 0; i < 3; i++) {
		rmv_server_h_ring(main, server_id);
		server_id += 100000;
	}
	server_id %= 100000;

	/* obiectele de pe serverul scos sunt redistribuite */
	unsigned int index = 0;
	rmv_server_rebalance(main, server_id, &index);

	/* se elimina serverul din liste, se elibereaza memoria */
	dll_node_t *removed2 = dll_rm_nth_node(main->servers, index);
	dll_node_t *removed2_id = dll_rm_nth_node(main->servers_ids, index);
	server_memory *current_server = *(server_memory**)removed2->data;
	free_server_memory(current_server);
	free(removed2->data);
	free(removed2);
	free(removed2_id->data);
	free(removed2_id);
}

void free_load_balancer(load_balancer* main) {
    /* se elibereaza lista aferenta hashring-ului */
    dll_free(&main->h_ring);

    /* se da free memoriei pentru fiecare server */
    unsigned int index = 0;
    dll_node_t *current = main->servers->head;
    while (index < main->servers->size) {
		server_memory *current_server = *(server_memory**)current->data;
		free_server_memory(current_server);
    	current = current->next;
    	++index;
    }

    /* se elibereaza listele de adrese, respectiv id-uri ale serverelor,
       iar in final se da free la load_balancer */
    dll_free(&main->servers);
    dll_free(&main->servers_ids);
    free(main);
}

/* functie ce returneaza serverul pe care se va stoca sau pe care
   se regaseste un anumit obiect */
server_memory*
find_server(load_balancer* main, char* key, int* server_id) {
	unsigned int object_hash = hash_function_key(key);

	/* se parcurge hashring-ul pana se ajunge la serverul dorit */
	dll_node_t *current = main->h_ring->head;
	unsigned int index = 0;
	while (hash_function_servers(current->data) < object_hash &&
		   index < main->h_ring->size) {
		++index;
		current = current->next;
	}

	/* in cazul in care hash-ul obiectului este mai mare decat 
	   serverul de pe ultima pozitie din hashring, serverul corespunzator
	   acestui obiect va fi primul din hashring */
	if (index == main->h_ring->size)
		current = main->h_ring->head;

	/* functia intoarce prin *server_id, id-ul serverului cautat; 
	   se face modulo 100000 pentru a se intoarce id-ul efectiv, si nu
	   tag-ul de pe hashring */
	*server_id = *((int*)current->data);
	(*server_id) %= 100000;

	/* se parcurg listele de servere (de adrese si id-uri) pana se gaseste
	   server-ul cu id-ul server_id */
	dll_node_t *current_server_node = main->servers->head;
	dll_node_t *current_server_node_id = main->servers_ids->head;
	while (*(int*)current_server_node_id->data != *server_id) {
		current_server_node = current_server_node->next;
		current_server_node_id = current_server_node_id->next;
	}

	server_memory *current_server = *(server_memory**)current_server_node->data;

	return current_server;
}

/* functie ce adauga o intrare - un tag - pe hashring */
void
add_server_h_ring(load_balancer* main, int server_id) {
	unsigned int server_hash = hash_function_servers(&server_id);

	dll_node_t *current = main->h_ring->head;
	if (main->h_ring->size == 0) {
		dll_add_nth_node(main->h_ring, 0, &server_id);
	} else if (main->h_ring->size == 1) {
		unsigned int server_hash1 = hash_function_servers(current->data);
		if (server_hash > server_hash1) {
			dll_add_nth_node(main->h_ring, 1, &server_id);
		} else {
			dll_add_nth_node(main->h_ring, 0, &server_id);
		}
	} else {
		unsigned int index = 0;
		while (index < main->h_ring->size + 1) {
			unsigned int server_hash1 = hash_function_servers(current->data);
			unsigned int server_hash2 = hash_function_servers(current->next->data);
			if (server_hash < server_hash2 && server_hash > server_hash1) {
				dll_add_nth_node(main->h_ring, index + 1, &server_id);
				break;
			}

			index++;
			current = current->next;
		}

		if (index > main->h_ring->size) {
			unsigned int server_hash3 =
			hash_function_servers(main->h_ring->head->prev->data);

			if (server_hash > server_hash3)
				dll_add_nth_node(main->h_ring, main->h_ring->size, &server_id);
			else
				dll_add_nth_node(main->h_ring, 0, &server_id);
		}
	}
}

/* functie ce rebalanseaza sistemul cand un nou server este adaugat */
void
add_server_rebalance(load_balancer* main, int server_id) {
	int current_server_id = server_id;

	/* se parcurg cele 3 intrari de pe hashring */
	for (int i = 0; i < 3; i++) {
		/* se gasesc pozitiile noului server, 
		   dar si a serverului urmator din hashring */
		dll_node_t *current = main->h_ring->head;
		while (*(int*)current->data != server_id && main->h_ring->size > 3) {
			current = current->next;
		}

		int next_server_id = *(int*)current->next->data;
		next_server_id %= 100000;

		dll_node_t *current_server_node = main->servers->head;
		dll_node_t *next_server_node = main->servers->head;
		dll_node_t *current_server_node_id = main->servers_ids->head;
		dll_node_t *next_server_node_id = main->servers_ids->head;

		while (*(int*)current_server_node_id->data != current_server_id) {
			current_server_node = current_server_node->next;
			current_server_node_id = current_server_node_id->next;
		}
		while (*(int*)next_server_node_id->data != next_server_id) {
			next_server_node = next_server_node->next;
			next_server_node_id = next_server_node_id->next;
		}

		server_memory *current_server_memory =
		*(server_memory**)current_server_node->data;
		server_memory *next_server_memory = *(server_memory**)next_server_node->data;

		/* obiectele de redistribuit sunt trecute iar prin loader, iar apoi sunt sterse
		   de pe serverul pe care se aflau inainte sa fie mutate pe cel nou */ 
		hashtable_t *ht = next_server_memory->hashtable;
		for (unsigned int i = 0; i < ht->hmax; i++) {
			linked_list_t *bucket = ht->buckets[i];
				ll_node_t *current_b = bucket->head;
				while(current_b != NULL)
				{
					struct info* info = (struct info*)current_b->data;
					int p;
					loader_store(main, info->key, info->value, &p);
					if (p != current_server_id)
						server_remove(current_server_memory, info->key);

					current_b = current_b->next;
				}
		}

		server_id += 100000;
	}
}

/* functie ce sterge o intrare - un tag - de pe hashring */
void
rmv_server_h_ring(load_balancer* main, int server_id) {
	unsigned int server_hash = hash_function_servers(&server_id);

	unsigned int index = 0;
	dll_node_t *current = main->h_ring->head;
	while (index < main->h_ring->size) {
		unsigned int server_hash2 = hash_function_servers(current->data);
		if (server_hash == server_hash2) {
			dll_node_t *removed = dll_rm_nth_node(main->h_ring, index);
			free(removed->data);
			free(removed);
			break;
		}

		++index;
		current = current->next;
	}
}

/* functie ce rebalanseaza sistemul cand un server este sters */
void
rmv_server_rebalance(load_balancer* main, int server_id, unsigned int* index) {
	/* se parcurg listele pana se gaseste serverul cu id server_id */
	dll_node_t *current_server_node = main->servers->head;
	dll_node_t *current_server_node_id = main->servers_ids->head;
	while (*(int*)current_server_node_id->data != server_id) {
		current_server_node = current_server_node->next;
		current_server_node_id = current_server_node_id->next;
		(*index)++;
	}

	server_memory *current_server_memory =
	*(server_memory**)current_server_node->data;

	/* se parcurge hashtable-ul serverului cu id server_id, iar obiectele ce
	   sunt de redistribuit sunt trecute iar prin loader */
	hashtable_t *ht = current_server_memory->hashtable;
	for (unsigned int i = 0; i < ht->hmax; i++) {
		linked_list_t *bucket = ht->buckets[i];
		if (bucket) {
			ll_node_t *current_b = bucket->head;
			while(current_b != NULL)
			{
				int sv_id;
				struct info* info = (struct info*)current_b->data;
				loader_store(main, info->key, info->value, &sv_id);

				current_b = current_b->next;
			}
		}
	}
}
