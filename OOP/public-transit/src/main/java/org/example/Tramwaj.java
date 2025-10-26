package org.example;

public class Tramwaj {
    private int obecnaPojemnosc=0;
    private final int maksymalnaPojemnosc;
    private final Linia linia;
    private final int numerBoczny;
    private final Przystanek zajezdnia;
    private final Przystanek koniecPetli;
    private Przystanek kierunek;
    private Przystanek obecnyPrzystanek;

    Pasazer[] pasazerowie;
    public Tramwaj(int maksymalnaPojemnosc, Linia linia, Przystanek zajezdnia, Przystanek koniecPetli , int numerBoczny) {
        this.maksymalnaPojemnosc=maksymalnaPojemnosc;
        this.linia=linia;
        this.zajezdnia=zajezdnia;
        this.numerBoczny=numerBoczny;
        this.koniecPetli=koniecPetli;
        this.kierunek=koniecPetli;
        this.pasazerowie= new Pasazer[maksymalnaPojemnosc];
    }

    public void pobierzPasazerow (Przystanek przystanek, Liczyciel liczyciel, int czasPrzyjazdu, int dzien) {
        int i=obecnaPojemnosc;
        while (i<maksymalnaPojemnosc && przystanek.getIloscOczekujacych() > 0) {
            Przystanek przystanekWyjscia = Losowanie.losujPrzystanekWyjscia(linia,przystanek,kierunek);
            if (przystanekWyjscia==null) {
                // Nie mam dla pasazerow juz pozostalych przystankow na lini
                break;
            }
            pasazerowie[i] = przystanek.zabierOstatniego();

            pasazerowie[i].setPrzystanekWyjscia(przystanekWyjscia);

            liczyciel.zwiekszLacznyCzasOczekiwaniaDnia(czasPrzyjazdu - pasazerowie[i].getCzasPrzyjscia());
            liczyciel.zwiekszLiczbePrzejazdowDnia();

            System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czasPrzyjazdu) + " Pasazer: " + pasazerowie[i].getId()
                    + " wsiada z zamiarem wyjscia na przystanku: " + pasazerowie[i].getPrzystanekWyjscia().getNazwa() + " do tramwaju numer: " + numerBoczny);
            i++;
            obecnaPojemnosc++;
        }

    }

    public void wypuscPasazerow(Liczyciel liczyciel, int czas, int dzien) {
        for (int i = 0; i < obecnaPojemnosc; i++) {
            Pasazer pasazer = pasazerowie[i]; // Pobieramy pasażera z tramwaju
            if (pasazer.getPrzystanekWyjscia() == obecnyPrzystanek && obecnyPrzystanek.getPojemnosc() != obecnyPrzystanek.getIloscOczekujacych()) { // Sprawdzamy, czy pasażer ma wysiąść na tym przystanku
                // Pobieram czas oczekiwania od obecnego pasazera
                liczyciel.zwiekszWejsciaNaPrzystanek();
                System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czas) + " Pasazer: " + pasazerowie[i].getId()
                        + " wysiada na przystanku: " +  obecnyPrzystanek.getNazwa());
                pasazerowie[i] = null; // Usuwamy pasażera z tramwaju
                obecnaPojemnosc--; // Zmniejszamy liczbę pasażerów w tramwaju
                obecnyPrzystanek.dodajPasazera(pasazer,czas);
            }
        }
        // Przesun wszystkie nulle na lewo
        int tyl = maksymalnaPojemnosc-1;
        int przod = 0;
        while (przod <= tyl) {
            if (pasazerowie[przod] != null)
                ++przod;
            else if (pasazerowie[tyl] == null)
                --tyl;
            else {
                //zamien(pasazerowie, przod, tyl);
                Pasazer temp = pasazerowie[przod];
                pasazerowie[przod] = pasazerowie[tyl];
                pasazerowie[tyl] = temp;
                ++przod;
                --tyl;
            }
        }
    }

    // Funkcja odpowiedzialna za dodanie zdarzen zwiazanych z jazda tramwajem
    public void jedz(KolejkaPriorytetowa kolejka, int czas, int dzien) {
        int indeksObecnego = linia.getIndeksPrzystanku(obecnyPrzystanek);
        if(kierunek.equals(linia.getKoniec())) {
            int indeksNastepnego = indeksObecnego + 1;
            if (indeksNastepnego < linia.getDlugoscTrasy()) {
                int czasDojazduNaNastepny = linia.getCzasPodrozy(indeksObecnego);
                obecnyPrzystanek = linia.getPrzystanekNaLini(indeksNastepnego);
                Zdarzenie noweZdarzenie = new PrzyjazdTramwaju(czas+czasDojazduNaNastepny,dzien,this,kolejka);
                kolejka.dodajZdarzenie(noweZdarzenie);
            }
            // Zawracam na petli
            else {
                int czasZawrotki = linia.getCzasPodrozy(linia.getDlugoscTrasy()-1);
                if (kierunek.equals(koniecPetli)) {
                    kierunek = zajezdnia;
                }
                else {
                    kierunek = koniecPetli;
                }
                Zdarzenie noweZdarzenie = new PrzyjazdTramwaju(czas+czasZawrotki,dzien,this,kolejka);
                kolejka.dodajZdarzenie(noweZdarzenie);
            }
        }
        else {
            int indeksPoprzedniego = indeksObecnego - 1;
            if (indeksPoprzedniego >= 0) {
                //System.out.println("Tramwaj: " + numerBoczny + "jedzie do tylu");
                int czasDojazduNaPoprzedni = linia.getCzasPodrozy(indeksPoprzedniego);
                obecnyPrzystanek = linia.getPrzystanekNaLini(indeksPoprzedniego);
                Zdarzenie noweZdarzenie = new PrzyjazdTramwaju(czas+czasDojazduNaPoprzedni,dzien,this,kolejka);
                kolejka.dodajZdarzenie(noweZdarzenie);
            }
            // Zawracam na zajezdni
            else {
                int czasZawrotki = linia.getCzasPodrozy(linia.getDlugoscTrasy()-1);
                if (kierunek.equals(koniecPetli)) {
                    kierunek = zajezdnia;
                }
                else {
                    kierunek = koniecPetli;
                }
                Zdarzenie noweZdarzenie = new PrzyjazdTramwaju(czas+czasZawrotki,dzien,this,kolejka);
                kolejka.dodajZdarzenie(noweZdarzenie);
            }
        }
    }

    // Wracam tramwaj do zajezdni
    public void wrocTramwaj () {
        for (int i=0; i<maksymalnaPojemnosc; i++) {
            if (pasazerowie[i] != null) {
                pasazerowie[i].wroc();
                pasazerowie[i]=null;
            }
        }
        this.obecnyPrzystanek=null;
        this.obecnaPojemnosc=0;
        this.kierunek=koniecPetli;
    }

    // Ustawia obecny trawmaj na poczatku trasy
    public void ustawNaZajezdni() {
        this.obecnyPrzystanek=zajezdnia;
        this.obecnaPojemnosc=0;
        this.kierunek=koniecPetli;
    }
    public Przystanek getZajezdnia() {
        return zajezdnia;
    }

    public Przystanek getObecnyPrzystanek() {
        return obecnyPrzystanek;
    }
    public int getNumerBoczny() { return numerBoczny; }
}
