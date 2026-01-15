#include <math.h>

#include "../include/magazine.h"


void calculate_cols_rows(int *out_columns, int *out_rows) {
    int start_point = (int)floor(sqrt(MAGAZINE_CAPACITY));
    for (int row = start_point; row >= 1; row--) {
        if (MAGAZINE_CAPACITY % row == 0) {
            *out_rows = row;
            *out_columns = MAGAZINE_CAPACITY / row;

            return;
        }
    }

    return;
}

void find_free_slots(Magazine *magazine, int space, int *out_row, int *out_column) {
    int k = 0;
    for (int row = 0; row < magazine->rows; row++) {
        for (int col = 0; col < magazine->columns; col++) {
            if (magazine->magazine_racks[INDEX_2D(row, col, magazine)] == 0 && k == 0) {
                *out_column = col;
                *out_row = row;
                k++;
            } else if (k > 0) {
                k++;
            } else {
                k = 0;
                *out_column = -1;
                *out_row = -1;
            }

            if (k == space)
                return;
        }
    }

    *out_column = -1;
    *out_row = -1;

    return;
}
