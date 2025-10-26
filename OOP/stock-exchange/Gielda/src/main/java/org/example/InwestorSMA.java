package org.example;

import java.util.*;

import static org.example.TypZlecenia.BEZTERMINOWE;

// Inwestor korzystający ze strategii sma
public class InwestorSMA extends Inwestor {
    private HashMap<String, Integer[]> stockHistory;

    public InwestorSMA(HashMap<String, Integer> stockPrices, HashMap<String, Integer> initialPortfolio, int initialCash, Gielda stockExchange) {
        super(stockPrices, initialPortfolio, initialCash, stockExchange);
        this.stockHistory = new HashMap<>();
        for (String stock : stockPrices.keySet()) {
            Integer[] history = new Integer[10];
            Arrays.fill(history, 0);
            this.stockHistory.put(stock, history);
        }
    }

    // Metoda odpowiedzialna za odpowiednie ustawianie tablicy historii
    private void updateTable (Integer[] history, int price) {
        for (int i = 0; i<9; i++) {
            history[i] = history [i+1];
        }
        history[9] = price;
    }

    // Wykonanie ruchu na podstawie sma
    @Override
    public void makeMove(Gielda stockExchange) {
        for (String stock : wallet.keySet()) {
            updateTable(stockHistory.get(stock), stockExchange.getLastPrice(stock));
            if (stockExchange.getCurrentTurn() > 10) {
                int sma5 = calculateSMA(stockHistory.get(stock), 5);
                int sma10 = calculateSMA(stockHistory.get(stock), 10);

                if (sma5 > sma10) {
                    Zlecenie zlecenie = new Zlecenie(stock, stockExchange.getLastPrice(stock), true, BEZTERMINOWE, cash/stockExchange.getLastPrice(stock), this);
                    stockExchange.addOrder(zlecenie);
                    break;
                }
                else if (sma5 < sma10) {
                    Zlecenie zlecenie = new Zlecenie(stock, stockExchange.getLastPrice(stock), false, BEZTERMINOWE, wallet.get(stock), this);
                    stockExchange.addOrder(zlecenie);
                    break;
                }
            }
        }
    }

    // Obliczenie sma dla n (<10) tur
    private int calculateSMA(Integer[] prices, int n) {
        if (prices.length < n) return 0;
        int sum = 0;
        for (int i = 0; i < n; i++) {
            sum += prices[i];
        }
        return sum / n;
    }

}
