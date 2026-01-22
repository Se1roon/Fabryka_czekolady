# Projekt Systemy Operacyjne - Fabryka Czekolady (temat 3)

| Parametr | Wartość |
|----------|---------|
| Operating System | CachyOS (Arch Linux) |
| Kernel | Linux 6.18.3-2-cachyos |
| Architecture | x86-64 |
| Kompilator | clang 21.1.6 |

**Wymagania**
- `clang` - kompilator
- `make` - narzędzie do automatyzacji procesu kompilacji wielu modułów

**Uruchomienie symulacji**
```
mkdir bin
make
./bin/dyrektor
```

**Modyfikowanie działania programu**

Możliwe jest modyfikowanie ilości czekolady jaką fabryka ma wygenerować.
Aby to zrobić należy zmienić wartości zmiennych w pliku **include/common.h** (linie 35,36)
```
// Change these
#define X_TYPE_TO_PRODUCE 100
#define Y_TYPE_TO_PRODUCE 800
```
## Opis projektu
Projekt ma na celu zasymulowanie działania fabryki czekolady typu X i Y. Fabryka składa się z *Dyrektora* zarządzającego całym zakładem, *Stanowisk X/Y* produkujących czekoladę odpowiedniego typu, oraz *Dostawców* każdego ze składników (A, B, C, D).

Czekolada typu **X** składa się ze składników:
- A
- B
- C

Czekolada typu **Y** składa się ze składników:
- A
- B
- D

Przy czym zakładamy, że każdy składnik zajmuje określoną ilość miejsca w magazynie.

- A - 1 jednostka
- B - 1 jednostka
- C - 2 jednostki
- D - 3 jednostki

## Architektura Programu
| Proces | Plik źródłowy | Opis |
|--------|---------------|------|
| **dyrektor** | `src/dyrektor.c` | Proces główny - Dyrektor |
| **dostawca** | `src/dostawca.c` | Dostawca określonego składnika |
| **pracownik** | `src/pracownik.c` | Pracownik, produkujący określony typ czekolady |
| **logger** | `src/logger.c` | Proces odpowiedzialny za logowanie działań Fabryki |

**Dyrektor**
- Wczytuje zapisany stan magazynu jeżeli taki istnieje.
- Uruchamia fabrykę
- Posiada dodatkowy wątek, upewniający się, że fabryka zakończyła pracę.
- Po otrzymaniu powiadomienia od Pracownika, zatrzymuje dostawę określonego składnika/ów

**Dostawca**
- Posiada określony typ składnika jaki dostarcza
- Sprawdza czy może wejść do magazynu
    - Jeśli TAK, szuka wolnego miejsca na swój składnik (jeżeli go nie znajdzie, wychodzi)
    - Jeśli NIE, czeka przed bramą

**Pracownik**
- Posiada określony typ czekolady jaki ma produkować
- Sprawdza czy magazyn jest wolny
    - Jeśli TAK, stara się znaleźć potrzebne mu składniki
        - Jeżeli mu się uda, produkuje czekoladę, dodatkowo sprawdzając czy wyprodukował już wystarczająco
            - Jeśli TAK, wysyła powiadomienie do dyrektora, z prośbą o zaprzestanie dostaw specyficznego dla jego rodzaju czekolady składnika.
            - Jeśli NIE, produkuje dalej
        - Jeżeli mu się nie uda, opuszcza magazyn
    - Jeśli NIE, czeka

**Logger**
- Każdy proces wysyła do niego akcję, którą właśnie wykonał
- Wyświetla to co zostanie do niego przesłane

## Komunikacja IPC
**Pamięć Dzielona - serce Fabryki, magazyn**
```
typedef struct {
    char buffer[MAGAZINE_CAPACITY];
    int typeX_produced;
    int typeY_produced;
} Magazine;
```
[Definicja w include/common.h](include/common.h:61)

**Semafor**
| Wartość początkowa | Zastosowanie |
|--------------------|--------------|
| 1 | Synchronizacja dostępu do magazynu |

[Użycie w procesie Pracownik](src/pracownik.c:63)

[Użycie w procesie Dostawca](src/dostawca.c:68)

**Kolejka komunikatów**

Używana jest w procesie logowania działań Fabryki. Posiada tylko jeden typ wiadomości:
```
typedef struct {
    long mtype;
    char text[256];
} LogMsg;

```
[Definicja w include/common.h](include/common.h:67)

## Obsługa błędów
Wszystkie kluczowe funkcje systemowe są sprawdzane pod kątem ewentualnych błędów. Gdy takowy zostanie wykryty następuje wysłanie wiadomości na kolejke komunikatów, oraz (jeśli konieczne) zakończenie działania Fabryki (wyczyszczenie pamięci i procesów potomnych)

Wiadomość jaka ma zostać wyświetlona jest tworzona przy użyciu funkcji `fprintf()` oraz `strerror()`.

## Działanie Symulacji

| Stała | Wartość |  Opis |
|-------|---------|-------|
| `IPC_KEY_ID` | 7616 | ID Klucza używanego przy komunikacji IPC |
| `LOG_FILE` | simulation.log | Ścieżka do pliku zawierającego raport z symulacji |
| `A_COMPONENT_SIZE` | 1 | Liczba bajtów zajmowanych przez składnik A |
| `B_COMPONENT_SIZE` | 1 | Liczba bajtów zajmowanych przez składnik B |
| `C_COMPONENT_SIZE` | 2 | Liczba bajtów zajmowanych przez składnik C |
| `D_COMPONENT_SIZE` | 3 | Liczba bajtów zajmowanych przez składnik D |
| `X_TYPE_SIZE` | A + B + C | Łączna liczba bajtów potrzebna na składniki do czekolady typu X |
| `Y_TYPE_SIZE` | A + B + D | Łączna liczba bajtów potrzebna na składniki do czekolady typu Y |
| `X_TYPE_TO_PRODUCE` | Modyfikowalne | Ilość czekolady typu X do wyprodukowania |
| `Y_TYPE_TO_PRODUCE` | Modyfikowalne | Ilość czekolady typu Y do wyprodukowania |
| `MAGAZINE_CAPACITY` | ilośćX * sizeX + ilośćY * sizeY | Ilość bajtów jaką powinien zajmować magazyn |
| `COMPONENT_SPACE` | sizeMagazyn / (A + B + C + D) | Ilość bajtów przypadająca na składniki A i B (2x dla C, 3x dla D) |
| `IsA` | 0 | Indeks startu segmentu składników A w magazynie |
| `IkA` | `COMPONENT_SPACE` - 1 | Indeks końca segmentu składników A w magazynie |
| `IsB` | `IkA` + 1 | Indeks startu segmentu składników B w magazynie |
| `IkB` | `IsB` + `COMPONENT_SPACE` - 1 | Indeks końca segmentu składników B w magazynie |
| `IsC` | `IkB` + 1 | Indeks startu segmentu składników C w magazynie |
| `IkC` | `IsC` + (2 * `COMPONENT_SPACE` - 1) | Indeks końca segmentu składników C w magazynie |
| `IsD` | `IkC` + 1 | Indeks startu segmentu składników D w magazynie |
| `IkD` | `IsD` + (3 * `COMPONENT_SPACE` - 1) | Indeks końca segmentu składników D w magazynie |

**Inicjacja IPC & Odczyt stany magazynu**

*Dyrektor* 
- przypisuje funkcję `signal_handler()` do sygnałów `SIGINT`, `SIGUSR1` oraz `SIGUSR`.
- Inicjuje mechanizmy IPC (tworzy klucz, pamięć dzieloną, semafor, oraz kolejkę komunikatów). 
- Inicjuje semafor wartością 1
- Przywraca stan magazynu, jeżeli takowy zapis istnieje. 
- Uruchamia wątek śledzący zmiany stanów procesów potomnych.
- Uruchamia procesy dostawców oraz pracowników.

## Testy
**Test 1 - Test obciążeniowy (X = 100000, Y = 100000)**
- Test ma za zadanie sprawdzić czy przy dużej liczbie czekolady do wyprodukowania, program się nie zawiesza i faktycznie produkuje odpowiednią liczbę czekolad



