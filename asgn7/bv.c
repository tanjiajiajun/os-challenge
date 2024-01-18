#include "bv.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

struct BitVector {
    uint32_t length;
    uint8_t *vector;
};

BitVector *bv_create(uint32_t length) {
    BitVector *bv = (BitVector *) malloc(sizeof(BitVector));
    if (bv) {
        uint32_t temp = length / 8;
        if (length % 8 > 0) {
            temp += 1; //this is for the ceiling function
        }
        bv->length = length;
        bv->vector = (uint8_t *) calloc(temp, sizeof(uint8_t)); //set bytes
    }
    return bv;
}

void bv_delete(BitVector **bv) {
    if (*bv && (*bv)->vector) {
        free((*bv)->vector);
        free(*bv);
        *bv = NULL;
    }
}

uint32_t bv_length(BitVector *bv) {
    return bv->length;
}

bool bv_set_bit(BitVector *bv, uint32_t i) {
    if (i > bv->length / 8 * 8) { //divided by 8 for floor
        return false;
    }
    bv->vector[i / 8] |= (1 << (i % 8));
    return true;
}
bool bv_clr_bit(BitVector *bv, uint32_t i) {
    if (i > bv->length / 8 * 8) { //divided by 8 for floor
        return false;
    }
    bv->vector[i / 8] &= ~(1 << (i % 8));
    return true;
}

bool bv_get_bit(BitVector *bv, uint32_t i) {
    if (i > bv->length / 8 * 8) {
        return false;
    }
    uint8_t x = 0;
    x = 1 << (i % 8);
    x &= (bv->vector[i / 8]);
    if (x > 0) {
        return true;
    } else {
        return false;
    }
}

void bv_print(BitVector *bv) {
    for (uint32_t i = 0; i < bv->length; i += 1) {
        if (bv->vector[i / 8] > 0) {
            if (i % 8 == 0 && i != 0) {
                printf(" %" PRIu32 "-%" PRIu32 "\n", i - 8, i - 1);
            }
            if (bv_get_bit(bv, i)) {
                printf("1");
            } else {
                printf("0");
            }
        }
    }
    printf(" %" PRIu32 "-%" PRIu32 "\n", bv->length - 8, bv->length - 1);
}
