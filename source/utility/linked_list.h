#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>
#include <stdint.h>

// Callback function types
typedef void (*free_callback)(void *);
typedef bool (*compare_callback)(void *, void *);
typedef void(iterate_callback)(void *, uint32_t);

// Handles
typedef struct node ListNode;
typedef struct list LinkedList;

// List operations
LinkedList *linkedList_initialize(size_t size, free_callback freeCb, compare_callback compareCb);
void linkedList_destroy(LinkedList *list);
void linkedList_iterate(LinkedList *list, iterate_callback iterateCb);
size_t linkedList_count(LinkedList *list);

// Data based list operations
void linkedList_append(LinkedList *list, void *data);
bool linkedList_remove(LinkedList *list, void *data);
bool linkedList_contains(LinkedList *list, void *data);

// Node based list operations
ListNode *linkedList_removeNode(LinkedList *list, ListNode *node);
ListNode *linkedList_getFirstNode(LinkedList *list);
ListNode *linkedList_getLastNode(LinkedList *list);
ListNode *listNode_getNext(ListNode *node);
ListNode *listNode_getPrevious(ListNode *node);
void *listNode_getData(ListNode *node);

#endif
