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
#include <stdint.h>

#include "CSE333.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "HashTable_priv.h"

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions.
//
#define INVALID_IDX -1

// Grows the hashtable (ie, increase the number of buckets) if its load
// factor has become too high.
static void MaybeResize(HashTable *ht);

int HashKeyToBucketNum(HashTable *ht, HTKey_t key) {
  return key % ht->num_buckets;
}

// Deallocation functions that do nothing.  Useful if we want to deallocate
// the structure (eg, the linked list) without deallocating its elements or
// if we know that the structure is empty.
static void LLNoOpFree(LLPayload_t freeme) { }
static void HTNoOpFree(HTValue_t freeme) { }


///////////////////////////////////////////////////////////////////////////////
// HashTable implementation.

HTKey_t FNVHash64(unsigned char *buffer, int len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //     http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  // FNV-1a hash each octet of the buffer.
  while (bp < be) {
    // XOR the bottom with the current octet.
    hval ^= (uint64_t) * bp++;
    // Multiply by the 64 bit FNV magic prime mod 2^64.
    hval *= FNV_64_PRIME;
  }
  return hval;
}

HashTable* HashTable_Allocate(int num_buckets) {
  HashTable *ht;
  int i;

  Verify333(num_buckets > 0);

  // Allocate the hash table record.
  ht = (HashTable *) malloc(sizeof(HashTable));
  Verify333(ht != NULL);

  // Initialize the record.
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets = (LinkedList **) malloc(num_buckets * sizeof(LinkedList *));
  Verify333(ht->buckets != NULL);
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = LinkedList_Allocate();
  }

  return ht;
}

void HashTable_Free(HashTable *table,
                    ValueFreeFnPtr value_free_function) {
  int i;

  Verify333(table != NULL);

  // Free each bucket's chain.
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList *bucket = table->buckets[i];
    HTKeyValue_t *kv;

    // Pop elements off the chain list one at a time.  We can't do a single
    // call to LinkedList_Free since we need to use the passed-in
    // value_free_function -- which takes a HTValue_t, not an LLPayload_t -- to
    // free the caller's memory.
    while (LinkedList_NumElements(bucket) > 0) {
      Verify333(LinkedList_Pop(bucket, (LLPayload_t *)&kv));
      value_free_function(kv->value);
      free(kv);
    }
    // The chain is empty, so we can pass in the
    // null free function to LinkedList_Free.
    LinkedList_Free(bucket, LLNoOpFree);
  }

  // Free the bucket array within the table, then free the table record itself.
  free(table->buckets);
  free(table);
}

int HashTable_NumElements(HashTable *table) {
  Verify333(table != NULL);
  return table->num_elements;
}

// helper function to find a key in a chain and return its key-value pair
// parameters: the chain to traverse through, the key we are wanting to find,
            // and a pointer to the key-value pair
// returns true if the key is found, false otherwise
static bool FindKey(LinkedList *chain, HTKey_t key, HTKeyValue_t **kv_ptr) {
  LLIterator *it;
  HTKeyValue_t *kv;

  it = LLIterator_Allocate(chain);
  while (LLIterator_IsValid(it)) {
    LLIterator_Get(it, (LLPayload_t*)&kv);
    if (kv->key == key) {
      // if the key is found, store the pointer and free the iterator
      *kv_ptr = kv;
      LLIterator_Free(it);
      return true;  // return true since the key was found
    }
    // if the current key is not the key we are looking for,
    // move the iterator to the next element in the chain to keep traversing
    LLIterator_Next(it);
  }
  // if we reach the end of the chain and the key is not found,
  // free the iterator since the search is over
  LLIterator_Free(it);
  return false;  // return false since the key was not found
}

bool HashTable_Insert(HashTable *table,
                      HTKeyValue_t newkeyvalue,
                      HTKeyValue_t *oldkeyvalue) {
  int bucket;
  LinkedList *chain;
  HTKeyValue_t *kv;

  Verify333(table != NULL);
  MaybeResize(table);

  // calculate which bucket the key is in
  bucket = HashKeyToBucketNum(table, newkeyvalue.key);
  // get the linked list at that bucket
  chain = table->buckets[bucket];

  // STEP 1: finish the implementation of InsertHashTable.
  // This is a fairly complex task, so you might decide you want
  // to define/implement a helper function that helps you find
  // and optionally remove a key within a chain, rather than putting
  // all that logic inside here.  You might also find that your helper
  // can be reused in steps 2 and 3.

  if (FindKey(chain, newkeyvalue.key, &kv)) {
    // if the key is found, replace the value
    *oldkeyvalue = *kv;  // copy the old key-value pair
    kv->value = newkeyvalue.value;  // update with the new value
    // return true since the key was found
    // and the old value was replaced with the new value
    return true;
  }

  // if the key is not found, create a new key-value pair
  // and insert it into the chain
  kv = (HTKeyValue_t*)malloc(sizeof(HTKeyValue_t));
  Verify333(kv != NULL);
  *kv = newkeyvalue;
  LinkedList_Push(chain, (LLPayload_t)kv);
  // update num_elements to show a new key-value pair was added
  table->num_elements++;
  // return false because there was no existing pair with that key
  return false;
}

bool HashTable_Find(HashTable *table,
                    HTKey_t key,
                    HTKeyValue_t *keyvalue) {
  int bucket;  // index of the bucket where the key should be
  LinkedList *chain;  // the chain we're traversing through
  HTKeyValue_t *kv;  // the key-value pair

  Verify333(table != NULL);

  // calculate which bucket this key is in
  bucket = HashKeyToBucketNum(table, key);
  // get the linked list at that bucket
  chain = table->buckets[bucket];

  if (FindKey(chain, key, &kv)) {
    // if the key is found, copy the key-value pair
    *keyvalue = *kv;
    return true;  // return true to indicate we found the key
  }
  return false;  // if the key isn't found, return false
}

bool HashTable_Remove(HashTable *table,
                      HTKey_t key,
                      HTKeyValue_t *keyvalue) {
  int bucket;  // the index of the bucket where the key should be
  LinkedList *chain;  // the chain we're iterating through
  HTKeyValue_t *kv;  // the key-value pair
  LLIterator *it;  // the iterator

  Verify333(table != NULL);

  // calculate which bucket this key is in
  bucket = HashKeyToBucketNum(table, key);
  // get the linked list at that bucket
  chain = table->buckets[bucket];

  if (FindKey(chain, key, &kv)) {
    // if the key is found, copy the key-value pair
    *keyvalue = *kv;
    // create an iterator to remove the element
    it = LLIterator_Allocate(chain);
    while (LLIterator_IsValid(it)) {
      HTKeyValue_t *curr;
      LLIterator_Get(it, (LLPayload_t*)&curr);
      if (curr == kv) {
        // the current element is the one we want to remove
        LLIterator_Remove(it, free);
        LLIterator_Free(it);  // free the iterator since we're done
        // update num_elements to show we removed an element from the chain
        table->num_elements--;
        // return true since the key was found, and therefore the associated
        // key-value pair was returned to the caller via that keyvalue return
        // parameter and the key-value pair was removed from the HashTable
        return true;
      }
      // if the current element is not the one we want to remove,
      // continue iterating through the chain
      LLIterator_Next(it);
    }
    // if we reach the end of the chain and the goal element is not found,
    // free the iterator since we are done
    LLIterator_Free(it);
  }
  return false;  // return false since the key wasn't found in the HashTable
}


///////////////////////////////////////////////////////////////////////////////
// HTIterator implementation.

HTIterator* HTIterator_Allocate(HashTable *table) {
  HTIterator *iter;
  int         i;

  Verify333(table != NULL);

  iter = (HTIterator *) malloc(sizeof(HTIterator));
  Verify333(iter != NULL);

  // If the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->ht = table;
    iter->bucket_it = NULL;
    iter->bucket_idx = INVALID_IDX;
    return iter;
  }

  // Initialize the iterator.  There is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (LinkedList_NumElements(table->buckets[i]) > 0) {
      iter->bucket_idx = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLIterator_Allocate(table->buckets[iter->bucket_idx]);
  return iter;
}

void HTIterator_Free(HTIterator *iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it != NULL) {
    LLIterator_Free(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  free(iter);
}

bool HTIterator_IsValid(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 4: implement HTIterator_IsValid.

  // check if the iterator is valid and returning false if it is invalid/NULL
  if (iter->bucket_idx == INVALID_IDX || iter->bucket_it == NULL) {
    return false;
  }

  // checking if the iterator is valid using our function from LinkedList.c
  return LLIterator_IsValid(iter->bucket_it);
}

bool HTIterator_Next(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 5: implement HTIterator_Next.

  // returning false if the iterator is invalid
  if (!HTIterator_IsValid(iter)) {
    return false;
  }

  // trying to iterate through the current bucket
  if (LLIterator_Next(iter->bucket_it)) {
    return true;  // successfuly moved to the next element in current bucket
  }

  // once we reach the end of the current bucket, free the iterator
  LLIterator_Free(iter->bucket_it);
  iter->bucket_it = NULL;

  // searching for the next non-empty bucket
  for (int i = iter->bucket_idx + 1; i < iter->ht->num_buckets; i++) {
    if (LinkedList_NumElements(iter->ht->buckets[i]) > 0) {
      // found a non-empty bucket
      iter->bucket_idx = i;  // update the bucket index
      iter->bucket_it = LLIterator_Allocate(iter->ht->buckets[i]);
      return true;  // successfully moved to the first element in the new bucket
    }
  }

  // if there are no more non-empty buckets found, return false
  iter->bucket_idx = INVALID_IDX;
  return false;
}

bool HTIterator_Get(HTIterator *iter, HTKeyValue_t *keyvalue) {
  HTKeyValue_t *kv;

  Verify333(iter != NULL);

  // STEP 6: implement HTIterator_Get.

  // return false if the iterator is invalid
  if (!HTIterator_IsValid(iter)) {
    return false;
  }

  // get the current element from the iterator
  LLIterator_Get(iter->bucket_it, (LLPayload_t*)&kv);

  // copy the key-value pair to the output parameter
  *keyvalue = *kv;
  return true;  // you may need to change this return value
}

bool HTIterator_Remove(HTIterator *iter, HTKeyValue_t *keyvalue) {
  HTKeyValue_t kv;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  if (!HTIterator_Get(iter, &kv)) {
    return false;
  }

  // Advance the iterator.  Thanks to the above call to
  // HTIterator_Get, we know that this iterator is valid (though it
  // may not be valid after this call to HTIterator_Next).
  HTIterator_Next(iter);

  // Lastly, remove the element.  Again, we know this call will succeed
  // due to the successful HTIterator_Get above.
  Verify333(HashTable_Remove(iter->ht, kv.key, keyvalue));
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return true;
}

static void MaybeResize(HashTable *ht) {
  HashTable *newht;
  HashTable tmp;
  HTIterator *it;

  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  newht = HashTable_Allocate(ht->num_buckets * 9);

  // Loop through the old ht copying its elements over into the new one.
  for (it = HTIterator_Allocate(ht);
       HTIterator_IsValid(it);
       HTIterator_Next(it)) {
    HTKeyValue_t item, unused;

    Verify333(HTIterator_Get(it, &item));
    HashTable_Insert(newht, item, &unused);
  }

  // Swap the new table onto the old, then free the old table (tricky!).  We
  // use the "no-op free" because we don't actually want to free the elements;
  // they're owned by the new table.
  tmp = *ht;
  *ht = *newht;
  *newht = tmp;

  // Done!  Clean up our iterator and temporary table.
  HTIterator_Free(it);
  HashTable_Free(newht, &HTNoOpFree);
}
