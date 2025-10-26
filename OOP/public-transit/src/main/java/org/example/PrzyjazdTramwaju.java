package org.example;

// Zdarzenia zwiazane z przyjazdem tramwaju
public class PrzyjazdTramwaju extends Zdarzenie {
    private final Tramwaj tramwaj;

    private final KolejkaPriorytetowa kolejka;

    public PrzyjazdTramwaju(int czas, int dzien, Tramwaj tramwaj, KolejkaPriorytetowa kolejka) {
        super(czas, dzien);
        this.tramwaj = tramwaj;
        this.kolejka = kolejka;
    }

    @Override
    public void wykonajZdarzenie(Liczyciel liczyciel) {
        // Implementacja logiki związanej z przyjazdem tramwaju
        int godzina23 = 1020;
        int godzina24 = 1080;
        if (czas> godzina23 && tramwaj.getObecnyPrzystanek().equals(tramwaj.getZajezdnia())) {
            System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czas) +
                    " Tramwaj numer: " + tramwaj.getNumerBoczny() + " wraca do zajedni.");
            tramwaj.wrocTramwaj();
        }
        else {
            System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czas) +
                    " Tramwaj numer: " + tramwaj.getNumerBoczny() + " przyjechal na: "
                    + tramwaj.getObecnyPrzystanek().getNazwa());

            tramwaj.wypuscPasazerow(liczyciel, czas, dzien);

            tramwaj.pobierzPasazerow(tramwaj.getObecnyPrzystanek(), liczyciel, czas, dzien);


            if (czas < godzina24) {
                tramwaj.jedz(kolejka,czas,dzien);
            }
        }
    }

    public int getCzas() {
        return czas;
    }
}
