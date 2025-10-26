package org.example;

// Zdarzenia zwiazane z przyjsciem pasazera na przystanek
public class PrzyjsciePasazera extends Zdarzenie {
    private final Pasazer pasazer;

    public PrzyjsciePasazera(int czas,int dzien, Pasazer pasazer) {
        super(czas, dzien);
        this.pasazer = pasazer;
    }

    @Override
    public void wykonajZdarzenie(Liczyciel liczyciel) {
        // Implementacja logiki związanej z przyjściem pasażera
        if (pasazer.getPrzystanekDom().getPojemnosc()==pasazer.getPrzystanekDom().getIloscOczekujacych()) {
            System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czas) + " Pasazer:" + pasazer.getId() + "nie znalazl miejsca na przystanku");
            pasazer.wroc();
        }
        else {
            System.out.println(dzien + " " + liczyciel.zmienNaCzasDobowy(czas) + " Pasazer numer: " + pasazer.getId() + " przychodzi na: " + pasazer.getPrzystanekDom().getNazwa() );
            pasazer.getPrzystanekDom().dodajPasazera(pasazer, czas);
            liczyciel.zwiekszWejsciaNaPrzystanek();
        }
    }
    public int getCzas() {
        return czas;
    }
}
