#include "bf.h"
#include "ht.h"
#include "parser.h"
#include "messages.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <ctype.h>

#define WORD    "([A-Za-z0-9_]+['-]{1})*[A-Za-z0-9_]+" //should account for all
#define OPTIONS "hst:f:"

void display_info(void) {
    printf("SYNOPSIS\n  A word filtering program for the GPRSC.\n  Filters out and reports bad "
           "words parsed from stdin.\n\nUSAGE\n  ./banhammer [-hs] [-t size] [-f size]\nOPTIONS\n  "
           "-h           Program usage and help.\n  -s           Print program statistics.\n  -t "
           "size      Specify hash table size (default: 2^16).\n  -f size      Specify Bloom "
           "filter size (default: 2^20).\n");
}

int main(int argc, char **argv) {
    Node *arrest = bst_create(); //bst for words that are purely thoughtcrime
    Node *correct = bst_create(); //bst for words that need to be corrected
    char old[1024]; //buffer for oldspeak
    char new[1024]; //buffer for newspeak
    int opt = 0;
    bool stats = false;
    FILE *badfile = fopen("badspeak.txt", "r");
    FILE *newfile = fopen("newspeak.txt", "r");
    uint32_t table_size = 65536; //default hash table size is 2^16
    uint32_t filter_size = 1048576; //default bloom filter size is 2^20
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 'h': display_info(); return 0;
        case 's': stats = true; break;
        case 't': table_size = atof(optarg); break;
        case 'f': filter_size = atof(optarg); break;
        }
    }
    HashTable *ht = ht_create(table_size);
    BloomFilter *bf = bf_create(filter_size);
    while (fscanf(badfile, "%s\n", old) != EOF) { //read lines from badspeak.txt
        ht_insert(ht, old, NULL); //insert only badtest
        bf_insert(bf, old);
    }
    while (fscanf(newfile, "%s %s\n", old, new) != EOF) { //read lines from newspeak.txt
        ht_insert(ht, old, new); //oldtest and its newtext translation
        bf_insert(bf, old);
    }
    regex_t re;
    if (regcomp(&re, WORD, REG_EXTENDED)) {
        fprintf(stderr, "Failed to compile regex.\n");
        return 1;
    }
    char *word = NULL;
    uint64_t temp_count = 0;
    //uint64_t leftover = 0;
    while ((word = next_word(stdin, &re)) != NULL) {
        for (uint32_t i = 0; i < strlen(word); i += 1) { //make sure all words are fully lowercase
            word[i] = tolower(word[i]);
        }
        if (bf_probe(bf, word)) { //check to see if the word may have been added to bloom filter
            Node *temp = ht_lookup(ht, word);
            if (temp) { //if something is returnedhh
                temp_count = branches; //this is her because bst_insert will add to the count
                if (temp->newspeak) { //if there is newspeak translation
                    correct = bst_insert(correct, word, temp->newspeak);
                } else { //no newspeak translation
                    arrest = bst_insert(arrest, word, NULL);
                }
                branches -= (branches - temp_count); //reset it to before bst_insert
            }
            //base case, nothing is done
        }
    }
    //output message
    if (arrest && correct && !stats) {
        printf("%s", mixspeak_message);
        bst_print(arrest);
        bst_print(correct);
    } else if (arrest && !stats) {
        printf("%s", badspeak_message);
        bst_print(arrest);
    } else if (correct && !stats) {
        printf("%s", goodspeak_message);
        bst_print(correct);
    }
    if (stats) { //print out stats
        printf("Average BST size: %f\n", ht_avg_bst_size(ht));
        printf("Average BST height: %f\n", ht_avg_bst_height(ht));
        double temp1 = branches;
        double temp2 = lookups;
        printf("Average branches traversed: %f\n", (temp1 / temp2) - 1);
        temp1 = ht_count(ht);
        temp2 = ht_size(ht);
        printf("Hash table load: %f%%\n", 100 * (temp1 / temp2));
        temp1 = bf_count(bf);
        temp2 = bf_size(bf);
        printf("Bloom filter load: %f%%\n", 100 * (temp1 / temp2));
    }
    //free all allocated data
    clear_words();
    regfree(&re);
    HashTable **ht_del = &ht;
    ht_delete(ht_del);
    BloomFilter **bf_del = &bf;
    bf_delete(bf_del);
    Node **bst_del = NULL;
    if (correct) {
        bst_del = &correct;
        bst_delete(bst_del);
    }
    if (arrest) {
        bst_del = &arrest;
        bst_delete(bst_del);
    }
    fclose(badfile);
    fclose(newfile);
    return 0;
}
