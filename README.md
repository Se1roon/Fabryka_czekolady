# Projekt Systemy Operacyjne - Fabryka Czekolady (temat 3)

| Parametr | Wartość |
|----------|---------|
| Operating System | CachyOS (Arch Linux) |
| Kernel | Linux 6.18.6-2-cachyos |
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
Aby to zrobić należy zmienić wartości zmiennych w pliku [common.h](include/common.h#L35)
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

## Architektura i opis działania programu
| Proces | Plik źródłowy | Opis |
|--------|---------------|------|
| **dyrektor** | `src/dyrektor.c` | Proces główny - Dyrektor |
| **dostawca** | `src/dostawca.c` | Dostawca określonego składnika |
| **pracownik** | `src/pracownik.c` | Pracownik, produkujący określony typ czekolady |
| **logger** | `src/logger.c` | Proces odpowiedzialny za logowanie działań Fabryki |

**Dyrektor**

Program rozpoczyna się od sprawdzenia wartości ustawianych przez użytkownika (`X_TYPE_TO_PRODUCE`, `Y_TYPE_TO_PRODUCE`, `MAGAZINE_CAPACITY`). Następnie dyrektor inicjuje struktury komunikacji IPC. Kolejnym krokiem w pracy dyrektora jest przywrócenie stanu magazynu jeżeli takowy zapis istnieje, stworzenie procesów dostawców i pracowników, oraz stworzenie wątki odpowiedzialnego za zbieranie martwych procesów.

Sygnały, na które reaguje dyrektor:
- `SIGINT`: Zatrzymuje pracę fabryki. Zapisuje stan magazynu, zabija dzieci, zwalnia struktury IPC.
- `SIGTERM`: Zmienia stan magazynu. Magazyn może być nieaktywny bądź aktywny, sygnał ten służy do zmiany jego stanu.
- `SIGUSR1`: Wstrzymuje dostawy niepotrzebnych składników. Sygnał ten jest wysyłany przez pracowników, którzy skończyli swoją pracę.
- `SIGUSR2`: Oznacza atak terrorystyczny, zatrzymuje pracę fabryki. Sygnał wysyłany przez procesy potomne, kiedy otrzymają zewnętrzny sygnał `SIGTERM`.

**Dostawcy**

Każdy proces dostawcy jako argument otrzymuje typ składnika jaki ma dostarczać. Następnie na podstawie tego składnika, oblicza przestrzeń magazynu przeznaczoną dla niego. Potem, sprawdza czy w tej przestrzeni jest w ogóle miejsce na składnik, jeżeli jest to czeka aż będzie miał możliwość wejścia do magazynu (obie te operacje są operacjami semaforowymi).

Sygnały, na które reaguje dostawca:
- `SIGINT`: Kończy pracę dostawcy. Jeżeli jakiś dostawca jest już w drodze, bądź czeka przy magazynie to jest on jeszcze obsługiwany. Za to nie nadchodzą juz kolejne dostawy.
- `SIGTERM`: Atak terrorystyczny, powiadamia dyrektora.

**Pracownicy**

Pracownik, podobnie jak dostawca, przy wywołaniu otrzymuje typ czekolady jaki ma produkować. Następnie sprawdza czy zostały dostarczone składniki potrzebne mu do produkcji, jeżeli tak to czeka na dostęp do magazynu. Gdy pracownik wykryje, że zostało wyprodukowane już wystarczająco czekolady, to wyśle sygnał do dyrektora po czym zakończy pracę.

Sygnały, na które reaguje pracownik:
- `SIGINT`: Kończy pracę pracownika. Podobnie jak z dostawcą, jak coś już jest w trakcie produkcji, to się wyprodukuje.
- `SIGTERM`: Atak terrorystyczny, powiadamia dyrektora.

**Logger**

Każdy z poprzednio opisanych procesów, wysyła swoje komunikaty na kolejke komunikatów. Proces loggera odbiera te komunikaty i wypisuje je zarówno na *STDOUT* jak i do pliku *simulation.log*

## Dodatkowe programy
| Program | Plik źródłowy | Opis |
|---------|---------------|------|
| **check_deliveries** | `check_deliveries.sh` | Skrypt sprawdzający liczbę dostaw (przeszukuje plik *simulation.log*)|
| **check_magazine** | `src/check_magazine.c` | Program sprawdzający liczbę składników w magazynie |

## Stałe używane w programie

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
| `MAGAZINE_CAPACITY` | Stała wartość (np. 1000) | Ilość bajtów przeznaczona na magazyn |
| `COMPONENT_SPACE` | sizeMagazyn / (A + B + C + D) | Ilość bajtów przypadająca na składniki A i B (2x dla C, 3x dla D) |
| `IsA` | 0 | Indeks startu segmentu składników A w magazynie |
| `IkA` | `COMPONENT_SPACE` - 1 | Indeks końca segmentu składników A w magazynie |
| `IsB` | `IkA` + 1 | Indeks startu segmentu składników B w magazynie |
| `IkB` | `IsB` + `COMPONENT_SPACE` - 1 | Indeks końca segmentu składników B w magazynie |
| `IsC` | `IkB` + 1 | Indeks startu segmentu składników C w magazynie |
| `IkC` | `IsC` + (2 * `COMPONENT_SPACE` - 1) | Indeks końca segmentu składników C w magazynie |
| `IsD` | `IkC` + 1 | Indeks startu segmentu składników D w magazynie |
| `IkD` | `IsD` + (3 * `COMPONENT_SPACE` - 1) | Indeks końca segmentu składników D w magazynie |

## Komunikacja IPC
**Pamięć Dzielona - Magazyn**
```
typedef struct {
    char buffer[MAGAZINE_CAPACITY];
    int typeX_produced;
    int typeY_produced;
} Magazine;
```
[Definicja w include/common.h](include/common.h#L62)

- **buffer**: Miejsce przechowywujące składniki, podzielone na odpowiednie segmenty
- **typeX_produced**: Ilość czekolady typu X jaka została wyprodukowana
- **typeY_produced**: Ilość czekolady typu Y jaka została wyprodukowana

**Semafory**
| Nazwa | Wartość początkowa | Zastosowanie |
|-------|--------------------|--------------|
| `SEM_MAGAZINE` | 1 | Dostęp do magazynu |
| `SEM_EMPTY_A`  | `IkA` - `IsA` + 1 | Liczba miejsc dostępnych w segmencie składnika A |
| `SEM_FULL_A`   | 0 | Liczba składników A dostarczonych (aktualnie w magazynie) |
| `SEM_EMPTY_B`  | `IkB` - `IsB` + 1 | Liczba miejsc dostępnych w segmencie składnika B |
| `SEM_FULL_B`   | 0 | Liczba składników B dostarczonych (aktualnie w magazynie) |
| `SEM_EMPTY_C`  | `IkC` - `IsC` + 1 | Liczba miejsc dostępnych w segmencie składnika C |
| `SEM_FULL_C`   | 0 | Liczba składników C dostarczonych (aktualnie w magazynie) |
| `SEM_EMPTY_D`  | `IkD` - `IsD` + 1 | Liczba miejsc dostępnych w segmencie składnika D |
| `SEM_FULL_D`   | 0 | Liczba składników D dostarczonych (aktualnie w magazynie) |

- Zmniejszenie wartości semaforów `SEM_EMPTY_x` następuje kiedy dostawca dostarcza składnik
- Zwiększenie wartości semaforów `SEM_EMPTY_x` następuje kiedy pracownik wyprodukował czekoladę 
- Zmniejszenie wartości semaforów `SEM_FULL_x` następuje gdy pracownik produkuje czekoladę 
- Zwiększenie wartości semaforów `SEM_FULL_x` następuje gdy dostawca dostarczył składnik

[Użycie w procesie Pracownik](src/pracownik.c#L74)

[Użycie w procesie Dostawca](src/dostawca.c#L75)

**Kolejka komunikatów**

Używana jest w procesie logowania działań Fabryki. Posiada tylko jeden typ wiadomości:
```
typedef struct {
    long mtype;
    char text[256];
} LogMsg;

```
[Definicja w include/common.h](include/common.h#L101)

[Użycie w funkcji pomocniczej `send_log()`](include/common.h#L117)

## Obsługa błędów
Wszystkie kluczowe funkcje systemowe są sprawdzane pod kątem ewentualnych błędów. Gdy takowy zostanie wykryty następuje wysłanie wiadomości na kolejke komunikatów, oraz (jeśli konieczne) zakończenie działania Fabryki (wyczyszczenie pamięci i procesów potomnych).

Wiadomość jaka ma zostać wyświetlona jest tworzona przy użyciu funkcji `fprintf()` oraz `strerror()`.

## Testy

Wszystkie testy przeprowadzane są przy założonej pojemności magazynu `MAGAZINE_CAPACITY = 1000`.

**Test 1 - Normalna praca fabryki, zwalnianie struktur IPC**
- Test ma na celu sprawdzić czy program działa poprawnie podczas zwyczajnych warunków

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 10000 |
| `Y_TO_PRODUCE` | 10000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor
[Wed Jan 28 19:24:30 2026] [Director] IPC Resources created!
[Wed Jan 28 19:24:30 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:24:30 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:24:30 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:24:30 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:24:30 2026] [Director] Spawning child processes...
[Wed Jan 28 19:24:30 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:24:30 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:24:30 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:24:30 2026] [Worker: Y] Worker Y started
[Wed Jan 28 19:24:30 2026] [Worker: X] Worker X started
[Wed Jan 28 19:24:30 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 19:24:30 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:24:30 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:24:30 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:24:30 2026] [Supplier: B] Starting deliveries of B!
.
.
[Wed Jan 28 19:24:31 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:24:31 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:24:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:24:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101747 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101751 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Worker: Y] Terminating
[Wed Jan 28 19:24:31 2026] [Director] Stopping A deliveries!
[Wed Jan 28 19:24:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:24:31 2026] [Director] Stopping B deliveries!
[Wed Jan 28 19:24:31 2026] [Supplier: A] Terminating
[Wed Jan 28 19:24:31 2026] [Director] Stopping D deliveries!
[Wed Jan 28 19:24:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:24:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:24:31 2026] [Supplier: B] Terminating
[Wed Jan 28 19:24:31 2026] [Supplier: D] Terminating
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101748 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101749 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101750 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Process Manager] Successfully collected process 101752 (exit code 0)!
[Wed Jan 28 19:24:31 2026] [Director] FACTORY FINISHED WORK!
[Wed Jan 28 19:24:31 2026] [Director] Saved magazine state!
[Director] Child process 101746 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 10000
	Y = 10000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ipcs

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      
0xce320210 1          root       666        1024       1                       

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     

~/Documents/Studia/SO/Fabryka_czekolady main
❯ ./check_deliveries.sh
A: 20002
B: 20002
C: 10142
D: 10144

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 2
Składniki B: 2
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 10000
Wyprodukowano Y: 10000
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

**Test 2 - Różne ilości czekolady do wyprodukowania & Procesy zombie**
- Test sprawdza czy kiedy jeden z pracowników skończy pracę, prawdidłowo informowany jest dyrektor oraz czy przerywane są dostawy nie potrzebnych dłużej składników. Sytuacja ta często skutkowała pojawianiem się procesów *zombie*, więc test dodatkowo sprawdza pracę wątku odpowiedzialnego za zbieranie procesów.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 10000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
[Wed Jan 28 19:29:46 2026] [Director] IPC Resources created!
[Wed Jan 28 19:29:46 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:29:46 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:29:46 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:29:46 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:29:46 2026] [Director] Spawning child processes...
[Wed Jan 28 19:29:46 2026] [Worker: X] Worker X started
[Wed Jan 28 19:29:46 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:29:46 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:29:46 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:29:46 2026] [Worker: Y] Worker Y started
[Wed Jan 28 19:29:46 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 19:29:46 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:46 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:46 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:46 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:46 2026] [Supplier: A] Delivered A component!
.
.

(z drugiego terminala, w momencie jak Y skończyło pracę):
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ps a
    PID TTY      STAT   TIME COMMAND
    852 tty2     S<sl+   0:01 /usr/lib/Xorg -nolisten tcp -background none -seat seat0 vt2 -auth /run/sddm/xauth_O
   1156 tty1     S<s+   0:00 /usr/bin/sh /usr/lib/uwsm/signal-handler.sh wayland-session-envelope@hyprland.desktop
   1207 tty1     S<+    0:00 systemctl --user start --wait wayland-session-envelope@hyprland.desktop.target
   1828 pts/0    S<s    0:08 /bin/fish
  14251 pts/1    S<sl   0:07 /bin/fish
  82377 pts/3    S<s    0:01 /bin/fish
 101557 pts/3    S<+    0:00 nvim README.md
 103094 pts/0    S<l+   0:00 ./bin/dyrektor
 103095 pts/0    R<+    0:01 ./bin/logger
 103096 pts/0    S<+    0:00 ./bin/pracownik X
 103098 pts/0    R<+    0:00 ./bin/dostawca A
 103099 pts/0    S<+    0:00 ./bin/dostawca B
 103100 pts/0    S<+    0:00 ./bin/dostawca C
 103112 pts/1    R<+    0:00 ps a

 .
 .
[Wed Jan 28 19:29:49 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:49 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:29:49 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:29:49 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:29:49 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:29:49 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Wed Jan 28 19:29:49 2026] [Worker: X] Terminating
[Wed Jan 28 19:29:49 2026] [Supplier: C] Not enough space for component C!
[Wed Jan 28 19:29:49 2026] [Director] Stopping A deliveries!
[Wed Jan 28 19:29:49 2026] [Director] Stopping B deliveries!
[Wed Jan 28 19:29:49 2026] [Director] Stopping C deliveries!
[Wed Jan 28 19:29:49 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:29:49 2026] [Supplier: A] Terminating
[Wed Jan 28 19:29:49 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:29:49 2026] [Supplier: C] Terminating
[Wed Jan 28 19:29:49 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:29:49 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:29:49 2026] [Supplier: B] Terminating
[Wed Jan 28 19:29:49 2026] [Process Manager] Successfully collected process 103096 (exit code 0)!
[Wed Jan 28 19:29:49 2026] [Process Manager] Successfully collected process 103098 (exit code 0)!
[Wed Jan 28 19:29:49 2026] [Process Manager] Successfully collected process 103100 (exit code 0)!
[Wed Jan 28 19:29:49 2026] [Process Manager] Successfully collected process 103099 (exit code 0)!
[Wed Jan 28 19:29:49 2026] [Director] FACTORY FINISHED WORK!
[Director] Child process 103095 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 10000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 110002
B: 110142
C: 100142
D: 10144

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 2
Składniki B: 142
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 100000
Wyprodukowano Y: 10000
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

**Test 3 - Sygnał CTRL+C**
- Test sprawdza czy po otrzymaniu sygnału *SIGINT* dyrektor poprawnie to wyłapuje i kończy pracę fabryki.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 200000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
[Wed Jan 28 19:33:34 2026] [Director] IPC Resources created!
[Wed Jan 28 19:33:34 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:33:34 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:33:34 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:33:34 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:33:34 2026] [Director] Spawning child processes...
[Wed Jan 28 19:33:34 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:33:34 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:33:34 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:33:34 2026] [Worker: X] Worker X started
[Wed Jan 28 19:33:34 2026] [Worker: Y] Worker Y started
[Wed Jan 28 19:33:34 2026] [Supplier: A] Starting deliveries of A!
.
.
^C[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:33:36 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:33:36 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:33:36 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:33:36 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:33:36 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:33:36 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:33:36 2026] [Supplier: A] Terminating
[Wed Jan 28 19:33:36 2026] [Supplier: B] Terminating
[Wed Jan 28 19:33:36 2026] [Supplier: D] Terminating
[Wed Jan 28 19:33:36 2026] [Worker] Received SIGINT - Terminating
[Wed Jan 28 19:33:36 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:33:36 2026] [Worker] Received SIGINT - Terminating
[Wed Jan 28 19:33:36 2026] [Worker: X] Terminating
[Wed Jan 28 19:33:36 2026] [Director] Received SIGINT | Shutting down
[Wed Jan 28 19:33:36 2026] [Worker: Y] Terminating
[Wed Jan 28 19:33:36 2026] [Supplier: C] Terminating
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104093 (exit code 0)!
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104094 (exit code 0)!
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104096 (exit code 0)!
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104091 (exit code 0)!
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104092 (exit code 0)!
[Wed Jan 28 19:33:36 2026] [Process Manager] Successfully collected process 104095 (exit code 0)!
[Director] Amount of chocolate produced:
	X = 28518
	Y = 28516
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 57035
B: 57035
C: 28660
D: 28660

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 1
Składniki B: 1
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 28518
Wyprodukowano Y: 28516
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```
- 28660 - 28518 = 142 (składniki C się zgadzają)
- 28660 - 28516 = 144 (składniki D się zgadzają)
- 57035 - 28518 - 28516 = 1 (składniki A się zgadzają)
- 57035 - 28518 - 28516 = 1 (składniki B się zgadzają)

**Test 4 - Sygnał SIGTERM, zmienianie dostępności magazynu**
- Test sprawdza czy po otrzymaniu sygnału SIGTERM dyrektor poprawnie steruje pracą magazynu. Sygnał ten działa jak przełączenik, który wstrzymuje/wznawia pracę magazynu.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 200000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor
[Wed Jan 28 19:47:44 2026] [Director] IPC Resources created!
[Wed Jan 28 19:47:44 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:47:44 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:47:44 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:47:44 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:47:44 2026] [Director] Spawning child processes...
[Wed Jan 28 19:47:44 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:47:44 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:47:44 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:47:44 2026] [Worker: Y] Worker Y started
[Wed Jan 28 19:47:44 2026] [Worker: X] Worker X started
[Wed Jan 28 19:47:44 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 19:47:44 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:44 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:44 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:44 2026] [Supplier: B] Starting deliveries of B!
.
.

(z drugiego terminala):
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ kill <pid dyrektora>

[Wed Jan 28 19:47:48 2026] [Director] Received SIGTERM... Stopping Magazine!
[Wed Jan 28 19:47:48 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:48 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:47:48 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:47:48 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:47:48 2026] [Director] Magazine Stopped!

(z drugiego terminala):
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ kill <pid dyrektora>
[Wed Jan 28 19:47:57 2026] [Director] Received SIGTERM... Activating Magazine!
[Wed Jan 28 19:47:57 2026] [Director] Magazine Activated!
[Wed Jan 28 19:47:57 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:57 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:47:57 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:47:57 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:47:57 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:57 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:47:57 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:47:57 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:47:57 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:47:57 2026] [Worker: X] PRODUCED chocolate type X!
.
.
[Wed Jan 28 19:48:02 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:48:02 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:48:02 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:48:02 2026] [Director] Stopping B deliveries!
[Wed Jan 28 19:48:02 2026] [Director] Stopping D deliveries!
[Wed Jan 28 19:48:02 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:48:02 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:48:02 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:48:02 2026] [Supplier: D] Terminating
[Wed Jan 28 19:48:02 2026] [Supplier: A] Terminating
[Wed Jan 28 19:48:02 2026] [Supplier: B] Terminating
[Wed Jan 28 19:48:02 2026] [Process Manager] Successfully collected process 107254 (exit code 0)!
[Wed Jan 28 19:48:02 2026] [Process Manager] Successfully collected process 107258 (exit code 0)!
[Wed Jan 28 19:48:02 2026] [Process Manager] Successfully collected process 107255 (exit code 0)!
[Wed Jan 28 19:48:02 2026] [Process Manager] Successfully collected process 107256 (exit code 0)!
[Wed Jan 28 19:48:02 2026] [Director] FACTORY FINISHED WORK!
[Wed Jan 28 19:48:02 2026] [Director] Saved magazine state!
[Director] Child process 107252 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 200000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 300003
B: 300005
C: 100142
D: 200144

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 3
Składniki B: 5
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 100000
Wyprodukowano Y: 200000
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

**Test 5 - Atak terrorystyczny. Niespodziewane zakończenie pracy jednego z procesów potomnych**
- Gdy podczas symulacji jeden z procesów potomnych (pracownik, dostawca, logger) otrzyma sygnał SIGTERM nie pochodzący od dyrektora uznawane jest to za atak terrorystyczny. W takim przypadku proces, który otrzymał takowy sygnał powiadamia o sytuacji dyrektora, który to z kolei zamyka fabrykę.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 200000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
[Wed Jan 28 19:39:27 2026] [Director] IPC Resources created!
[Wed Jan 28 19:39:27 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:39:27 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:39:27 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:39:27 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:39:27 2026] [Director] Spawning child processes...
[Wed Jan 28 19:39:27 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:39:27 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:39:27 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:27 2026] [Worker: Y] Worker Y started
.
.

(z drugiego terminala):
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ kill <pid pracownika/dostawcy/loggera>

.
.

[Wed Jan 28 19:39:31 2026] [Worker] Something weird is happening... Notifying Director  <--
[Wed Jan 28 19:39:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:39:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:39:31 2026] [Director] Received SIGUSR2... A terrorist attack detected!  <--
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:39:31 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:39:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:39:31 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:39:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 19:39:31 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 19:39:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:39:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:39:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:39:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:39:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:39:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:39:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:39:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:39:31 2026] [Supplier: D] Terminating
[Wed Jan 28 19:39:31 2026] [Worker] Received SIGINT - Terminating
[Wed Jan 28 19:39:31 2026] [Supplier: B] Terminating
[Wed Jan 28 19:39:31 2026] [Supplier: C] Terminating
[Wed Jan 28 19:39:31 2026] [Worker: Y] Terminating
[Wed Jan 28 19:39:31 2026] [Supplier: A] Terminating
[Wed Jan 28 19:39:31 2026] [Worker] Received SIGINT - Terminating
[Wed Jan 28 19:39:31 2026] [Worker: X] Terminating
[Wed Jan 28 19:39:31 2026] [Director] Child process 105175 has terminated (code 0)
[Wed Jan 28 19:39:31 2026] [Director] Child process 105174 has terminated (code 0)
[Wed Jan 28 19:39:31 2026] [Director] Child process 105173 has terminated (code 0)
[Wed Jan 28 19:39:31 2026] [Director] Child process 105172 has terminated (code 0)
[Wed Jan 28 19:39:31 2026] [Director] Child process 105171 has terminated (code 0)
[Wed Jan 28 19:39:31 2026] [Director] Child process 105170 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 78719
	Y = 78724
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 157453
B: 157443
C: 78861
D: 78868

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 10
Składniki B: 0
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 78719
Wyprodukowano Y: 78724
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```
- 157453 - 78719 - 78724 = 10 (składniki A się zgadzają)
- 157443 - 78719 - 78724 = 0  (składniki B się zgadzają)
- 78861 - 78719 = 142 (składniki C się zgadzają)
- 78868 - 78724 = 144 (składniki D się zgadzają)

**Test 6 - Blokowanie magazynu**
- Test sprawdza czy, kiedy jeden z pracowników będzie bardzo wolny, to ciągłe próby dostaw nie zablokują dostępu do magazynu.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 10 |

*Po wyprodukowaniu Y proces pracownika jest usypiany na 3 sekundy*

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor
Wed Jan 28 19:58:25 2026] [Director] IPC Resources created!
[Wed Jan 28 19:58:25 2026] [Director] Restoring magazine state...
[Wed Jan 28 19:58:25 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 19:58:25 2026] [Director] Synchronized semaphores!
[Wed Jan 28 19:58:25 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 19:58:25 2026] [Director] Spawning child processes...
[Wed Jan 28 19:58:25 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 19:58:25 2026] [Worker: Y] Worker Y started
[Wed Jan 28 19:58:25 2026] [Worker: X] Worker X started
[Wed Jan 28 19:58:25 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 19:58:25 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 19:58:25 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 19:58:25 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:25 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:25 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:25 2026] [Supplier: A] Delivered A component!
.
.
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCED chocolate type X!
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Wed Jan 28 19:58:31 2026] [Worker: X] Terminating
[Wed Jan 28 19:58:31 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Director] Stopping C deliveries!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier] Received SIGINT
[Wed Jan 28 19:58:31 2026] [Supplier: C] Terminating
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 19:58:31 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 19:58:31 2026] [Process Manager] Successfully collected process 109901 (exit code 0)!
.
.
Wed Jan 28 20:05:13 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 20:05:16 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 20:05:16 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 20:05:16 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 20:05:16 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 20:05:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 20:05:19 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 20:05:19 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 20:05:19 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 20:05:22 2026] [Worker: Y] PRODUCED chocolate type Y!
[Wed Jan 28 20:05:22 2026] [Supplier: D] Delivered D component!
[Wed Jan 28 20:05:22 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 20:05:22 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 20:05:25 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Wed Jan 28 20:05:25 2026] [Director] Stopping A deliveries!
[Wed Jan 28 20:05:25 2026] [Director] Stopping B deliveries!
[Wed Jan 28 20:05:25 2026] [Director] Stopping D deliveries!
[Wed Jan 28 20:05:25 2026] [Supplier: D] Not enough space for component D!
[Wed Jan 28 20:05:25 2026] [Supplier] Received SIGINT
[Wed Jan 28 20:05:25 2026] [Supplier: B] Terminating
[Wed Jan 28 20:05:25 2026] [Supplier] Received SIGINT
[Wed Jan 28 20:05:25 2026] [Supplier] Received SIGINT
[Wed Jan 28 20:05:25 2026] [Supplier: D] Terminating
[Wed Jan 28 20:05:25 2026] [Supplier: A] Terminating
[Wed Jan 28 20:05:25 2026] [Process Manager] Successfully collected process 110895 (exit code 0)!
[Wed Jan 28 20:05:25 2026] [Process Manager] Successfully collected process 110896 (exit code 0)!
[Wed Jan 28 20:05:25 2026] [Process Manager] Successfully collected process 110898 (exit code 0)!
[Wed Jan 28 20:05:28 2026] [Worker: Y] Terminating
[Wed Jan 28 20:05:28 2026] [Process Manager] Successfully collected process 110894 (exit code 0)!
[Wed Jan 28 20:05:28 2026] [Director] FACTORY FINISHED WORK!
[Wed Jan 28 20:05:28 2026] [Director] Saved magazine state!
[Director] Child process 110892 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 10
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 100152
B: 100152
C: 100037
D: 154

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 142
Składniki B: 142
Składniki C: 37 (zajęte bajty: 74)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 100000
Wyprodukowano Y: 10
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯
```

**Test 7 - Poprawna ilość dostarczonych składników (bez wczytywania magazynu)**

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 250000 |
| `Y_TO_PRODUCE` | 100000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor
.
.
[Wed Jan 28 17:50:11 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:50:11 2026] [Supplier: A] Terminating
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Director] Stopping B deliveries!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:50:11 2026] [Director] Stopping C deliveries!
[Wed Jan 28 17:50:11 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:50:11 2026] [Process Manager] Successfully collected process 84337 (exit code 0)!
[Wed Jan 28 17:50:11 2026] [Supplier: B] Terminating
[Wed Jan 28 17:50:11 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:50:11 2026] [Supplier: C] Terminating
[Wed Jan 28 17:50:11 2026] [Process Manager] Successfully collected process 84339 (exit code 0)!
[Wed Jan 28 17:50:11 2026] [Process Manager] Successfully collected process 84340 (exit code 0)!
[Wed Jan 28 17:50:11 2026] [Process Manager] Successfully collected process 84341 (exit code 0)!
[Wed Jan 28 17:50:11 2026] [Director] FACTORY FINISHED WORK!
[Wed Jan 28 17:50:11 2026] [Director] Saved magazine state!
[Director] Child process 84336 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 250000
	Y = 100000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ head -20 simulation.log 
--- SIMULATION START ---
[Wed Jan 28 17:49:56 2026] [Director] IPC Resources created!
[Wed Jan 28 17:49:56 2026] [Director] Restoring magazine state...
[Wed Jan 28 17:49:56 2026] [Director] magazine.bin file not found. Nothing to restore from.
[Wed Jan 28 17:49:56 2026] [Director] Synchronized semaphores!
[Wed Jan 28 17:49:56 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 17:49:56 2026] [Director] Spawning child processes...
[Wed Jan 28 17:49:56 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 17:49:56 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 17:49:56 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 17:49:56 2026] [Worker: X] Worker X started
[Wed Jan 28 17:49:56 2026] [Supplier: A] Starting deliveries of A!
[Wed Jan 28 17:49:56 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 17:49:56 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 17:49:56 2026] [Supplier: B] Starting deliveries of B!
[Wed Jan 28 17:49:56 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 17:49:56 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:49:56 2026] [Supplier: A] Delivered A component!
```

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh 
A: 350004
B: 350012
C: 250142
D: 100144

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 4
Składniki B: 12
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 250000
Wyprodukowano Y: 100000
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

- Jak widać fabryka pobiera tylko tyle składników ile faktycznie jest jej potrzebne do produkcji czekolady. Reszta jest przechowywana w magazynie.

**Test 8 - Poprawna ilość dostarczonych składników (wczytanie magazynu)**

Z poprzedniego testu widać, że w magazynie zostało 4 składniki A, 12 B, 142 C oraz 144 D. Pozwala to na wyprodukowanie 4 czekolad, jeszcze przed rozpoczęciem dostaw.

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
.
.
[Wed Jan 28 17:55:52 2026] [Supplier: C] Delivered C component!
[Wed Jan 28 17:55:52 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Wed Jan 28 17:55:52 2026] [Worker: X] Terminating
[Wed Jan 28 17:55:52 2026] [Director] Stopping A deliveries!
[Wed Jan 28 17:55:52 2026] [Supplier: A] Delivered A component!
[Wed Jan 28 17:55:52 2026] [Director] Stopping B deliveries!
[Wed Jan 28 17:55:52 2026] [Director] Stopping C deliveries!
[Wed Jan 28 17:55:52 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:55:52 2026] [Supplier: A] Terminating
[Wed Jan 28 17:55:52 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:55:52 2026] [Supplier: B] Terminating
[Wed Jan 28 17:55:52 2026] [Supplier] Received SIGINT
[Wed Jan 28 17:55:52 2026] [Supplier: C] Terminating
[Wed Jan 28 17:55:52 2026] [Process Manager] Successfully collected process 85382 (exit code 0)!
[Wed Jan 28 17:55:52 2026] [Process Manager] Successfully collected process 85384 (exit code 0)!
[Wed Jan 28 17:55:52 2026] [Process Manager] Successfully collected process 85385 (exit code 0)!
[Wed Jan 28 17:55:52 2026] [Process Manager] Successfully collected process 85386 (exit code 0)!
[Wed Jan 28 17:55:52 2026] [Director] FACTORY FINISHED WORK!
[Wed Jan 28 17:55:52 2026] [Director] Saved magazine state!
[Director] Child process 85381 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 250000
	Y = 100000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main* 15s
❯ head -30 simulation.log
--- SIMULATION START ---
[Wed Jan 28 17:55:37 2026] [Director] IPC Resources created!
[Wed Jan 28 17:55:37 2026] [Director] Restoring magazine state...
[Wed Jan 28 17:55:37 2026] [Director] Restored magazine state!
[Wed Jan 28 17:55:37 2026] [Director] Synchronized semaphores!
[Wed Jan 28 17:55:37 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Wed Jan 28 17:55:37 2026] [Director] Spawning child processes...
[Wed Jan 28 17:55:37 2026] [Director] Spawned child processes! Starting process manager thread
[Wed Jan 28 17:55:37 2026] [Director] Process Management thread launched successfully!
[Wed Jan 28 17:55:37 2026] [Director] Waiting for Factory to finish work!
[Wed Jan 28 17:55:37 2026] [Worker: X] Worker X started
[Wed Jan 28 17:55:37 2026] [Worker: X] PRODUCED chocolate type X!       <--
[Wed Jan 28 17:55:37 2026] [Worker: X] PRODUCED chocolate type X!       <--
[Wed Jan 28 17:55:37 2026] [Worker: X] PRODUCED chocolate type X!       <--
[Wed Jan 28 17:55:37 2026] [Worker: X] PRODUCED chocolate type X!       <--
[Wed Jan 28 17:55:37 2026] [Worker: Y] Worker Y started
[Wed Jan 28 17:55:37 2026] [Supplier: B] Starting deliveries of B!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!
[Wed Jan 28 17:55:37 2026] [Supplier: B] Delivered B component!

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

- Jak widać program zachował się poprawnie i wyprodukował 4 czekolady typu X.

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./check_deliveries.sh
A: 350001
B: 350068
C: 250000
D: 100000

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/check_magazine
===== STAN MAGAZYNU (magazine.bin) =====
Pojemność całkowita (bajty): 1000
----------------------------------------
Składniki A: 5
Składniki B: 80
Składniki C: 142 (zajęte bajty: 284)
Składniki D: 144 (zajęte bajty: 432)
----------------------------------------
Wyprodukowano X: 250000
Wyprodukowano Y: 100000
========================================

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

Przed uruchomieniem symulacji w magazynie było przechowywane 4 składniki A, 12 B, 142 C oraz 144 D. W takim razie do wyprodukowanie 250000 czekolad X i 100000 czekolad Y program potrzebował dostaw 349996 składników A, 349988 składników B, 249858 składników C oraz 99856 składników D. Wobec tego do magazynu powinno trawić (i trafia):
- 350001 - 349996 = 5 A
- 350068 - 349988 = 80 B
- 250000 - 249858 = 142 C
- 100000 - 99856  = 144 D


## Kluczowe funkcje systemowe użyte w projekcie

- **Obsługa plików**
    - `fopen()`: [użycie w funkcji `restore_state()` oraz `save_state()`](src/dyrektor.c#L389)
    - `fwrite()`: [użycie w funckji `save_state()`](src/dyrektor.c#L408)
    - `fread()`: [użycie w funkcji `restore_state()`](src/dyrektor.c#L391)
    - `fclose()`: [użycie w funkcji `restore_state()` oraz `save_state()`](src/dyrektor.c#L396)

- **Tworzenie procesów**
    - `fork()`: [użycie w procesie dyrektora](src/dyrektor.c#L155)
    - `execl()`: [użycie w procesie dyrektora - technicznie w dziecku](src/dyrektor.c#L168)
    - `waitpid()`: [użycie w wątku process managera](src/dyrektor.c#L425)

- **Tworzenie i obsługa wątków**
    - `pthread_create()`: [użycie w procesie dyrektora](src/dyrektor.c#L205)
    - `pthread_join()`: [użycie w procesie dyrektora](src/dyrektor.c#L309)

- **Obsługa sygnałów**
    - `kill()`: [przykładowe użycie w procesie pracownika](src/pracownik.c#L104)
    - `sigaction()`: [przykładowe użycie w procesie pracownika](src/pracownik.c#L20)
    - `sigtimedwait()`: [użycie w procesie dyrektora](src/dyrektor.c#L225)

- **Synchronizacja procesów**
    - `ftok()`: [użycie w procesie dyrektora](src/dyrektor.c#L60)
    - `semget()`: [użycie w procesie dyrektora](src/dyrektor.c#L79)
    - `semctl()`: [użycie w procesie dyrektora](src/dyrektor.c#L100)
    - `semop()`: [użycie w procesie dostawcy](src/dostawca.c#L95)

- **Segmenty pamięcie dzielonej**
    - `shmget()`: [użycie w procesie dyrektora](src/dyrektor.c#L66)
    - `shmat()`: [użycie w procesie dostawcy](src/dostawca.c#L62)
    - `shmdt()`: [użycie w procesie pracownika](src/pracownik.c#L251)
    - `shmctl()`: [użycie w procesie dyrektora](src/dyrektor.c#L338)

- **Kolejki komunikatów**
    - `msgget()`: [użycie w procesie dyrektora](src/dyrektor.c#L106)
    - `msgsnd()`: [użycie w funkcji `send_log()`](include/common.h#L117)
    - `msgrcv()`: [użycie w procesie loggera](src/logger.c#L49)
    - `msgctl()`: [użycie w procesie loggera](src/logger.c#L61)
