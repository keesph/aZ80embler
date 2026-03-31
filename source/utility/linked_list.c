#include "linked_list.h"

#include "logging/logging.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Handles
typedef struct node
{
  void *data;
  ListNode *previous;
  ListNode *next;
} ListNode;

typedef struct list
{
  size_t nodeCount;
  size_t dataSize;
  ListNode *head;
  ListNode *tail;
  free_callback freeCallback;
  compare_callback compareCallback;
} LinkedList;

static void remove_node(ListNode *node, free_callback freeCb)
{
  assert(node);
  if (freeCb != NULL)
  {
    freeCb(node->data);
  }
  else
  {
    free(node->data);
  }
  free(node);
}

/**************************************************************************************************/
/**************************************************************************************************/
LinkedList *linkedList_initialize(size_t size, free_callback freeCb, compare_callback compareCb)
{
  LinkedList *list = calloc(1, sizeof(LinkedList));
  if (!list)
  {
    LOG_ERROR("Malloc failed when initializing a new list!");
    return list;
  }

  list->nodeCount = 0;
  list->dataSize = size;
  list->head = NULL;
  list->tail = NULL;
  list->freeCallback = freeCb;
  list->compareCallback = compareCb;
  return list;
}

/**************************************************************************************************/
/**************************************************************************************************/
void linkedList_destroy(LinkedList *list)
{
  assert(list);

  if (list->nodeCount == 0)
  {
    free(list);
    return;
  }

  ListNode *node = list->head;
  while (node != NULL)
  {
    list->head = node->next;
    remove_node(node, list->freeCallback);
    node = list->head;
  }
  free(list);
}

/**************************************************************************************************/
/**************************************************************************************************/
void linkedList_clear(LinkedList *list)
{
  assert(list);

  if (list->nodeCount == 0)
  {
    return;
  }

  ListNode *node = list->head;
  while (node != NULL)
  {
    list->head = node->next;
    remove_node(node, list->freeCallback);
    node = list->head;
  }

  list->tail = NULL;
  list->head = NULL;

  list->nodeCount = 0;
}

/**************************************************************************************************/
/**************************************************************************************************/
void linkedList_iterate(LinkedList *list, iterate_callback iterateCb)
{
  assert(list);

  ListNode *node = list->head;
  uint32_t index = 0;
  while (node != NULL)
  {
    iterateCb(node->data, index++);
    node = node->next;
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
size_t linkedList_count(LinkedList *list)
{
  assert(list);

  return list->nodeCount;
}

/**************************************************************************************************/
/**************************************************************************************************/
void linkedList_append(LinkedList *list, void *data)
{
  assert(list);
  assert(data);

  ListNode *node = calloc(1, sizeof(ListNode));
  if (!node)
  {
    LOG_ERROR("Malloc failed for new list node when appending to list!");
    abort();
  }

  node->data = malloc(list->dataSize);

  if (!node->data)
  {
    LOG_ERROR("Malloc failed for data of new list node when appending to list!");
    abort();
  }

  memcpy(node->data, data, list->dataSize);

  if (list->tail == NULL)
  {
    list->head = node;
    list->tail = node;
  }
  else
  {
    node->previous = list->tail;
    list->tail->next = node;
    list->tail = node;
  }
  list->nodeCount++;
}

/**************************************************************************************************/
/**************************************************************************************************/
bool linkedList_contains(LinkedList *list, void *data)
{
  assert(list);
  assert(list->compareCallback);
  assert(data);

  ListNode *node = list->head;
  while (node != NULL)
  {
    if (list->compareCallback(data, node->data))
    {
      return true;
    }
    node = node->next;
  }
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
bool linkedList_remove(LinkedList *list, void *data)
{
  assert(list);
  assert(list->compareCallback);
  assert(data);

  ListNode *node = list->head;

  while (node != NULL)
  {
    if (list->compareCallback(node->data, data))
    {
      linkedList_removeNode(list, node);
      return true;
    }
    node = node->next;
  }
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
ListNode *linkedList_removeNode(LinkedList *list, ListNode *node)
{
  assert(list);
  assert(node);

  if (node->previous != NULL)
  {
    node->previous->next = node->next;
  }
  if (node->next != NULL)
  {
    node->next->previous = node->previous;
  }

  if (node == list->head)
  {
    list->head = node->next;
  }
  if (node == list->tail)
  {
    list->tail = node->previous;
  }

  ListNode *nextNode = node->next;
  remove_node(node, list->freeCallback);
  list->nodeCount--;

  return nextNode;
}

/**************************************************************************************************/
/**************************************************************************************************/
ListNode *linkedList_getFirstNode(LinkedList *list)
{
  assert(list);
  return list->head;
}

/**************************************************************************************************/
/**************************************************************************************************/
ListNode *linkedList_getLastNode(LinkedList *list)
{
  assert(list);
  return list->tail;
}

/**************************************************************************************************/
/**************************************************************************************************/
ListNode *listNode_getNext(ListNode *node)
{
  assert(node);
  return node->next;
}

/**************************************************************************************************/
/**************************************************************************************************/
ListNode *listNode_getPrevious(ListNode *node)
{
  assert(node);
  return node->previous;
}

/**************************************************************************************************/
/**************************************************************************************************/
void *listNode_getData(ListNode *node)
{
  assert(node);
  return node->data;
}
