package org.example;

public class Liczyciel {
    // Wejscie do tramwaju i wyjscie na wlasciwym przystanku
    private int lacznaLiczbaPrzejazdow=0;
    private int lacznyCzasOczekiwania=0;

    private int wejscianaprzystanek=0;
    // DLA KAZDEGO DNIA
    private int lacznaLiczbaPrzejazdowDnia=0;
    private int lacznyCzasOczekiwaniaDnia=0;


    public int obliczSredniCzasOczekiwania(){
        if (wejscianaprzystanek != 0) {
            return lacznyCzasOczekiwania / wejscianaprzystanek;
        }
        else {
            System.out.println("Nikt nie wszedl na przystanek");
            return -1;
        }
    }
    public int konwertujNaGodziny (int czas) {
        return czas / 60;
    }

    public int konwertujNaMinuty (int czas) {
        return czas % 60;
    }

    public String zmienNaCzasDobowy (int czas) {
        int godziny = konwertujNaGodziny(czas);
        int minuty = konwertujNaMinuty(czas);
        return (6+godziny) + ":" + minuty;
    }

    public void zwiekszWejsciaNaPrzystanek() {
        wejscianaprzystanek++;
    }
    public void zwiekszLiczbePrzejazdowDnia(){
        lacznaLiczbaPrzejazdowDnia++;
    }
    public void zwiekszLacznyCzasOczekiwaniaDnia(int czas){
        lacznyCzasOczekiwaniaDnia+=czas;
    }

    // Dodaje przy okazji do wartosci calego czasu symulacji
    public void resetujLicznikPodKoniecDnia() {
        lacznaLiczbaPrzejazdow+=lacznaLiczbaPrzejazdowDnia;
        lacznyCzasOczekiwania+=lacznyCzasOczekiwaniaDnia;
        lacznaLiczbaPrzejazdowDnia=0;
        lacznyCzasOczekiwaniaDnia=0;
    }

    public int getLacznaLiczbaPrzejazdowDnia(){
        return lacznaLiczbaPrzejazdowDnia;
    }

    public int getLacznaLiczbaPrzejazdow(){
        return lacznaLiczbaPrzejazdow;
    }
    public int getLacznyCzasOczekiwaniaDnia() {
        return lacznyCzasOczekiwaniaDnia;
    }


}
