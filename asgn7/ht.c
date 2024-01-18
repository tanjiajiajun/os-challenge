#include "ht.h"
#include "salts.h"
#include "speck.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

struct HashTable {
    uint64_t salt[2];
    uint32_t size;
    Node **trees;
};

uint64_t lookups;

HashTable *ht_create(uint32_t size) {
    lookups = 0;
    HashTable *ht = (HashTable *) malloc(sizeof(HashTable));
    if (ht) {
        ht->salt[0] = SALT_HASHTABLE_LO;
        ht->salt[1] = SALT_HASHTABLE_HI;
        ht->size = size;
        ht->trees = (Node **) calloc(size, sizeof(Node)); //pointer to array of pointer to nodes
    }
    return ht;
}

void ht_delete(HashTable **ht) {
    if (ht) {
        HashTable *temp = *ht;
        Node **bst_del = NULL; //for deleting the BST's
        for (uint32_t i = 0; i < temp->size; i += 1) {
            if (temp->trees[i]) { //if the trees index is not NULL
                bst_del = &(temp->trees[i]);
                bst_delete(bst_del); //delete that Binary Search Tree
            }
        }
        free(temp->trees); //free the calloc's trees array
        free(temp); //free the entire thing
        *ht = NULL;
    }
}

uint32_t ht_size(HashTable *ht) {
    return ht->size;
}

Node *ht_lookup(HashTable *ht, char *oldspeak) {
    lookups += 1;
    uint32_t i = hash(ht->salt, oldspeak) % ht->size; //find index for salted oldspeak
    return bst_find(ht->trees[i], oldspeak); //if it is found, return it
}

void ht_insert(HashTable *ht, char *oldspeak, char *newspeak) {
    lookups += 1;
    uint32_t i = hash(ht->salt, oldspeak) % ht->size; //get the index for where we hash to
    ht->trees[i]
        = bst_insert(ht->trees[i], oldspeak, newspeak); //insert new node into ht.trees at index i
}

uint32_t ht_count(HashTable *ht) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ht->size; i += 1) {
        if (ht->trees[i]) { //non_NULL BSTs
            count += 1;
        }
    }
    return count;
}

double ht_avg_bst_size(HashTable *ht) { //this and height are similar
    uint32_t count = 0; //how many non-NULL nodes
    double size = 0.0; //counts total size
    for (uint32_t i = 0; i < ht->size; i += 1) {
        if (ht->trees[i]) {
            count += 1;
            size += bst_size(ht->trees[i]);
        }
    }
    return size / count;
}

double ht_avg_bst_height(HashTable *ht) {
    uint32_t count = 0;
    double height = 0.0;
    for (uint32_t i = 0; i < ht->size; i += 1) {
        if (ht->trees[i]) {
            count += 1;
            height += bst_height(ht->trees[i]);
        }
    }
    return height / count;
}

void ht_print(HashTable *ht) {
    for (uint32_t i = 0; i < ht->size; i += 1) {
        if (ht->trees[i]) {
            printf("BST tree at %" PRIu32 ":\n", i);
            bst_print(ht->trees[i]);
        }
    }
}
