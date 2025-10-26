package org.example;

// Interfejs kolejki
public interface KolejkaPriorytetowa {
    void dodajZdarzenie(Zdarzenie zdarzenie);

    Zdarzenie getNastepneZdarzenie();

    boolean jestPusta();
}
