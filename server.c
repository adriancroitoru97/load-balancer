/* Copyright 2021 Adrian Croitoru */

#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "Hashtable.h"
#include "utils.h"

#define HT_MAX 15

server_memory* init_server_memory() {
	server_memory *server = malloc(sizeof(*server));
	DIE(!server, "server malloc error");
	server->hashtable = ht_create(HT_MAX, hash_function_string,
								  compare_function_strings);
	return server;
}

void server_store(server_memory* server, char* key, char* value) {
	ht_put(server->hashtable, key, strlen(key) + 1, value, strlen(value) + 1);
}

void server_remove(server_memory* server, char* key) {
	ht_remove_entry(server->hashtable, key);
}

char* server_retrieve(server_memory* server, char* key) {
	return ht_get(server->hashtable, key);
}

void free_server_memory(server_memory* server) {
	ht_free(server->hashtable);
	free(server);
}
