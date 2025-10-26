package org.example;

public class Main {
    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println("Niepoprawne argumenty!!");
            System.exit(1);
        }

        String fileName = args[0];
        int simulationTurns = 0;

        // Sprawdzenie rozszerzenia pliku
        if (!fileName.endsWith(".txt")) {
            System.err.println("Nieporpawnie rozszerzenie pliku .txt!");
            System.exit(1);
        }

        try {
            simulationTurns = Integer.parseInt(args[1]);
        } catch (NumberFormatException e) {
            System.err.println("Niepoprawne dni symulacji!");
            System.exit(1);
        }

        Symulacja.przeprowadz(fileName, simulationTurns);
    }
}