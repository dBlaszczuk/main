package org.example;

import java.util.Scanner;

// Klasa odpowiedzialna za wczytanie danych
public class Wczytaj {
    private static Scanner scanner = new Scanner(System.in);

    public static int wczytajDni() {
        return scanner.nextInt();
    }
    public static int wczytajPojemnosc() {
        return scanner.nextInt();
    }
    public static int wczytajLiczbePrzystankow(){
        return scanner.nextInt();
    }

    public static Przystanek[] wczytajPrzystanki(int liczbaPrzystankow, int pojemnosc){
        Przystanek[] przystanki = new Przystanek[liczbaPrzystankow];

        for (int i = 0; i<liczbaPrzystankow; i++) {
            Przystanek nowy = new Przystanek(scanner.next(), pojemnosc);
            przystanki[i] = nowy;
        }
        return przystanki;
    }
    // Tutaj liczba pasazerow
    public static int liczbaPasazerow () {
        return scanner.nextInt();
    }

    // Pojemnośc pojazdu (w tym przypadku tramwaju)
    public static int pojemnoscPojazdu () {
        return scanner.nextInt();
    }

    // Lizcba lini (tramwajowych)
    public static int liczbaLiniPojazdu () {
        return scanner.nextInt();
    }

    // Metoda odpowiedzialna za stworzenie wielu lini tego  samego pojazdu
    public static Linia[] liniePojazdu (int iloscLini, Przystanek[] przystanki) {
        // tymczasowo
        int numerLini=1;

        Linia[] linie = new Linia[iloscLini];
        for (int i = 0; i<iloscLini; i++) {

            int liczbaPojazdow = scanner.nextInt();
            int iloscPrzystankow = scanner.nextInt(); // Dlugosc trasy
            System.out.println("Przystanki na lini: ");
            Przystanek[] rozkladJazdy = new Przystanek[iloscPrzystankow];
            int[] czasDojazduTablica = new int[iloscPrzystankow];

            for (int j = 0; j<iloscPrzystankow; j++) {
                String nazwaPrzystanku = scanner.next();
                System.out.println(nazwaPrzystanku);
                int czasDojazdu = scanner.nextInt();
                czasDojazduTablica[j]=czasDojazdu;
                for (int k = 0; k<przystanki.length; k++) {
                    if (przystanki[k].getNazwa().equals(nazwaPrzystanku)) {
                        rozkladJazdy[j] = przystanki[k];
                        break;
                    }
                }
            }
            linie[i] = new Linia(numerLini,liczbaPojazdow,iloscPrzystankow,rozkladJazdy,czasDojazduTablica);
            numerLini++;
        }
        return linie;
    }
}
