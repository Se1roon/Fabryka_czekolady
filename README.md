# SO - Fabryka Czekolady

Zadanie polega na zaprojektowaniu fabryki czekolady, w której produkowane są dwa typy czekolady. Pierwszy typ składa się ze składników A, B oraz C, drugi natomiast ze składników A, B i D. Składniki przechowywane są w magazynie o pojemności N jednostek. Każdy ze składników zajmuje określoną ilość miejsca w magazynie:

- A zajmuje 1 jednostkę
- B zajmuje 1 jednostkę
- C zajmuje 2 jednostki
- D zajmuje 3 jednostki

Fabryka składa się z dwóch stanowiski, na których produkowana jest czekolada danego typu. Dodatkowo przez cały czas pracy fabryki dostarczane są składniki do magazynu pochodzące z niezależnych źródeł. Nad fabryką czuwa, dyrektor który może wydawać następujące polecenia:
- polecenie_1: fabryka kończy pracę
- polecenie_2: magazyn kończy pracę
- polecenie_3: dostawcy przerywają dostawy
- polecenie_4: fabryka i magazyn kończą pracę (*połączenie poleceń 1 oraz 2*)

![Schemat projektu](./schemat.png)

---

Działanie każdego z opisanych poniżej procesów jest logowane.

## Proces 'Fabryka'

Fabryka składa się z dwóch wątków reprezentujących stanowiska, na których produkowana jest czekolada. Magazyn reprezentowany jest jako struktura zawierające dane na temat jego pojemności oraz ilości składników.

Ponieważ, **stanowisko_1** oraz **stanowisko_2** dokonują modyfikacji magazynu potrzebna jest ich synchronizacja - poprzez *mutexy*.

Fabryka może otrzymać od dyrektora nastepujące sygnały:
- **polecenie_1**: powoduje zablokowanie magazynu, uniemożliwiając zmiane jego wartości
- **polecenie_2**: powoduje zaprzestanie produkcji na stanowiskach

*polecenie_4 reprezentowane będzie jako połączenie polecenia 1 oraz 2*

Dodatkowo, gdy magazyn kończy pracę, jego stan zapisywany jest do pliku, a przy ponownym uruchomieniu jest z tego pliku odczytywany.

## Proces 'Dostawcy'

Dostawcy składa się z czterech wątków reprezentujących dostawców każdego ze składników. Ich jedynym zadaniem jest modyfikowanie obszaru pamięci współdzielonej magazynu, która będzie synchronizowana za pomocą semafor.

Dostawcy mogą otrzymać sygnał **polecenie_3** co stopuje ich pracę.

## Proces 'Dyrektor'

Dyrektor jest procesem rodzicem dla procesów *Fabryka* oraz *Dostawcy*. Wydaje on polecenia swoim dzieciom, oraz jest interfejsem pomiędzy użytkownikiem a fabryką (pozwala chociażby na wyświetlenie statystyk produkcji (mam nadzieje, że będzie)).
