package org.example;

public class KolejkaPriorytet implements KolejkaPriorytetowa {
    private static final int poczatkowaPojemnosc = 1;
    private Zdarzenie[] zdarzenia;
    private int wielkosc;

    public KolejkaPriorytet() {
        zdarzenia = new Zdarzenie[poczatkowaPojemnosc];
        wielkosc = 0;
    }



    // Dodawanie zdarzenia do kolejki
    public void dodajZdarzenie(Zdarzenie zdarzenie) {
        zapewnijPojemnosc();
        int index = wielkosc;
        while (index > 0 && zdarzenie.compareTo(zdarzenia[index - 1]) < 0) {
            zdarzenia[index] = zdarzenia[index - 1];
            index--;
        }
        zdarzenia[index] = zdarzenie;
        wielkosc++;
    }

    // Pobieranie i usuwanie najwcześniejszego zdarzenia z kolejki
    public Zdarzenie getNastepneZdarzenie() {
        if (jestPusta()) {
            throw new IllegalStateException("Queue is empty");
        }
        Zdarzenie pierwszeZdarzenie = zdarzenia[0];  // Pobranie pierwszego zdarzenia
        przesunZdarzeniaWLewo();  // Przesunięcie pozostałych zdarzeń w lewo
        wielkosc--;  // Zmniejszenie ilości zdarzeń w kolejce
        return pierwszeZdarzenie;  // Zwrócenie pierwszego zdarzenia
    }

    private void przesunZdarzeniaWLewo() {
        for (int i = 0; i < wielkosc - 1; i++) {
            zdarzenia[i] = zdarzenia[i + 1];  // Przesunięcie każdego zdarzenia w lewo
        }
        zdarzenia[wielkosc - 1] = null;  // Usunięcie referencji do ostatniego zdarzenia
    }


    // Sprawdzenie, czy kolejka jest pusta
    public boolean jestPusta() {
        return wielkosc == 0;
    }

    // Zwiększenie pojemności tablicy w razie potrzeby
    private void zapewnijPojemnosc() {
        if (wielkosc == zdarzenia.length) {
            Zdarzenie[] nowaTablica = new Zdarzenie[zdarzenia.length * 2];
            System.arraycopy(zdarzenia, 0, nowaTablica, 0, wielkosc);
            zdarzenia = nowaTablica;
        }
    }
}