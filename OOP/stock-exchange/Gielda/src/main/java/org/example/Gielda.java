package org.example;

import java.util.HashMap;

// Klasa odpowiedzialna za symulacje procesow na gieldzie
public class Gielda {
    private HashMap<String, Integer> stocks;
    private int currentTurn=0;

    private HashMap<String, ZleceniaAkcji> orderMap;
    public Gielda (HashMap<String, Integer> stocks) {
        this.stocks=stocks;
        this.orderMap = new HashMap<>();
        for (String stock : stocks.keySet()) {
            orderMap.put(stock, new ZleceniaAkcji(stock));
        }
    }


    // Dodanie zlecenia
    public void addOrder(Zlecenie order) {
        String stockName = order.getStockName();
        ZleceniaAkcji stockOrders = orderMap.get(stockName);
        stockOrders.addOrder(order);
    }

    // Przeprowadzenie transakcji
    private void executeTransaction(Inwestor buyer, Inwestor seller, String stockName, int quantity, int price, Zlecenie buyOrder, Zlecenie sellOrder) {
        // Ustawiamy cene ostatniej transakcji
        stocks.put(stockName, price);

        // Dajemy kupującemu akcje, jeżeli ich nie ma to dodajemy typ akcji do portfela
        buyer.getWallet().put(stockName, buyer.getWallet().getOrDefault(stockName, 0) + quantity);
        // Ustawiamy ilość pieniędzy po transakcji
        buyer.setCash(buyer.getCash() - quantity * price);

        // Zabieramy sprzedajacemu akcje
        seller.getWallet().put(stockName, seller.getWallet().get(stockName) - quantity);
        // Ustawiamy ilość pieniędzy po transakcji
        seller.setCash(seller.getCash() + quantity * price);

        // Zmiejszamy ilość akcji w zleceniach
        buyOrder.setQuantity(buyOrder.getQuantity() - quantity);
        sellOrder.setQuantity(sellOrder.getQuantity() - quantity);
    }

    // Dopasowanie zlecen sprzedarzy do zlecenia kupna
    private void matchSellOrdersWith(ZleceniaAkcji orders, Zlecenie orderToMatch) {

        Inwestor buyer = orderToMatch.getInvestor();
        int buyPrice = orderToMatch.getPrice();
        //buyer.printWallet();
        // Sprawdzenie czy wykonanie zlecenia jest możliwe; jeżeli zostało wykonane w całości przejdź do kolejnego
        while (orders.ordersNotEmpty() && (buyPrice >= orders.getMinSellOrderPrice()) && (buyer.getCash() >= orders.getMinSellOrderPrice())) {
            Zlecenie sellOrder = orders.getSellOrders().peek();
            if (sellOrder != null && buyPrice >= sellOrder.getPrice() && buyer.getCash() >= sellOrder.getPrice() ) {
                int maxPossibleQuantity = checkMaxPossibleQuantity(orderToMatch, sellOrder, buyer.getCash());
                // Kupujący nie ma możliwości zakupu; usuwamy jego zlecenie
                if (maxPossibleQuantity == 0) {
                    removeOrder(orderToMatch);
                    return;
                }
                // Sprzedający nie ma wystarczającej ilości akcji
                if (!sellerHasEnoughQuantity(sellOrder.getInvestor(), sellOrder.getStockName() ,maxPossibleQuantity)) {
                    removeOrder(sellOrder);
                    return;
                }
                executeTransaction(buyer, sellOrder.getInvestor(), orderToMatch.getStockName(), maxPossibleQuantity, sellOrder.getPrice(), orderToMatch, sellOrder);
                orders.removeEmptyOrders();
            } else {
                break;
            }
        }
    }

    // Sprawdzenie czy sprzedający ma wystarczającą ilość akcji
    private boolean sellerHasEnoughQuantity (Inwestor seller, String stock, int quantity) {
        int stockQuantity = seller.getWallet().get(stock);
        return stockQuantity >= quantity;
    }

    // Sprawdzenie maksymalnej możliwej ilości akcji do sprzedania w transakcji
    private int checkMaxPossibleQuantity(Zlecenie buyOrder, Zlecenie sellOrder, int buyerCash) {
        // Przypadek oddania akcji za darmo
        int sellPrice = sellOrder.getPrice();
        int buyQuantity = buyOrder.getQuantity();
        if (sellPrice == 0) {
            return buyQuantity;
        }
        int sellQuantity = sellOrder.getQuantity();
        int maxOrders = Math.min(buyerCash/sellPrice, sellQuantity);
        if (sellQuantity > buyQuantity) {
            maxOrders = buyQuantity;
        }
        return maxOrders;
    }

    // Oblicz wynik działania tury
    public void computeTurn() {
        for (String name : orderMap.keySet()) {
            // bufor
            ZleceniaAkcji orders = orderMap.get(name);
            while (orders.ordersNotEmpty() && (orders.getMaxBuyOrderPrice() >= orders.getMinSellOrderPrice())) {
                Zlecenie buyOrder = orders.getBuyOrders().peek();
                if (buyOrder.getInvestor().getCash() < orders.getMinSellOrderPrice()) {
                    removeOrder(buyOrder);
                    continue;
                }
                matchSellOrdersWith(orders, buyOrder);
            }
        }
        // Usuwanie zleceń odpowiednich typów
        for (String name : orderMap.keySet()) {
            ZleceniaAkcji orders = orderMap.get(name);
            orders.removeExpiredOrders(currentTurn);
            orders.removeEmptyOrders();
            orders.removeEmptyImmediateOrders();
        }
        // Zwiekszenie licznika tury dla inwestorow
        currentTurn++;
    }

    // Usuń zlecenie
    private void removeOrder(Zlecenie order) {
        String stockName = order.getStockName();
        ZleceniaAkcji stockOrder = orderMap.get(stockName);
        if (stockOrder != null) {
            if (order.isBuy()) {
                stockOrder.getBuyOrders().remove(order);
            } else {
                stockOrder.getSellOrders().remove(order);
            }
        } else {
            // Handle the case where the stockName is not found
            System.out.println("Stock name not found: " + stockName);
        }
    }

    public int getCurrentTurn() {
        return currentTurn;
    }

    public int getLastPrice (String stock) {
        return stocks.get(stock);
    }
}
