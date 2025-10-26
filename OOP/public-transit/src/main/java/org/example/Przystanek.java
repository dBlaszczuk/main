package org.example;

// Klasa przystanku
public class Przystanek {
    private final String nazwa;
    private final int pojemnosc;

    private final Pasazer[] pasazerowie;
    private int iloscOczekujacych=0;
    public Przystanek(String nazwa, int pojemnosc){
        this.nazwa=nazwa;
        this.pojemnosc=pojemnosc;
        this.pasazerowie = new Pasazer[pojemnosc];
    }

    public void wrocPasazerow(Liczyciel liczyciel){
        for (int i = 0; i<pojemnosc; i++) {
            if (pasazerowie[i] != null) {
                pasazerowie[i].wrocZprzystanku(liczyciel);
                pasazerowie[i] = null;
            }
        }
        iloscOczekujacych=0;
    }
    public void dodajPasazera(Pasazer pasazer, int czasPrzyjscia) {
        pasazer.setCzasPrzyjscia(czasPrzyjscia);
        pasazerowie[iloscOczekujacych]=pasazer;
        iloscOczekujacych++;
    }

    // Metoda do przenoszenia ostatniego pasazera w tablicy do pojazdu
    public Pasazer zabierOstatniego() {
        Pasazer ostatniPasazer = pasazerowie[iloscOczekujacych - 1];
        pasazerowie[iloscOczekujacych - 1] = null;
        iloscOczekujacych--;
        return ostatniPasazer;
    }
    public String getNazwa() {
        return nazwa;
    }

    public int getPojemnosc() {
        return pojemnosc;
    }

    public int getIloscOczekujacych() {
        return iloscOczekujacych;
    }
}
