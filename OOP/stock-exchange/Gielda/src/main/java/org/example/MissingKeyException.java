package org.example;

import java.util.HashMap;

// Definicja klasy wyjątku brakującego klucza
class MissingKeyException extends Exception {
    public MissingKeyException(String message) {
        super(message);
    }

    public static void checkKeys(HashMap<String, Integer> map1, HashMap<String, Integer> map2) throws MissingKeyException {
        for (String key : map2.keySet()) {
            if (!map1.containsKey(key)) {
                throw new MissingKeyException("Brakuje klucza " + key + " w mapie 2");
            }
        }
    }
}