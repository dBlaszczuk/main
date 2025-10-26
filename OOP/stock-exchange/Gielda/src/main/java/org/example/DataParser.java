package org.example;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

// Klasa odpowiedzialna za czytanie danych z wejscia
public class DataParser {

    private ArrayList<String> investors = new ArrayList<>();
    private HashMap<String, Integer> stockPrices = new HashMap<>();
    private HashMap<String, Integer> initialPortfolio = new HashMap<>();
    private int initialCash = 0;
    public DataParser(String file) {
        wczytajDane(file);
    }

    public ArrayList<String> getInvestors() {
        return investors;
    }

    public HashMap<String, Integer> getInitialStockPrices() {
        return stockPrices;
    }

    public HashMap<String, Integer> getInitialPortfolio() {
        return initialPortfolio;
    }

    public int getInitialCash() {
        return initialCash;
    }

    // Czyta plik z wejścia
    private void wczytajDane(String file) {
        String fileName = file; // Np "dane.txt";

        try (BufferedReader br = new BufferedReader(new FileReader(fileName))) {
            String line;
            int lineNumber = 0;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                // Zignoruj komentarze i puste linie
                if (line.startsWith("#") || line.isEmpty()) {
                    continue;
                }
                // Zwieksz licznik lini zawierajacych interesujace nas dane
                lineNumber++;
                // Przejdz po wszystkich tokenach w danej lini w celu uzyskania danych
                String[] tokens = line.split(" ");
                // Wczytaj typy inwestorów
                if (lineNumber == 1) {
                    for (String token : tokens) {
                        investors.add(token);
                    }
                }
                // Wczytaj typy akcji wraz z cenami ostatnich transakcji
                else if (lineNumber == 2) {
                    for (String token : tokens) {
                        String[] pair = token.split(":");
                        if (pair.length == 2) {
                            String key = pair[0];
                            int value = Integer.parseInt(pair[1]);

                            stockPrices.put(key,value);
                        }
                        else {
                            System.out.println("Nieprawidłowe dane!");
                            System.exit(1);
                        }
                    }
                }
                // Wczytaj początkowy stan portweli
                else if (lineNumber == 3) {
                    for (String token : tokens) {
                        if (token.contains(":")) {
                            String[] pair = token.split(":");
                            if (pair.length == 2) {
                                String key = pair[0];
                                int quantity = Integer.parseInt(pair[1]);
                                initialPortfolio.put(key,quantity);
                            }
                            else {
                                System.out.println("Nieprawidłowe dane!");
                                System.exit(1);
                            }
                        }
                        else {
                            try {
                                initialCash = Integer.parseInt(token);
                            } catch (NumberFormatException e) {
                                System.out.println("Nieprawidłowe dane!");
                                System.exit(1);
                            }
                        }
                    }
                }
                // Zła ilość lini danych
                else {
                    System.out.println("Nieprawidłowe dane!");
                    System.exit(1);
                }
            }

            sprawdzPoprawnoscAkcji(stockPrices,initialPortfolio);
            System.out.println("Inwestorzy: " + investors);
            System.out.println("Ceny Akcji: " + stockPrices);
            System.out.println("Początkowe portfolio: " + initialPortfolio);
            System.out.println("Początkowe pieniądze: " + initialCash);

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    private static void sprawdzPoprawnoscAkcji(HashMap<String, Integer> stockPrices, HashMap<String, Integer> initialPortfolio) {
        try {
            MissingKeyException.checkKeys(initialPortfolio, stockPrices);
        }
        catch (MissingKeyException mke){
            System.out.println("Nieprawidłowe dane!");
            System.exit(1);
        }
    }
}
