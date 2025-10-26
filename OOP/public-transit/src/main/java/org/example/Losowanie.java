package org.example;

// Klasa losowania
public class Losowanie {
    public static int losuj(int min, int max) {
        int liczbaLosowa = min + (int) (Math.random() * (max - min + 1));
        return liczbaLosowa;
    }

    public static Przystanek losujPrzystanek(Przystanek[] przystanki) {
        int randomIndex = (int) (Math.random() * przystanki.length);
        return przystanki[randomIndex];
    }

    public static int losujCzasPrzyjscia () {
        int min = 0;
        int max = 360; // Maksymalna i minimalna godzina przyjscia w minutach od 6
        return min + (int) (Math.random() * (max - min) + 1);
    }
    public static Przystanek losujPrzystanekWyjscia(Linia linia, Przystanek obecnyPrzystanek, Przystanek kierunek) {
        if (kierunek.equals(linia.getKoniec())) {
            int indeksOd = linia.getIndeksPrzystanku(obecnyPrzystanek) + 1;
            if (indeksOd>=linia.getDlugoscTrasy()) {
                return null;
            }
            int indeksDocelowy = losuj(indeksOd,linia.getDlugoscTrasy()-1);
            return linia.getPrzystanekNaLini(indeksDocelowy);
        }
        else {
            int indeksDo = linia.getIndeksPrzystanku(obecnyPrzystanek) - 1;
            if (indeksDo<0) {
                return null;
            }
            int indeksDocelowy = losuj(0,indeksDo);
            return linia.getPrzystanekNaLini(indeksDocelowy);
        }
    }
}
