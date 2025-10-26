package org.example;

// Abstrakcyjna klasa odpowiedzialna za generowanie zdarzen do kolejki
public abstract class Zdarzenie implements Comparable<Zdarzenie> {
    protected int czas;
    protected int dzien;
    public Zdarzenie(int czas, int dzien) {
        this.czas = czas;
        this.dzien=dzien;
    }

    public abstract int getCzas();
    public abstract void wykonajZdarzenie(Liczyciel liczyciel);

    @Override
    public int compareTo(Zdarzenie other) {
        return Integer.compare(this.czas, other.czas);
    }
}

