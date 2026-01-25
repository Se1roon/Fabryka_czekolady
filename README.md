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
.
.
[Fri Jan 23 16:54:20 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Fri Jan 23 16:54:20 2026] [Worker: X] Terminating
[Fri Jan 23 16:54:20 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Director] Stopping C deliveries!
[Fri Jan 23 16:54:20 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Fri Jan 23 16:54:20 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:54:20 2026] [Worker: Y] Terminating
[Fri Jan 23 16:54:20 2026] [Supplier: C] Terminating
[Fri Jan 23 16:54:20 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:54:20 2026] [Director] Stopping A deliveries!
[Fri Jan 23 16:54:20 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Supplier: A] Terminating
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:54:20 2026] [Director] Stopping B deliveries!
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74191 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74189 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Supplier: B] Terminating
[Fri Jan 23 16:54:20 2026] [Director] Stopping D deliveries!
[Fri Jan 23 16:54:20 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74192 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Supplier: D] Terminating
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74187 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74188 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Process Manager] Successfully collected process 74190 (exit code 0)!
[Fri Jan 23 16:54:20 2026] [Director] FACTORY FINISHED WORK!
[Fri Jan 23 16:54:20 2026] [Director] Saved magazine state!
[Director] Child process 74186 has terminated (code 0)
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
.
.
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:56:02 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:56:02 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:56:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:56:02 2026] [Supplier: A] Delivered A component!
^Zfish: Job 2, './bin/dyrektor' has stopped

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ps a
    PID TTY      STAT   TIME COMMAND
    855 tty2     S<sl+   0:01 /usr/lib/Xorg -nolisten tcp -background none -seat seat0 vt2 -auth /run/sddm/xauth_nYSvBj -noreset -displayfd 16
   1146 tty1     S<s+   0:00 /usr/bin/sh /usr/lib/uwsm/signal-handler.sh wayland-session-envelope@hyprland.desktop.target
   1204 tty1     S<+    0:00 systemctl --user start --wait wayland-session-envelope@hyprland.desktop.target
   3412 pts/1    S<sl   0:05 /bin/fish
  71498 pts/1    T<     0:00 nvim README.md
  74607 pts/1    T<l    0:02 ./bin/dyrektor
  74609 pts/1    T<     0:02 ./bin/logger
  74610 pts/1    T<     0:00 ./bin/dostawca A
  74611 pts/1    T<     0:00 ./bin/dostawca B
  74612 pts/1    T<     0:00 ./bin/dostawca C
  74614 pts/1    T<     0:00 ./bin/pracownik X
  74709 pts/1    R<+    0:00 ps a

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ fg
.
.
[Fri Jan 23 16:57:34 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:57:34 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:57:34 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:57:34 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:57:34 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:57:34 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:57:34 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:57:34 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:57:34 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:57:34 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:57:34 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:57:34 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 16:57:34 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:57:34 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:57:34 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 16:57:34 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Fri Jan 23 16:57:34 2026] [Worker: X] Terminating
[Fri Jan 23 16:57:34 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 16:57:34 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 16:57:34 2026] [Director] Stopping A deliveries!
[Fri Jan 23 16:57:34 2026] [Director] Stopping B deliveries!
[Fri Jan 23 16:57:34 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:57:34 2026] [Supplier: A] Terminating
[Fri Jan 23 16:57:34 2026] [Director] Stopping C deliveries!
[Fri Jan 23 16:57:34 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:57:34 2026] [Supplier] Received SIGINT
[Fri Jan 23 16:57:34 2026] [Supplier: C] Terminating
[Fri Jan 23 16:57:34 2026] [Supplier: B] Terminating
[Fri Jan 23 16:57:34 2026] [Process Manager] Successfully collected process 74614 (exit code 0)!
[Fri Jan 23 16:57:34 2026] [Process Manager] Successfully collected process 74610 (exit code 0)!
[Fri Jan 23 16:57:34 2026] [Process Manager] Successfully collected process 74611 (exit code 0)!
[Fri Jan 23 16:57:34 2026] [Process Manager] Successfully collected process 74612 (exit code 0)!
[Fri Jan 23 16:57:34 2026] [Director] FACTORY FINISHED WORK!
[Fri Jan 23 16:57:34 2026] [Director] Saved magazine state!
[Director] Child process 74609 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 10000
[Director] IPC cleaned up.

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
.
.
^C[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:06:14 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:06:14 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:06:14 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:06:14 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:06:14 2026] [Worker] Received SIGINT - Terminating
[Fri Jan 23 17:06:14 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:06:14 2026] [Worker: Y] Terminating
[Fri Jan 23 17:06:14 2026] [Worker] Received SIGINT - Terminating
[Fri Jan 23 17:06:14 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:06:14 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:06:14 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:06:14 2026] [Supplier: C] Terminating
[Fri Jan 23 17:06:14 2026] [Supplier: D] Terminating
[Fri Jan 23 17:06:14 2026] [Worker: X] Terminating
[Fri Jan 23 17:06:14 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:06:14 2026] [Director] Received SIGINT
[Fri Jan 23 17:06:14 2026] [Supplier: A] Terminating
[Fri Jan 23 17:06:14 2026] [Supplier: B] Terminating
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76750 (exit code 0)!
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76747 (exit code 0)!
[Fri Jan 23 17:06:14 2026] [Director] Saved magazine state!
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76748 (exit code 0)!
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76749 (exit code 0)!
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76745 (exit code 0)!
[Fri Jan 23 17:06:14 2026] [Process Manager] Successfully collected process 76746 (exit code 0)!
[Director] Amount of chocolate produced:
	X = 26074
	Y = 26070
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

**Test 4 - Sygnał SIGTERM, zmienianie dostępności magazynu**
- Test sprawdza czy po otrzymaniu sygnału SIGTERM dyrektor poprawnie steruje pracą magazynu. Sygnał ten działa jak przełączenik, który wstrzymuje/wznawia pracę magazynu.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 200000 |

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
.
.
.
[kill <pid dyrektora>]
.
[Fri Jan 23 17:07:35 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:35 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:35 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:35 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:07:35 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:35 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:35 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:35 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:07:35 2026] [Director] Received SIGTERM... Stopping Magazine!
[Fri Jan 23 17:07:35 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:35 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:35 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:35 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:07:35 2026] [Director] Magazine Stopped!
[kill <pid dyrektora]
.
.
[Fri Jan 23 17:07:57 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:07:57 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:57 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:57 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:57 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Fri Jan 23 17:07:57 2026] [Worker: Y] Terminating
[Fri Jan 23 17:07:57 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:57 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:57 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:57 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:07:57 2026] [Director] Stopping A deliveries!
[Fri Jan 23 17:07:57 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:07:57 2026] [Director] Stopping B deliveries!
[Fri Jan 23 17:07:57 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:07:57 2026] [Director] Stopping D deliveries!
[Fri Jan 23 17:07:57 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:07:57 2026] [Supplier: A] Terminating
[Fri Jan 23 17:07:57 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:07:57 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:07:57 2026] [Supplier: B] Terminating
[Fri Jan 23 17:07:57 2026] [Supplier: D] Terminating
[Fri Jan 23 17:07:57 2026] [Process Manager] Successfully collected process 77187 (exit code 0)!
[Fri Jan 23 17:07:57 2026] [Process Manager] Successfully collected process 77188 (exit code 0)!
[Fri Jan 23 17:07:57 2026] [Process Manager] Successfully collected process 77190 (exit code 0)!
[Fri Jan 23 17:07:57 2026] [Process Manager] Successfully collected process 77192 (exit code 0)!
[Fri Jan 23 17:07:57 2026] [Director] FACTORY FINISHED WORK!
[Fri Jan 23 17:07:57 2026] [Director] Saved magazine state!
[Director] Child process 77186 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 200000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main* 28s
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
.
.
[kill <pid dostawcy]
.
Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier] Something weird is happening... Notifying Director
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Director] Received SIGUSR2... A terrorist attack detected!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Director] Saved magazine state!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:18:18 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:18:18 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Worker] Received SIGINT - Terminating
[Fri Jan 23 17:18:18 2026] [Worker: Y] Terminating
[Fri Jan 23 17:18:18 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:18:18 2026] [Supplier: B] Terminating
[Fri Jan 23 17:18:18 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:18:18 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:18:18 2026] [Supplier: D] Terminating
[Fri Jan 23 17:18:18 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:18:18 2026] [Supplier: A] Terminating
[Fri Jan 23 17:18:18 2026] [Director] Child process 78919 has terminated (code 0)
[Fri Jan 23 17:18:18 2026] [Director] Child process 78917 has terminated (code 0)
[Fri Jan 23 17:18:18 2026] [Director] Child process 78915 has terminated (code 0)
[Fri Jan 23 17:18:18 2026] [Director] Child process 78914 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 135272
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main* 6s
❯ 
```

**Test 6 - Prawidłowy zapis/odczyt do pliku. Prawidłowe wznawianie pracy po przedwczesnym zamknięciu fabryki**
- Test sprawdza czy stan magazynu jest prawidłowo zapisywany oraz odczytywany. Gdy do wyprodukowania było np. 100000 czekolady typu X, ale w skutek chociażby ataku terrorystycznego, fabryka została zamknięta, to przy ponownym uruchomieniu liczba czekolady, którą udało się wyprodukować powinno zostać odczytana.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 100000 |
| `Y_TO_PRODUCE` | 200000 |

*Te wartości nie powinny się zmienić podczas drugiej egzekucji programu*

*1 wywołanie programu*
```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
.
.
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
^C[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:21:19 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:21:19 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:21:19 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:21:19 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:21:19 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:21:19 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:21:19 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:21:19 2026] [Supplier: A] Terminating
[Fri Jan 23 17:21:19 2026] [Worker] Received SIGINT - Terminating
[Fri Jan 23 17:21:19 2026] [Worker: X] Terminating
[Fri Jan 23 17:21:19 2026] [Supplier: D] Terminating
[Fri Jan 23 17:21:19 2026] [Supplier: B] Terminating
[Fri Jan 23 17:21:19 2026] [Worker] Received SIGINT - Terminating
[Fri Jan 23 17:21:19 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:21:19 2026] [Supplier: C] Terminating
[Fri Jan 23 17:21:19 2026] [Worker: Y] Terminating
[Fri Jan 23 17:21:19 2026] [Director] Received SIGINT
[Fri Jan 23 17:21:19 2026] [Director] Saved magazine state!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79516 (exit code 0)!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79517 (exit code 0)!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79518 (exit code 0)!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79519 (exit code 0)!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79520 (exit code 0)!
[Fri Jan 23 17:21:19 2026] [Process Manager] Successfully collected process 79521 (exit code 0)!
[Director] Amount of chocolate produced:
	X = 8760
	Y = 8758
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main*
❯ 
```

*2 wywołanie programu*
```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor 
[Fri Jan 23 17:22:53 2026] [Director] IPC Resources created!
[Fri Jan 23 17:22:53 2026] [Director] Restoring magazine state...
[Fri Jan 23 17:22:53 2026] [Director] Restored magazine state!
[Fri Jan 23 17:22:53 2026] [Director] Amount of chocolate produced so far:
	X = 8760
	Y = 8758
[Fri Jan 23 17:22:53 2026] [Director] Creating thread for managing processes...
[Fri Jan 23 17:22:53 2026] [Director] Process Management thread launched successfully!
[Fri Jan 23 17:22:53 2026] [Director] Spawning child processes...
[Fri Jan 23 17:22:53 2026] [Supplier: A] Starting deliveries of A!
[Fri Jan 23 17:22:53 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:22:53 2026] [Supplier: A] Delivered A component!
.
.
.
[Fri Jan 23 17:23:02 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:23:02 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:23:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:23:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:23:02 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Fri Jan 23 17:23:02 2026] [Worker: Y] Terminating
[Fri Jan 23 17:23:02 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:23:02 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:23:02 2026] [Director] Stopping A deliveries!
[Fri Jan 23 17:23:02 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:23:02 2026] [Director] Stopping B deliveries!
[Fri Jan 23 17:23:02 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:23:02 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:23:02 2026] [Director] Stopping D deliveries!
[Fri Jan 23 17:23:02 2026] [Supplier: A] Terminating
[Fri Jan 23 17:23:02 2026] [Supplier: B] Terminating
[Fri Jan 23 17:23:02 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:23:02 2026] [Supplier: D] Terminating
[Fri Jan 23 17:23:02 2026] [Process Manager] Successfully collected process 79839 (exit code 0)!
[Fri Jan 23 17:23:02 2026] [Process Manager] Successfully collected process 79834 (exit code 0)!
[Fri Jan 23 17:23:02 2026] [Process Manager] Successfully collected process 79835 (exit code 0)!
[Fri Jan 23 17:23:02 2026] [Process Manager] Successfully collected process 79837 (exit code 0)!
[Fri Jan 23 17:23:02 2026] [Director] FACTORY FINISHED WORK!
[Fri Jan 23 17:23:02 2026] [Director] Saved magazine state!
[Director] Child process 79833 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 100000
	Y = 200000
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main* 8s
❯ 

```

**Test 7 - Blokowanie magazynu**
- Test sprawdza czy, kiedy jeden z pracowników będzie bardzo wolny, to ciągłe próby dostaw nie zablokują dostępu do magazynu.

| Stała | Wartość |
|-------|---------|
| `X_TO_PRODUCE` | 10 |
| `Y_TO_PRODUCE` | 3 |

*Po wyprodukowaniu Y proces pracownika jest usypiany na 5 sekund*

```
~/Documents/Studia/SO/Fabryka_czekolady main*
❯ ./bin/dyrektor
[Fri Jan 23 17:41:46 2026] [Director] IPC Resources created!
[Fri Jan 23 17:41:46 2026] [Director] Restoring magazine state...
[Fri Jan 23 17:41:46 2026] [Director] Restored magazine state!
[Fri Jan 23 17:41:46 2026] [Director] Amount of chocolate produced so far:
	X = 0
	Y = 0
[Fri Jan 23 17:41:46 2026] [Director] Creating thread for managing processes...
[Fri Jan 23 17:41:46 2026] [Director] Process Management thread launched successfully!
[Fri Jan 23 17:41:46 2026] [Director] Spawning child processes...
[Fri Jan 23 17:41:46 2026] [Supplier: A] Starting deliveries of A!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Director] Spawned child processes!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Director] Waiting for Factory to finish work!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Starting deliveries of B!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Starting deliveries of C!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: D] Starting deliveries of D!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Worker: Y] Worker Y started
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Worker: X] Worker X started
[Fri Jan 23 17:41:46 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCED chocolate type X!
[Fri Jan 23 17:41:46 2026] [Supplier: C] Delivered C component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:46 2026] [Worker: X] PRODUCTION X COMPLETE | Sending notification to Director
[Fri Jan 23 17:41:46 2026] [Worker: X] Terminating
[Fri Jan 23 17:41:46 2026] [Director] Stopping C deliveries!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:41:46 2026] [Supplier: C] Terminating
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Process Manager] Successfully collected process 83890 (exit code 0)!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Process Manager] Successfully collected process 83888 (exit code 0)!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:46 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:51 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:41:51 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:51 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:51 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:41:56 2026] [Worker: Y] PRODUCED chocolate type Y!
[Fri Jan 23 17:41:56 2026] [Supplier: A] Delivered A component!
[Fri Jan 23 17:41:56 2026] [Supplier: B] Delivered B component!
[Fri Jan 23 17:41:56 2026] [Supplier: D] Delivered D component!
[Fri Jan 23 17:42:01 2026] [Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director
[Fri Jan 23 17:42:01 2026] [Director] Stopping A deliveries!
[Fri Jan 23 17:42:01 2026] [Director] Stopping B deliveries!
[Fri Jan 23 17:42:01 2026] [Director] Stopping D deliveries!
[Fri Jan 23 17:42:01 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:42:01 2026] [Supplier: A] Terminating
[Fri Jan 23 17:42:01 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:42:01 2026] [Supplier: B] Terminating
[Fri Jan 23 17:42:01 2026] [Supplier] Received SIGINT
[Fri Jan 23 17:42:01 2026] [Supplier: D] Terminating
[Fri Jan 23 17:42:01 2026] [Process Manager] Successfully collected process 83886 (exit code 0)!
[Fri Jan 23 17:42:01 2026] [Process Manager] Successfully collected process 83887 (exit code 0)!
[Fri Jan 23 17:42:01 2026] [Process Manager] Successfully collected process 83889 (exit code 0)!
[Fri Jan 23 17:42:06 2026] [Worker: Y] Terminating
[Fri Jan 23 17:42:06 2026] [Process Manager] Successfully collected process 83891 (exit code 0)!
[Fri Jan 23 17:42:06 2026] [Director] FACTORY FINISHED WORK!
[Fri Jan 23 17:42:06 2026] [Director] Saved magazine state!
[Director] Child process 83885 has terminated (code 0)
[Director] Amount of chocolate produced:
	X = 10
	Y = 3
[Director] IPC cleaned up.

~/Documents/Studia/SO/Fabryka_czekolady main* 20s
❯ 
```

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
