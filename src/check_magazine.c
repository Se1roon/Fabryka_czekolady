#include "../include/common.h"
#include <stdio.h>

int main() {
    FILE *f = fopen("magazine.bin", "rb");
    if (!f) {
        printf("Błąd: Nie można otworzyć pliku magazine.bin (czy symulacja została uruchomiona?)\n");
        return 1;
    }

    Magazine mag;
    if (fread(&mag, sizeof(Magazine), 1, f) != 1) {
        printf("Błąd: Nie udało się odczytać struktury Magazine z pliku.\n");
        fclose(f);
        return 1;
    }
    fclose(f);

    int count_A = 0;
    int count_B = 0;
    int count_C = 0;
    int count_D = 0;

    // Zliczanie wystąpień bajtów w buforze
    for (int i = 0; i < MAGAZINE_CAPACITY; i++) {
        if (mag.buffer[i] == 'A')
            count_A++;
        else if (mag.buffer[i] == 'B')
            count_B++;
        else if (mag.buffer[i] == 'C')
            count_C++;
        else if (mag.buffer[i] == 'D')
            count_D++;
    }

    int items_C = count_C / 2;
    int items_D = count_D / 3;

    printf("===== STAN MAGAZYNU (magazine.bin) =====\n");
    printf("Pojemność całkowita (bajty): %d\n", MAGAZINE_CAPACITY);
    printf("----------------------------------------\n");
    printf("Składniki A: %d\n", count_A);
    printf("Składniki B: %d\n", count_B);
    printf("Składniki C: %d (zajęte bajty: %d)\n", items_C, count_C);
    printf("Składniki D: %d (zajęte bajty: %d)\n", items_D, count_D);
    printf("----------------------------------------\n");
    printf("Wyprodukowano X: %d\n", mag.typeX_produced);
    printf("Wyprodukowano Y: %d\n", mag.typeY_produced);
    printf("========================================\n");

    return 0;
}
