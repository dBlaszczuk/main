package org.example;

import java.util.HashMap;
import java.util.Random;

import static org.example.TypZlecenia.WAZNE_DO_KONCA_TURY;

// Inwestor wykorzystujący strategie maksymalizacji losowości
public class InwestorRANDOM extends Inwestor{

    public InwestorRANDOM(HashMap<String, Integer> stockPrices, HashMap<String, Integer> initialPortfolio, int initialCash, Gielda stockExchange) {
        super(stockPrices, initialPortfolio, initialCash, stockExchange);
    }


    @Override
    public void makeMove(Gielda stockExchange) {
        Random rand = new Random();

        // Decydujemy, czy wykonać ruch
        if (rand.nextBoolean()) {
            // Wybieramy losową spółkę z portfela
            String[] stocks = wallet.keySet().toArray(new String[0]);
            String randomStock = stocks[rand.nextInt(stocks.length)];

            // Decydujemy, czy kupić, czy sprzedać
            boolean buy = rand.nextBoolean();

            // Wybieramy losowy typ zlecenia
            TypZlecenia[] orderType = TypZlecenia.values();
            TypZlecenia randomType = orderType[rand.nextInt(orderType.length)];

            // Losujemy cenę
            int price = stockExchange.getLastPrice(randomStock);
            if (rand.nextBoolean()) {
                price += rand.nextInt(21) - 10;
            }
            else {
                price -= rand.nextInt(21) + 10;
            }
            price = Math.max(price, 1);

            // Wybieramy ilość sprzedarzy/kupna
            int quantity = rand.nextInt(getCash()/price) + 1;

            // Tworzymy zlecenie i wysyłamy je do giełdy
            if (randomType != WAZNE_DO_KONCA_TURY) {
                Zlecenie zlecenie = new Zlecenie(randomStock, price, buy, randomType, quantity, this);
                stockExchange.addOrder(zlecenie);
            }
            else {
                int range = Integer.MAX_VALUE - stockExchange.getCurrentTurn();
                int expirationTurn = stockExchange.getCurrentTurn() + rand.nextInt(range);
                Zlecenie zlecenie = new Zlecenie(randomStock, price, buy, randomType, quantity, this, expirationTurn);
                stockExchange.addOrder(zlecenie);
            }
        }
    }
}
