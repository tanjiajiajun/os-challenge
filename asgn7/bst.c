/***********
Note:
A lot of this code is either taken from the lecture slides by Prof. Long
and are then later modified to better suit this assignment
************/

#include "bst.h"

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

uint64_t branches;

Node *bst_create(void) {
    return NULL;
}
static int max(int x, int y) {
    return x > y ? x : y;
}

uint32_t bst_height(Node *root) {
    if (root) {
        return 1 + max(bst_height(root->left), bst_height(root->right));
    }
    return 0;
}

uint32_t bst_size(Node *root) { //very similar to bst_height()
    if (root) {
        return 1 + bst_size(root->left) + bst_size(root->right); //counts itself and children
    }
    return 0;
}

Node *bst_find(Node *root, char *oldspeak) {
    branches += 1;
    if (root) {
        if (strcmp(root->oldspeak, oldspeak) > 0) { //if node to be found is less than root
            return bst_find(root->left, oldspeak); //go to the left
        } else if (strcmp(root->oldspeak, oldspeak) < 0) { //if node to be found is more than root
            return bst_find(root->right, oldspeak); //go to the right
        }
        return root; //last case is if they are equal
    }
    return NULL; //if either the root does not exist, or key does not exist, return NULL
}

Node *bst_insert(Node *root, char *oldspeak, char *newspeak) {
    branches += 1;
    if (root) {
        if (strcmp(root->oldspeak, oldspeak) > 0) {
            root->left = bst_insert(root->left, oldspeak, newspeak);
        } else if (strcmp(root->oldspeak, oldspeak) < 0) {
            root->right = bst_insert(root->right, oldspeak, newspeak);
        }
        return root; //if it does exist, just return it back
    }
    return node_create(oldspeak, newspeak); //if it doesnt exist, make it
}
void bst_print(Node *node) {
    if (node) {
        bst_print(node->left);
        node_print(node);
        bst_print(node->right);
    }
}

void bst_delete(Node **root) {
    if (*root) {
        bst_delete(&(*root)->left);
        bst_delete(&(*root)->right);
        node_delete(root);
    }
}
