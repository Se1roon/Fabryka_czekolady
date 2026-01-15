#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdio.h>

#define MAGAZINE_CAPACITY 200
#define SAVE_FILE         "./.magazyn"
#define INIT_WORK         true
#define USE_USLEEP        true

// Macro for calculating index inside the magazine_racks array (it is treated as 2D but is 1D)
#define INDEX_2D(row, col, mag) (((row) * (mag->columns)) + (col))

typedef enum {
    A,
    B,
    C,
    D
} E_Component;

typedef struct {
    int columns;
    int rows;
    size_t space_left;
    char magazine_racks[MAGAZINE_CAPACITY];
} Magazine;


void calculate_cols_rows(int *out_columns, int *out_rows);
void find_free_slots(Magazine *magazine, int space, int *out_row, int *out_column);

#endif
