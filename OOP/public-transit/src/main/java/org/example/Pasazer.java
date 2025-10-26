package org.example;


public class Pasazer {
    private final int id;
    private final Przystanek przystanekDom;
    private Przystanek przystanek = null;
    private Przystanek przystanekWyjscia = null;
    private int czasPrzyjscia;
    public Pasazer(Przystanek przystanekDom, int id) {
        this.przystanekDom = przystanekDom;
        this.id=id;
    }

    // Poranne wylosowanie czasu przyjscia pasazera na przystanek
    public void idzNaPrzystanek(KolejkaPriorytetowa kolejka, int dzien) {
        Zdarzenie przyjsciePasazera = new PrzyjsciePasazera(Losowanie.losujCzasPrzyjscia(),dzien,this);
        this.czasPrzyjscia = przyjsciePasazera.getCzas();
        kolejka.dodajZdarzenie(przyjsciePasazera);
    }

    public void wroc() {
        czasPrzyjscia=0;
        przystanek=null;
        przystanekWyjscia=null;
    }

    public void wrocZprzystanku (Liczyciel liczyciel) {
        int godzina24=1080;
        liczyciel.zwiekszLacznyCzasOczekiwaniaDnia(godzina24-czasPrzyjscia);
        przystanek = null;
        przystanekWyjscia = null;
    }

    public void setPrzystanekWyjscia(Przystanek przystanekWyjscia){
        this.przystanekWyjscia=przystanekWyjscia;
    }
    public void setCzasPrzyjscia(int czasPrzyjscia) {
        this.czasPrzyjscia=czasPrzyjscia;
    }
    public int getCzasPrzyjscia() {
        return czasPrzyjscia;
    }
    public Przystanek getPrzystanekDom(){
        return przystanekDom;
    }
    public Przystanek getPrzystanekWyjscia(){
        return przystanekWyjscia;
    }
    public int getId() {
        return id;
    }
}
