#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

// #define PRINT_TO_STDOUT

// Capacity of magazine
#define MAG_CAPACITY 10000

typedef enum {
    A,
    B,
    C,
    D
} component_type;

typedef struct {
    size_t capacity;
    size_t a_count;
    size_t b_count;
    size_t c_count;
    size_t d_count;
    size_t type1_produced;
    size_t type2_produced;
} SHM_DATA;


#endif