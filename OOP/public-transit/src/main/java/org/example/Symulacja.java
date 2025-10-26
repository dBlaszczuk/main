package org.example;

public class Symulacja {

    // Metoda odpowiedzialna za przeprowadzenie symulacji
    public static void przeprowadz() {
        int dni = Wczytaj.wczytajDni();

        System.out.println("Ilosc dni symulacji: " + dni);

        int pojemnoscPrzystanku = Wczytaj.wczytajPojemnosc();

        System.out.println("Pojemnosc kazdego przystanku: " + pojemnoscPrzystanku);

        int liczbaPrzystankow = Wczytaj.wczytajLiczbePrzystankow();

        System.out.println("Liczba przystankow: " + liczbaPrzystankow);

        Przystanek[] przystanki = Wczytaj.wczytajPrzystanki(liczbaPrzystankow, pojemnoscPrzystanku);

        for (int i=0; i<liczbaPrzystankow; i++) {
            System.out.println("Nazwa przystanku: " + przystanki[i].getNazwa());
        }

        int liczbaPasazerow = Wczytaj.liczbaPasazerow();

        System.out.println("Liczba pasazerow: " + liczbaPasazerow);

        int pojemnoscTramwaju = Wczytaj.pojemnoscPojazdu();

        System.out.println("Pojemnosc tramwaju: " + pojemnoscTramwaju);

        int liczbaLini = Wczytaj.liczbaLiniPojazdu();

        System.out.println("Liczba lini: " + liczbaLini);

        // Od teraz znamy linie
        Linia[] linieTramwajowe = Wczytaj.liniePojazdu(liczbaLini,przystanki);

        Pasazer[] pasazerowie = generujPasazerow(liczbaPasazerow,przystanki);

        // Stworzenie lini tramwajowych
        stworzTramwaje(linieTramwajowe, pojemnoscTramwaju);

        KolejkaPriorytet kolejka = new KolejkaPriorytet();

        // Klasa pozwalajaca na liczenie danych z tresci zadania
        Liczyciel liczyciel = new Liczyciel();

        for (int i = 0; i<dni; i++) {
            symulujDzien(linieTramwajowe,pasazerowie, liczbaLini,liczbaPasazerow,kolejka, liczyciel, i);

            System.out.println("Łączny czas czekania na przystanku dnia: " + i + " to: " + liczyciel.getLacznyCzasOczekiwaniaDnia());

            System.out.println("Łączna liczba przejazdów dnia " + i + " to " + liczyciel.getLacznaLiczbaPrzejazdowDnia());

            liczyciel.resetujLicznikPodKoniecDnia();

            przywrocStanPoczatkowy(linieTramwajowe, liczbaLini, przystanki, liczbaPrzystankow, liczyciel);
        }

        System.out.println("Łączna liczba przejazdow pasazerow: " + liczyciel.getLacznaLiczbaPrzejazdow());
        
        System.out.println("Średni czas oczekiwania: " + liczyciel.obliczSredniCzasOczekiwania() + " minut");
    }

    // Metoda odpowiedzialna za generacje pasazerow zgodnie ze specyfikacja zadania
    private static Pasazer[] generujPasazerow(int liczbaPasazerow, Przystanek[] przystanki) {
        Pasazer[] pasazerowie = new Pasazer[liczbaPasazerow];
        for (int i = 0; i<liczbaPasazerow; i++) {
            Pasazer nowyPasazer = new Pasazer(Losowanie.losujPrzystanek(przystanki),i);
            pasazerowie[i]=nowyPasazer;
        }
        return pasazerowie;
    }

    // Metoda odpowiedzialana za symulacje dnia zgodnie z kolejnoscia kolejki zdarzen
    private static void symulujDzien(Linia[] linie, Pasazer[] pasazerowie, int liczbaLini,int liczbaPasazerow, KolejkaPriorytetowa kolejka, Liczyciel liczyciel, int dzien) {
        int minutyOd6=0;
        for(int i=0; i<liczbaLini; i++) {
            linie[i].wypuscTramwaje(kolejka, dzien);
        }
        for (int i=0; i<liczbaPasazerow; i++) {
            pasazerowie[i].idzNaPrzystanek(kolejka, dzien);
        }
        while (!kolejka.jestPusta()) {
            kolejka.getNastepneZdarzenie().wykonajZdarzenie(liczyciel);
        }
    }

    // Metoda wracajaca ludzi znajdujacych sie w tramwajach i na przystankach; nie rusza ludzi siedzach w domu
    private static void przywrocStanPoczatkowy(Linia[] linie, int liczbaLini, Przystanek[] przystanki, int liczbaPrzystankow, Liczyciel liczyciel) {
        for (int i=0; i<liczbaLini; i++) {
            linie[i].wrocTramwaje();
        }
        for (int i=0; i<liczbaPrzystankow; i++) {
            przystanki[i].wrocPasazerow(liczyciel);
        }
    }

    // Metoda odpowiedzialna za stworzenie tramwaji dla kazdej linii
    private static void stworzTramwaje(Linia[] linieTramwajowe, int pojemnoscTramwaju) {
        int numerBoczny=0;
        for (int i=0; i < linieTramwajowe.length; i++) {
            Tramwaj[] dlaLini = new Tramwaj[linieTramwajowe[i].getLiczbaPojazdow()];
            if(linieTramwajowe[i].getLiczbaPojazdow() % 2 == 0) {
                for (int j=0; j<linieTramwajowe[i].getLiczbaPojazdow()/2; j++) {
                    Tramwaj nowy = new Tramwaj(pojemnoscTramwaju,linieTramwajowe[i],linieTramwajowe[i].getPoczatek(), linieTramwajowe[i].getKoniec(),numerBoczny);
                    dlaLini[j] = nowy;
                    numerBoczny++;
                }
                for (int j=linieTramwajowe[i].getLiczbaPojazdow()/2; j<linieTramwajowe[i].getLiczbaPojazdow(); j++) {
                    Tramwaj nowy = new Tramwaj(pojemnoscTramwaju,linieTramwajowe[i],linieTramwajowe[i].getKoniec(), linieTramwajowe[i].getPoczatek(),numerBoczny);
                    dlaLini[j] = nowy;
                    numerBoczny++;
                }
            }
            else {
                for (int j = 0; j < linieTramwajowe[i].getLiczbaPojazdow() / 2 + 1; j++) {
                    Tramwaj nowy = new Tramwaj(pojemnoscTramwaju, linieTramwajowe[i], linieTramwajowe[i].getPoczatek(), linieTramwajowe[i].getKoniec(), numerBoczny);
                    dlaLini[j] = nowy;
                    numerBoczny++;
                }
                for (int j = linieTramwajowe[i].getLiczbaPojazdow() / 2 + 1; j < linieTramwajowe[i].getLiczbaPojazdow(); j++) {
                    Tramwaj nowy = new Tramwaj(pojemnoscTramwaju, linieTramwajowe[i], linieTramwajowe[i].getKoniec(), linieTramwajowe[i].getPoczatek(), numerBoczny);
                    dlaLini[j] = nowy;
                    numerBoczny++;
                }
            }
            linieTramwajowe[i].setLinie(dlaLini);
        }
    }
}

