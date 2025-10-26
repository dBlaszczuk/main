package org.example;

// Klasa zlecenia
public class Zlecenie {
    private String stock;
    private int price;
    private boolean buy;
    private TypZlecenia type;
    private int expirationTurn;
    private int quantity;
    private Inwestor investor;

    // Konstruktor dla standardowego typu zlecenia
    public Zlecenie(String stock, int price, boolean buy, TypZlecenia type, int quantity, Inwestor investor) {
        this.stock = stock;
        this.price = price;
        this.buy = buy;
        this.type = type;
        this.quantity = quantity;
        this.investor = investor;
    }

    // Konstruktor dla typu zlecenia wygasającego
    public Zlecenie(String stock, int price, boolean buy, TypZlecenia type, int quantity, Inwestor investor, int expirationTurn) {
        this.stock = stock;
        this.price = price;
        this.buy = buy;
        this.type = type;
        this.quantity = quantity;
        this.investor = investor;
        this.expirationTurn = expirationTurn;
    }
    public Inwestor getInvestor() {
        return investor;
    }

    public String getStockName() {
        return stock;
    }

    public int getPrice() {
        return price;
    }

    public boolean isBuy() {
        return buy;
    }

    public TypZlecenia getType() {
        return type;
    }

    public boolean isEmpty() {
        return quantity == 0;
    }

    public int getQuantity() {
        return quantity;
    }

    public void setQuantity(int quantity) {
        this.quantity = quantity;
    }

    public int getExpirationTurn() {
        return expirationTurn;
    }
}
