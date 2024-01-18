#include "node.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Node *node_create(char *oldspeak, char *newspeak) {
    Node *n = (Node *) malloc(sizeof(Node));
    if (n) {
        if (newspeak) { //if newspeak is not NULL;
            char *new = strdup(newspeak);
            //printf("%s\n", newspeak);
            //printf("%s\n", new);
            n->newspeak = new; //this is here so we can strcmp better
            //free(newspeak); //freed so we can immediately make a new node
        } else { //this is only here for valgrind
            //char *new = strdup("NULL"); //is never actually used
            n->newspeak = NULL;
            //free(newspeak);
        }
        char *old = strdup(oldspeak);
        n->oldspeak = old;
        n->left = NULL;
        n->right = NULL;
        //free(oldspeak);
    }
    return n;
}

void node_delete(Node **n) {
    Node *temp = *n;
    free(temp->oldspeak); //free the strdup'd oldspeak string
    free(temp->newspeak);
    free(temp);
    *n = NULL;
}

void node_print(Node *n) {
    if (n->newspeak) { //if newspeak is not "NULL"
        printf("%s -> %s\n", n->oldspeak, n->newspeak); //print both
    } else {
        printf("%s\n", n->oldspeak); //print only newspeak
    }
}
