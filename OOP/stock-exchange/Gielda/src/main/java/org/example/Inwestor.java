package org.example;
import java.util.HashMap;

// Klasa abstrakcyjna inwestor
public abstract class Inwestor {
    protected HashMap<String, Integer> wallet;
    protected int cash;
    protected Gielda stockExchange;

    public HashMap<String, Integer> getWallet() {
        return wallet;
    }

    public int  getCash () {
        return cash;
    }

    public void setCash (int cash) {
        this.cash = cash;
    }
    public Inwestor(HashMap<String, Integer> stockPrices, HashMap<String, Integer> initialPortfolio, int initialCash, Gielda stockExchange) {
        this.cash = initialCash;
        this.wallet = new HashMap<>(initialPortfolio); // Kopiujemy początkowy portfel
        this.stockExchange=stockExchange;
        // Dodajemy akcje z stockPrices do initialPortfolio, jeśli ich tam jeszcze nie ma
        for (String stock : stockPrices.keySet()) {
            initialPortfolio.putIfAbsent(stock, 0);
        }
    }
    public abstract void makeMove(Gielda stockExchange);

    // Drukujemy zawartość portfolio
    public void printWallet() {
        System.out.print(cash);
        System.out.print(" ");
        for (String stock : wallet.keySet()) {
            System.out.print(stock + ": " + wallet.get(stock) + " ");
        }
        System.out.println();
    }
}
