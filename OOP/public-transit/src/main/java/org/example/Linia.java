package org.example;

// Klasa lini
public class Linia {
    private final int numerLini;
    private final int liczbaPojazdow;
    private final int dlugoscTrasy;
    private Tramwaj[] pojazdy;
    private final int[] czasPodrozy;
    private final Przystanek[] przystanki;

    public void wrocTramwaje() {
        for (int i=0; i<liczbaPojazdow; i++) {
            pojazdy[i].wrocTramwaj();
        }
    }

    public Linia(int numerLini, int liczbaPojazdow, int dlugoscTrasy, Przystanek[] przystanki, int[] czasDojazdu){
        this.numerLini=numerLini;
        this.liczbaPojazdow=liczbaPojazdow;
        this.dlugoscTrasy=dlugoscTrasy;
        this.przystanki=przystanki;
        this.czasPodrozy=czasDojazdu;
    }
    public int getOdstep(){
        if (liczbaPojazdow == 0 || dlugoscTrasy == 0) {
            throw new IllegalArgumentException("Zakladamy zgodnie z wpisami na form ze nie sa zerowe");
        }
        int czas=0;
        for(int i=0; i<dlugoscTrasy;i++) {
            czas += czasPodrozy[i];
        }
        czas*=2;
        return czas/liczbaPojazdow;
    }

    public int getIndeksPrzystanku(Przystanek przystanek) {
        for (int i = 0; i<dlugoscTrasy; i++) {
            if (przystanek.equals(przystanki[i])) {
                return i;
            }
        }

        return -1;
    }

    // Metoda odpowiadajaca za jednoczesne wypuszczenie tramwajow danej lini z dwoch stron na raz z odpowienim opoznieniem
    public void wypuscTramwaje(KolejkaPriorytetowa kolejka, int dzien){
        int czasOdstepu = getOdstep();
        if (liczbaPojazdow % 2 == 0) {
            for (int i=0; i<liczbaPojazdow/2; i++) {
                pojazdy[i].ustawNaZajezdni();
                Zdarzenie przyjazdTramwaju = new PrzyjazdTramwaju(czasOdstepu*i,dzien,pojazdy[i],kolejka);
                kolejka.dodajZdarzenie(przyjazdTramwaju);
            }
            int mnoznik = 0;
            for (int i=liczbaPojazdow/2; i<liczbaPojazdow; i++) {
                pojazdy[i].ustawNaZajezdni();
                Zdarzenie przyjazdTramwaju = new PrzyjazdTramwaju(czasOdstepu*mnoznik,dzien,pojazdy[i],kolejka);
                kolejka.dodajZdarzenie(przyjazdTramwaju);
                mnoznik++;
            }
        }
        else {
            for (int i=0; i<liczbaPojazdow/2+1; i++) {
                pojazdy[i].ustawNaZajezdni();
                Zdarzenie przyjazdTramwaju = new PrzyjazdTramwaju(czasOdstepu*i,dzien,pojazdy[i],kolejka);
                kolejka.dodajZdarzenie(przyjazdTramwaju);
            }
            int mnoznik = 0;
            for (int i=liczbaPojazdow/2+1; i<liczbaPojazdow; i++) {
                pojazdy[i].ustawNaZajezdni();
                Zdarzenie przyjazdTramwaju = new PrzyjazdTramwaju(czasOdstepu*mnoznik,dzien,pojazdy[i],kolejka);
                kolejka.dodajZdarzenie(przyjazdTramwaju);
                mnoznik++;
            }
        }
    }

    public void setLinie (Tramwaj[] Tramwaje) {
        this.pojazdy = Tramwaje;
    }
    public int getNumerTramwaju(int i){
        return pojazdy[i].getNumerBoczny();
    }
    public Przystanek getPrzystanekNaLini(int i) {
        return przystanki[i];
    }
    public int getCzasPodrozy(int i) {
        return czasPodrozy[i];
    }
    public int getNumerLini() {
        return numerLini;
    }
    public Przystanek getPoczatek(){
        return przystanki[0];
    }
    public Przystanek getKoniec(){
        return przystanki[przystanki.length-1];
    }
    public int getLiczbaPojazdow() {
        return liczbaPojazdow;
    }
    public int getDlugoscTrasy() {
        return dlugoscTrasy;
    }
}
