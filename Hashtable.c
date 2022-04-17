/* Copyright 2021 Adrian Croitoru */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#include "Hashtable.h"

#define MAX_BUCKET_SIZE 64

/*
 * Functii de comparare a cheilor:
 */
int
compare_function_ints(void *a, void *b)
{
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

int
compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
unsigned int
hash_function_int(void *a)
{
	/*
	 * Credits: https://stackoverflow.com/a/12996028/7883884
	 */
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int
hash_function_string(void *a)
{
	/*
	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
	 */
	unsigned char *puchar_a = (unsigned char*) a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

/*
 * Functie apelata dupa alocarea unui hashtable pentru a-l initializa.
 * Trebuie alocate si initializate si listele inlantuite.
 */
hashtable_t *
ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*))
{
	hashtable_t *result = malloc(sizeof(*result));
	DIE(result == NULL, "malloc failed");

	result->hmax = hmax;
	result->size = 0;
	result->compare_function = compare_function;
	result->hash_function = hash_function;

	result->buckets = malloc(hmax * sizeof(*(result->buckets)));
	DIE(result->buckets == NULL, "malloc failed");

	unsigned int i = 0;
	for(; i < hmax; i++)
	{
		result->buckets[i] = ll_create(sizeof(struct info));
	}

	return result;
}

/*
 * Atentie! Desi cheia este trimisa ca un void pointer (deoarece nu se impune tipul ei), in momentul in care
 * se creeaza o noua intrare in hashtable (in cazul in care cheia nu se gaseste deja in ht), trebuie creata o copie
 * a valorii la care pointeaza key si adresa acestei copii trebuie salvata in structura info asociata intrarii din ht.
 * Pentru a sti cati octeti trebuie alocati si copiati, folositi parametrul key_size_bytes.
 *
 * Motivatie:
 * Este nevoie sa copiem valoarea la care pointeaza key deoarece dupa un apel put(ht, key_actual, value_actual),
 * valoarea la care pointeaza key_actual poate fi alterata (de ex: *key_actual++). Daca am folosi direct adresa
 * pointerului key_actual, practic s-ar modifica din afara hashtable-ului cheia unei intrari din hashtable.
 * Nu ne dorim acest lucru, fiindca exista riscul sa ajungem in situatia in care nu mai stim la ce cheie este
 * inregistrata o anumita valoare.
 */
void
ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size)
{
	if(ht == NULL)
	{
		fprintf(stderr, "error\n");
		return;
	}
	unsigned int index = ht->hash_function(key) % ht->hmax;

	linked_list_t *bucket = ht->buckets[index];

	/* updated value */
	ll_node_t *current = bucket->head;
	while(current != NULL)
	{
		struct info* info = (struct info*)current->data;
		if(ht->compare_function(key, info->key) == 0)
		{
			free(info->value);
			info->value = malloc(value_size);
			DIE(info->value == NULL, "malloc failed");
			memcpy(info->value, value, value_size);
			return;
		}
		current = current->next;
	}

	/* adaugare perechea cheie valoare */
	struct info info;
	info.value = malloc(value_size);
	DIE(info.value == NULL, "malloc failed");
	info.key = malloc(key_size);
	DIE(info.key == NULL, "malloc failed");
	memcpy(info.value, value, value_size);
	memcpy(info.key, key, key_size);

	ll_add_nth_node(bucket, 0, &info);

	ht->size++;
}

void *
ht_get(hashtable_t *ht, void *key)
{
	if(ht == NULL)
	{
		fprintf(stderr, "error\n");
		return NULL;
	}
	unsigned int index = ht->hash_function(key) % ht->hmax;

	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	while(current != NULL)
	{
		struct info* info = (struct info*)current->data;
		if(ht->compare_function(key, info->key) == 0)
		{
			return info->value;
		}
		current = current->next;
	}

	return NULL;
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable folosind functia put
 * 0, altfel.
 */
int
ht_has_key(hashtable_t *ht, void *key)
{
	if(ht == NULL)
	{
		fprintf(stderr, "error\n");
		return 0;
	}
	unsigned int index = ht->hash_function(key) % ht->hmax;

	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	while(current != NULL)
	{
		struct info* info = (struct info*)current->data;
		if(ht->compare_function(key, info->key) == 0)
		{
			return 1;
		}
		current = current->next;
	}

	return 0;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key.
 * Atentie! Trebuie avuta grija la eliberarea intregii memorii folosite pentru o intrare din hashtable (adica memoria
 * pentru copia lui key --vezi observatia de la procedura put--, pentru structura info si pentru structura Node din
 * lista inlantuita).
 */
void
ht_remove_entry(hashtable_t *ht, void *key)
{
	if(ht == NULL)
	{
		fprintf(stderr, "error\n");
		return;
	}
	unsigned int index = ht->hash_function(key) % ht->hmax;

	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	int index2 = 0;
	while(current != NULL)
	{
		struct info* info = (struct info*)current->data;
		if(ht->compare_function(key, info->key) == 0)
		{
			free(info->key);
			free(info->value);
			free(info);

			ll_node_t *removed = ll_remove_nth_node(bucket, index2);
			free(removed);
			ht->size--;
			return;
		}
		index2++;
		current = current->next;
	}
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable, dupa care elibereaza si memoria folosita
 * pentru a stoca structura hashtable.
 */
void
ht_free(hashtable_t *ht)
{
	if(ht == NULL)
	{
		fprintf(stderr, "error\n");
		return;
	}

	unsigned int index = 0;
	while (index < ht->hmax) {
		linked_list_t *bucket = ht->buckets[index];
		ll_node_t *current = bucket->head;
		while(current != NULL)
		{
			struct info* info = (struct info*)current->data;
			free(info->key);
			free(info->value);
			current = current->next;
		}

		ll_free(&bucket);
		index++;
	}

	free(ht->buckets);
	free(ht);
}

unsigned int
ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int
ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}
