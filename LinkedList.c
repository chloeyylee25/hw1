/*
 * Copyright ©2025 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <stdlib.h>

#include "CSE333.h"
#include "LinkedList.h"
#include "LinkedList_priv.h"


///////////////////////////////////////////////////////////////////////////////
// LinkedList implementation.

LinkedList* LinkedList_Allocate(void) {
  // Allocate the linked list record.
  LinkedList *ll = (LinkedList *) malloc(sizeof(LinkedList));
  Verify333(ll != NULL);

  // STEP 1: initialize the newly allocated record structure.
  ll->num_elements = 0;
  ll->head = NULL;
  ll->tail = NULL;

  // Return our newly minted linked list.
  return ll;
}

void LinkedList_Free(LinkedList *list,
                     LLPayloadFreeFnPtr payload_free_function) {
  Verify333(list != NULL);
  Verify333(payload_free_function != NULL);

  // STEP 2: sweep through the list and free all of the nodes' payloads
  // (using the payload_free_function supplied as an argument) and
  // the nodes themselves.
  LinkedListNode *curr = list->head;
  while (curr != NULL) {
    LinkedListNode *next = curr->next;
    payload_free_function(curr->payload);
    free(curr);
    curr = next;
  }

  // free the LinkedList
  free(list);
}

int LinkedList_NumElements(LinkedList *list) {
  Verify333(list != NULL);
  return list->num_elements;
}

void LinkedList_Push(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {
    // Degenerate case: list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // STEP 3: typical case: list has >=1 elements
    ln->next = list->head;
    ln->prev = NULL;
    list->head->prev = ln;
    list->head = ln;
    list->num_elements++;
  }
}

bool LinkedList_Pop(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 4: implement LinkedList_Pop.  Make sure you test for
  // and empty list and fail.  If the list is non-empty, there
  // are two cases to consider: (a) a list with a single element in it
  // and (b) the general case of a list with >=2 elements in it.
  // Be sure to call free() to deallocate the memory that was
  // previously allocated by LinkedList_Push().

  // for an empty list
  if (list->num_elements == 0) {
    return false;
  }

  // the node to be popped (the head of the list)
  LinkedListNode *old_head = list->head;
  *payload_ptr = old_head->payload;

  // for a list with a single element
  if (list->num_elements == 1) {
    list->head = list->tail = NULL;
  } else {  // general case of a list with >=2 elements
    list->head = old_head->next;
    list->head->prev = NULL;
  }

  // freeing the node that was popped
  free(old_head);
  // updating the num_elements field
  list->num_elements--;

  return true;  // you may need to change this return value
}

void LinkedList_Append(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // STEP 5: implement LinkedList_Append.  It's kind of like
  // LinkedList_Push, but obviously you need to add to the end
  // instead of the beginning.

  // malloc for the new node being appended
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  // make sure the node is not NULL
  Verify333(ln != NULL);

  ln->payload = payload;

  // checking for an empty list
  if (list->num_elements == 0) {
    // setting next and prev as NULL because it is the only element in the list
    ln->next = ln->prev = NULL;
    // setting the head and tail to the node because
    // it is the only element in the list
    list->head = list->tail = ln;
  } else {  // for a non-empty list (list containing >=1 element)
    ln->prev = list->tail;
    ln->next = NULL;
    list->tail->next = ln;
    list->tail = ln;
  }

  // updating the num_elements field
  list->num_elements++;
}

void LinkedList_Sort(LinkedList *list, bool ascending,
                     LLPayloadComparatorFnPtr comparator_function) {
  Verify333(list != NULL);
  if (list->num_elements < 2) {
    // No sorting needed.
    return;
  }

  // We'll implement bubblesort! Nice and easy, and nice and slow :)
  int swapped;
  do {
    LinkedListNode *curnode;

    swapped = 0;
    curnode = list->head;
    while (curnode->next != NULL) {
      int compare_result = comparator_function(curnode->payload,
                                               curnode->next->payload);
      if (ascending) {
        compare_result *= -1;
      }
      if (compare_result < 0) {
        // Bubble-swap the payloads.
        LLPayload_t tmp;
        tmp = curnode->payload;
        curnode->payload = curnode->next->payload;
        curnode->next->payload = tmp;
        swapped = 1;
      }
      curnode = curnode->next;
    }
  } while (swapped);
}


///////////////////////////////////////////////////////////////////////////////
// LLIterator implementation.

LLIterator* LLIterator_Allocate(LinkedList *list) {
  Verify333(list != NULL);

  // OK, let's manufacture an iterator.
  LLIterator *li = (LLIterator *) malloc(sizeof(LLIterator));
  Verify333(li != NULL);

  // Set up the iterator.
  li->list = list;
  li->node = list->head;

  return li;
}

void LLIterator_Free(LLIterator *iter) {
  Verify333(iter != NULL);
  free(iter);
}

bool LLIterator_IsValid(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);

  return (iter->node != NULL);
}

bool LLIterator_Next(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 6: try to advance iterator to the next node and return true if
  // you succeed, false otherwise
  // Note that if the iterator is already at the last node,
  // you should move the iterator past the end of the list

  // if the current node is the last node,
  // then set the new node that the iterator points to as NULL
  // (past the end of the list) and return false
  if (iter->node->next == NULL) {
    iter->node = NULL;
    return false;
  } else {
  // if the current node is not the last node,
  // set the current node that the iterator points to as the next node
  // and return true
    iter->node = iter->node->next;
    return true;
  }
}

void LLIterator_Get(LLIterator *iter, LLPayload_t *payload) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  *payload = iter->node->payload;
}

bool LLIterator_Remove(LLIterator *iter,
                       LLPayloadFreeFnPtr payload_free_function) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 7: implement LLIterator_Remove.  This is the most
  // complex function you'll build.  There are several cases
  // to consider:
  // - degenerate case: the list becomes empty after deleting.
  // - degenerate case: iter points at head
  // - degenerate case: iter points at tail
  // - fully general case: iter points in the middle of a list,
  //                       and you have to "splice".
  //
  // Be sure to call the payload_free_function to free the payload
  // the iterator is pointing to, and also free any LinkedList
  // data structure element as appropriate.

  // the node that the iterator is currently pointing to
  // is the node we want to delete
  LinkedListNode *node = iter->node;
  // the list from which the node is being removed
  // is the list that the iterator is pointing to
  LinkedList *list = iter->list;

  // freeing the payload of the node we want to remove
  payload_free_function(node->payload);

  // degenerate case: the list becomes empty after deleting
  // (currently has one element)
  if (list->num_elements == 1) {
    list->head = list->tail = NULL;
    iter->node = NULL;
    free(node);
    list->num_elements--;
    return false;
  }

  if (node == list->head) {  // degenerate case: iter points at head
    list->head = node->next;
    list->head->prev = NULL;
    iter->node = list->head;
  } else if (node == list->tail) {  // degenerate case: iter points at tail
    list->tail = node->prev;
    list->tail->next = NULL;
    iter->node = list->tail;
  } else {
    // fully general case: iter points in the middle of a list,
    // and you have to "splice"
    node->prev->next = node->next;
    node->next->prev = node->prev;
    iter->node = node->next;
  }

  // free the node that was deleted
  free(node);
  // update the num_elements field since the node was successfully deleted
  list->num_elements--;
  return true;  // you may need to change this return value
}


///////////////////////////////////////////////////////////////////////////////
// Helper functions

bool LLSlice(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 8: implement LLSlice.

  // fails if the list is empty, so return false
  if (list->num_elements == 0) {
    return false;
  }

  // the node that we want to delete is the list's tail node
  LinkedListNode *old_tail = list->tail;
  *payload_ptr = old_tail->payload;

  // if there are no remaining elements after the deletion
  // (the list currently contains one node), the result of the deletion
  // would be the head and tail being NULL
  if (list->num_elements == 1) {
    list->head = list->tail = NULL;
  // general case (list currently contains >=2 nodes):
  // the new tail of the list is set as the node that was before the old tail,
  // and the new tail's next field is set to NULL
  // because it is the new end of the list
  } else {
    list->tail = old_tail->prev;
    list->tail->next = NULL;
  }

  // free the node that was deleted
  free(old_tail);
  // update num_elements to reflect that the node was successfully deleted
  list->num_elements--;

  return true;  // you may need to change this return value
}

void LLIteratorRewind(LLIterator *iter) {
  iter->node = iter->list->head;
}
