package org.example;

import java.util.Comparator;
import java.util.PriorityQueue;

// Klasa odpowiedzialna za przechowywanie zleceń kupna/Sprzedaży akcji
public class ZleceniaAkcji {
    private final String name;
    private PriorityQueue<Zlecenie> buyOrders;
    private PriorityQueue<Zlecenie> sellOrders;

    public ZleceniaAkcji(String name) {
        this.name = name;

        // Kolejność malejąca dla zleceń kupna (najwyższa cena pierwsza)
        this.buyOrders = new PriorityQueue<>(Comparator.comparingInt(Zlecenie::getPrice).reversed());

        // Kolejność rosnąca dla zleceń sprzedaży (najniższa cena pierwsza)
        this.sellOrders = new PriorityQueue<>(Comparator.comparingInt(Zlecenie::getPrice));
    }

    // Sprzedaż i kupno niepuste
    public boolean ordersNotEmpty() {
        return !buyOrders.isEmpty() && !sellOrders.isEmpty();
    }

    public void addOrder(Zlecenie order) {
        if (order.isBuy()) {
            buyOrders.add(order);
        } else {
            sellOrders.add(order);
        }
    }

    public void removeEmptyOrders() {
        buyOrders.removeIf(Zlecenie::isEmpty);
        sellOrders.removeIf(Zlecenie::isEmpty);
    }

    public void removeEmptyImmediateOrders() {
        // Usuwanie pustych zleceń typu NATYCHMIASTOWE z kolejki kupna/sprzedaży
        buyOrders.removeIf(order -> order.getType() == TypZlecenia.NATYCHMIASTOWE);
        sellOrders.removeIf(order -> order.getType() == TypZlecenia.NATYCHMIASTOWE);
    }

    public void removeExpiredOrders(int currentTurn) {
        // Usuwanie zleceń typu WAZNE_DO_KONCA_TURY, których czas wygaśnięcia jest mniejszy niż aktualna tura
        buyOrders.removeIf(order -> order.getType() == TypZlecenia.WAZNE_DO_KONCA_TURY && order.getExpirationTurn() < currentTurn);
        sellOrders.removeIf(order -> order.getType() == TypZlecenia.WAZNE_DO_KONCA_TURY && order.getExpirationTurn() < currentTurn);
    }



    public int getMaxBuyOrderPrice() {
        Zlecenie maxBuyOrder = buyOrders.peek();
        return (maxBuyOrder != null) ? maxBuyOrder.getPrice() : -1; // -1 gdy brak zleceń
    }

    public int getMinSellOrderPrice() {
        Zlecenie minSellOrder = sellOrders.peek();
        return (minSellOrder != null) ? minSellOrder.getPrice() : -1; // -1 gdy brak zleceń
    }
    public PriorityQueue<Zlecenie> getBuyOrders() {
        return buyOrders;
    }

    public PriorityQueue<Zlecenie> getSellOrders() {
        return sellOrders;
    }
}
