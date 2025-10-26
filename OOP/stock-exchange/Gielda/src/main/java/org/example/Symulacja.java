package org.example;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

// Objekt odpowiedzialny za przeprowadzenie symulacji
public class Symulacja {

    public static void przeprowadz(String fileName, int turns){
        DataParser parser = new DataParser(fileName);

        ArrayList<String> investorsCode = parser.getInvestors();
        HashMap<String, Integer> initialStockPrices = parser.getInitialStockPrices();
        HashMap<String, Integer> initialPortfolio = parser.getInitialPortfolio();
        int initialCash = parser.getInitialCash();

        Gielda stockExchange = new Gielda(initialStockPrices);

        // Inicjalizacja odpowiednich typow inwestorow
        ArrayList<Inwestor> investors = new ArrayList<>();
        for (String inwestor : investorsCode) {
            if (inwestor.equals("R")) {
                Inwestor tempInvestor = new InwestorRANDOM(initialStockPrices,initialPortfolio, initialCash, stockExchange);
                investors.add(tempInvestor);
            }
            else if (inwestor.equals("S")) {
                Inwestor tempInvestor = new InwestorSMA(initialStockPrices, initialPortfolio, initialCash, stockExchange);
                investors.add(tempInvestor);
            }
            else {
                System.out.println("Nieznany kod inwestora!");
                System.exit(1);
            }
        }

        ArrayList<Inwestor> shuffledInvestors = new ArrayList<>(investors);
        // Przeprowadzenie symulacji
        for (int i=0; i<turns; i++) {
            for (Inwestor investor : shuffledInvestors) {
                Collections.shuffle(shuffledInvestors); // Tasowanie listy
                investor.makeMove(stockExchange);
            }
            stockExchange.computeTurn();
        }

        // Wydrukowanie wyników eksperymentu
        for (Inwestor investor : investors) {
            investor.printWallet();
        }
    }
}
