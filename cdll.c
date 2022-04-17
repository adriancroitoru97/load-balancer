/* Copyright 2021 Adrian Croitoru */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdll.h"
#include "utils.h"

doubly_linked_list_t*
dll_create(unsigned int data_size)
{
    doubly_linked_list_t *list = malloc(sizeof(*list));
    DIE(!list, "Malloc error");

    list->head = NULL;
    list->data_size = data_size;
    list->size = 0;

    return list;
}

dll_node_t*
dll_get_nth_node(doubly_linked_list_t *list, unsigned int n)
{
    if (!list || list->size == 0)
        return NULL;

    dll_node_t *current = list->head;

    unsigned int index = 0;
    n = n % list->size;
    while (current && current->next && index < n) {
        current = current->next;
        ++index;
    }

    return current;
}

void
dll_add_nth_node(doubly_linked_list_t *list, unsigned int n, const void* data)
{
    if (!list)
        return;

    dll_node_t *current = list->head;
    unsigned int index = 0;
    while (current && current->next && index + 1 < n) {
        current = current->next;
        ++index;
    }

    dll_node_t *new_node = malloc(sizeof(*new_node));
    DIE(!new_node, "Malloc error");
    new_node->data = malloc(list->data_size);
    DIE(!new_node->data, "Malloc error");
    memcpy(new_node->data, data, list->data_size);
    new_node->next = NULL;
    new_node->prev = NULL;

    dll_node_t *nextnode = NULL, *prevnode = NULL;
    if (n > 0) {
        if (current) {
            nextnode = current->next;
        } else {
            list->head = new_node;
            list->head->next = new_node;
            list->head->prev = new_node;
            list->size = 1;
            return;
        }

        current->next = new_node;
        if (nextnode)
            nextnode->prev = new_node;
        new_node->next = nextnode;
        new_node->prev = current;
    } else if (n == 0) {
        if (current) {
            prevnode = current->prev;
        } else {
            list->head = new_node;
            list->head->next = new_node;
            list->head->prev = new_node;
            list->size = 1;
            return;
        }

        current->prev = new_node;
        if (prevnode)
            prevnode->next = new_node;
        new_node->next = current;
        new_node->prev = prevnode;
        list->head = new_node;
    }
    list->size++;
}

dll_node_t*
dll_rm_nth_node(doubly_linked_list_t *list, unsigned int n)
{
    if (!list)
        return NULL;
    if (!list->head)
        return NULL;

    dll_node_t *removed, *prev, *next;
    if (list->size == 1) {
        removed = list->head;
        list->head = NULL;
    } else {
        if (n + 1 >= list->size)
            n = list->size - 1;
        removed = dll_get_nth_node(list, n);
        prev = removed->prev;
        next = removed->next;
        prev->next = removed->next;
        next->prev = removed->prev;
        if (removed == list->head)
            list->head = removed->next;
    }

    list->size--;

    return removed;
}

unsigned int
dll_get_size(doubly_linked_list_t *list)
{
    return list->size;
}

void
dll_free(doubly_linked_list_t** pp_list)
{
    dll_node_t *current, *temp;
    current = (*pp_list)->head;
    unsigned int index = 0;
    while (current && index < (*pp_list)->size) {
        index++;
        free(current->data);
        temp = current->next;
        free(current);
        current = temp;
    }
    free(*(pp_list));
    (*pp_list) = NULL;
}

void
dll_print_int_list(doubly_linked_list_t *list)
{
    if (!list)
        return;

    dll_node_t *current = list->head;
    unsigned int index = 0;
    while (current && index < list->size) {
        index++;
        printf("%d ", *(int *)current->data);
        current = current->next;
    }

    printf("\n");
}

void
dll_print_string_list(doubly_linked_list_t *list)
{
    if (!list)
        return;

    dll_node_t *current = list->head;
    unsigned int index = 0;
    while (current && index < list->size) {
        index++;
        printf("%s ", (char *)current->data);
        current = current->next;
    }

    printf("\n");
}

void
dll_print_ints_left_circular(dll_node_t *start)
{
    dll_node_t *current = start;

    printf("%d ", *(int *)current->data);
    current = current->prev;

    while (current != start) {
        printf("%d ", *(int *)current->data);
        current = current->prev;
    }

    printf("\n");
}

void
dll_print_ints_right_circular(dll_node_t *start)
{
    dll_node_t *current = start;

    printf("%d ", *(int *)current->data);
    current = current->next;

    while (current != start) {
        printf("%d ", *(int *)current->data);
        current = current->next;
    }

    printf("\n");
}
