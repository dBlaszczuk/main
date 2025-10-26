#include <stddef.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <nand.h>

// Definicja struktury NAND.
typedef struct nand nand_t;

// Struktura sygnalu.
typedef struct signal {
    bool* pointerToSignal;  // Wskaznik na wartosc sygnalu.
    nand_t* to;             // Wskaznik na bramke wejscia sygnalu.
    nand_t* from;           // Wskaznik na bramke z ktorej sygnal wychodzi.
} signal_t;

// Struktura listy sygnalow.
typedef struct node {
    signal_t* signal;       // Wskaznik na sygnal.
    struct node* next;
} node_t;

// Struktura bramki NAND
typedef struct nand {
    unsigned num_inputs;    // Liczba wejsc bramki.
    signal_t** inputs;      // Tablica wskaznikow na sygnaly wejsciowe.
    node_t* outputs;        // Lista sygnalow wyjsciowych
    bool evaluated;         // Flaga oznaczajaca czy bramka zostala ewaluowana.
    bool isevaluated;       // Flaga pomocnicza do detekcji cykli.
    int gate_val;           // Wartosc bramki (0, 1, -1 dla cykli).
    unsigned path_len;      // Dlugosc sciezki krytycznej.
} nand_t;

// Funkcja tworząca nowa bramke NAND.
nand_t* nand_new(unsigned n) {
    errno = 0;

    // Alokacja pamieci dla bramki NAND.
    nand_t* gate = (nand_t*)malloc(sizeof(nand_t));
    if (gate == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // Alokacja pamieci dla tablicy wejsc.
    gate->inputs = (signal_t**)malloc(n * sizeof(signal_t*));
    if (gate->inputs == NULL) {
        free(gate);
        errno = ENOMEM;
        return NULL;
    }

    // Inicjalizacja wejsc jako NULL.
    for (unsigned i = 0; i < n; ++i) {
        gate->inputs[i] = NULL;

    }

    // Inicjalizacja pozostalych pol struktowy.
    gate->outputs = NULL;
    gate->evaluated = false;
    gate->isevaluated = false;
    gate->gate_val = 2;
    gate->num_inputs = n;
    gate->path_len = 0;

    return gate;
}

// Funkcja odlaczajaca sygnal z wyjsc bramki.
void disconnect_signal_from_outputs(signal_t* signalToDisconnect) {
    if (signalToDisconnect == NULL || signalToDisconnect->from == NULL) return;
    node_t* current = signalToDisconnect->from->outputs;
    node_t* previous = NULL;

    // Przechodzimy przez liste i zwalniamy sygnal.
    while (current != NULL) {
        if (current->signal == signalToDisconnect) {
            if (previous == NULL) {
                signalToDisconnect->from->outputs =
                        signalToDisconnect->from->outputs->next;
            }
            else {
                previous->next=current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Funkcja usuwajaca bramke oraz jej wszystkie polaczenia
void nand_delete(nand_t* g) {
    if (g == NULL) return;

    // Usuwanie powiazan wyjsciowych
    node_t* current = g->outputs;
    while (current != NULL) {
        if (current->signal->to != NULL) {
            // Szukanie i usuwanie sygnalu w bramce docelowej.
            for (unsigned i = 0; i < current->signal->to->num_inputs; ++i) {
                if (current->signal->to->inputs[i] == current->signal) {
                    current->signal->to->inputs[i] = NULL;
                }
            }
        }
        // Zwalnianie sygnalu i wezla.
        node_t* temp = current->next;
        free(current->signal);
        free(current);
        current=temp;
    }

    // Usuwanie powiazan wejsciowych.
    for (unsigned i = 0; i < g->num_inputs; ++i) {
        if (g->inputs[i] != NULL) {
            disconnect_signal_from_outputs(g->inputs[i]);
            free (g->inputs[i]);
        }
    }

    // Zwalnianie pamieci bramki.
    free(g->inputs);
    free(g);
}

// Funkcja usuwajaca sygnal.
void delete_signal(signal_t* signalToDelete) {
    if (signalToDelete == NULL) return;

    // Odlaczenie sygnalu z listy wyjsc w bramce zrodlowej
    if (signalToDelete->from != NULL && signalToDelete->from->outputs != NULL){
        node_t* current = signalToDelete->from->outputs;
        node_t* prev = NULL;
        while (current != NULL) {
            if (current->signal == signalToDelete) {
                if (prev == NULL) {
                    signalToDelete->from->outputs = current->next;
                }
                else {
                    prev->next = current->next;
                }
                free (current);
                break;
            }
            prev = current;
            current = current->next;
        }
    }

    // Ustawienie odpowiedniego indeksu w tablicy wejsc na NULL.
    if (signalToDelete->to != NULL) {
        for (unsigned i=0; i<signalToDelete->to->num_inputs; ++i) {
            if (signalToDelete->to->inputs[i] == signalToDelete) {
                signalToDelete->to->inputs[i] = NULL;
            }
        }
    }

    // Zwolnienie pamieci sygnalu.
    free(signalToDelete);
}

// Funkcja podlaczajaca sygnal do bramki NAND.
int nand_connect_signal(bool const* s, nand_t* g, unsigned k) {
    errno = 0;
    if (s == NULL || g == NULL || k >= g->num_inputs) {
        errno = EINVAL;
        return -1;
    }

    // Alokacja pamieci dla nowego sygnalu.
    signal_t* newSignal = (signal_t*)malloc(sizeof(signal_t));
    if (newSignal == NULL) {
        errno = ENOMEM;
        return -1;
    }

    // Inicjalizacja pol struktury sygnalu.
    newSignal->from = NULL;
    newSignal->to = g;
    newSignal->pointerToSignal = (bool*)s;

    // Usuniecie starego sygnalu (jesli istnieje) i podlaczenie nowego.
    if (g->inputs[k] != NULL) {
        delete_signal(g->inputs[k]);
    }

    g->inputs[k] = newSignal;

    return 0;
}

// Funkcja podlaczajaca wyjscie jednej bramki NAND do wejscia drugiej.
int nand_connect_nand(nand_t* g_out, nand_t* g_in, unsigned k) {
    errno = 0;
    if (g_out == NULL || g_in == NULL || k >= g_in->num_inputs) {
        errno = EINVAL;
        return -1;
    }

    // Alokacja pamieci dla nowego sygnalu.
    signal_t* newSignal = (signal_t*)malloc(sizeof(signal_t));
    if (newSignal == NULL) {
        errno = ENOMEM;
        return -1;
    }

    // Alokacja pamieci dla nowego wezla listy.
    node_t* newNode = (node_t*)malloc(sizeof(node_t));
    if (newNode == NULL) {
        free(newSignal);
        errno = ENOMEM;
        return -1;
    }

    // Inicjalizacja nowego sygnalu.
    newSignal->from = g_out;
    newSignal->to = g_in;

    // Usuniecie starego sygnalu (jeśli istnieje) i podlaczenie nowego.
    if (g_in->inputs[k] != NULL) {
        delete_signal(g_in->inputs[k]);
    }

    g_in->inputs[k] = newSignal;

    // Dodanie sygnalu do listy wyjsc bramki wyjsciowej.
    newNode->signal = newSignal;
    if (g_out->outputs == NULL) {
        newNode->next = NULL;
        g_out->outputs = newNode;
    }
    else {
        newNode->next = g_out->outputs;
        g_out->outputs = newNode;
    }

    return 0;
}

// Funkcja ewaluujaca bramke NAND i ustalajaca wartosc oraz dlugosc
// sciezki krytycznej.
void get_path_evaluate(nand_t* g) {
    if (g->evaluated == true) {
        return;
    }

    // Detekacja cykli.
    if (g->isevaluated) {
        g->isevaluated = false;
        g->evaluated = true;
        g->gate_val = -1;
        g->path_len = 0;
        return;
    }

    g->isevaluated = true;

    // Bramka bez wejsc.
    if (g->num_inputs == 0) {
        g->evaluated = true;
        g->isevaluated = false;
        g->gate_val = 0;
        g->path_len = 0;
        return;
    }

    // Detekcja pustych wejsc.
    for (unsigned k = 0; k < g->num_inputs; ++k) {
        if (g->inputs[k] == NULL) {
            g->isevaluated = false;
            g->evaluated = true;
            g->gate_val = -1;
            g->path_len = 0;
            return;
        }
    }

    // Bramka z jednym wejsciem - negacja.
    if (g->num_inputs == 1) {
        if (g->inputs[0]->from != NULL) {
            get_path_evaluate(g->inputs[0]->from);
            if (g->inputs[0]->from->gate_val == -1) {
                g->gate_val = -1;
                g->path_len = 0;
            } else {
                g->gate_val = !(g->inputs[0]->from->gate_val);
                g->path_len = 1 + g->inputs[0]->from->path_len;
            }
        } else {
            g->gate_val = !(*(g->inputs[0]->pointerToSignal));
            g->path_len = 1;
        }
        g->evaluated = true;
        g->isevaluated = false;
        return;
    }

    // Ewalucaja wejsc.
    unsigned true_inputs = 0;       // Sygnaly o wartosci true.
    unsigned signals_connected = 0; // Sygnaly podlaczone bezposrednio.
    for (unsigned k = 0; k < g->num_inputs; ++k) {
        if (g->inputs[k]->from == NULL) {
            if (*(g->inputs[k]->pointerToSignal) == true) {
                true_inputs++;
            }
            signals_connected++;
            continue;
        }
        nand_t* from_gate = g->inputs[k]->from;
        get_path_evaluate(from_gate);
        if (from_gate->gate_val == 1) {
            true_inputs++;
        } else if (from_gate->gate_val == -1) {
            g->gate_val = -1;
            g->path_len = 0;
            g->evaluated = true;
            g->isevaluated = false;
            return;
        }
    }

    if (signals_connected == g->num_inputs) {
        g->path_len = 1;
    } else {
        unsigned max_path_len = 0;
        for (unsigned k = 0; k < g->num_inputs; ++k) {
            if (g->inputs[k]->from != NULL &&
                     g->inputs[k]->from->path_len > max_path_len) {
                max_path_len = g->inputs[k]->from->path_len;
            }
        }
        g->path_len = 1 + max_path_len;
    }

    g->gate_val = (true_inputs == g->num_inputs) ? 0 : 1;
    g->evaluated = true;
    g->isevaluated = false;
}

// Funkcja resetujaca bramke NAND do stanu niewyewaluowanego.
void reset_evaluate(nand_t* g) {
    if (g==NULL || !(g->evaluated)) return;

    g->path_len = 0;
    g->evaluated = false;
    g->gate_val = 2;
    g->isevaluated = false;

    for (unsigned i=0; i<g->num_inputs; ++i) {
        if (g->inputs[i] != NULL && g->inputs[i]->from != NULL)
         reset_evaluate(g->inputs[i]->from);
    }
}

// Funkcja ewaluujaca sygnaly NAND
ssize_t nand_evaluate(nand_t** g, bool* s, size_t m) {
    errno = 0;
    if (g == NULL || s == NULL || m == 0) {
        errno = EINVAL;
        return -1;
    }

    for (unsigned  i = 0; i < m; ++i) {
        if (g[i] == NULL) {
            errno = EINVAL;
            return -1;
        }
    }

    for (unsigned i = 0; i < m; ++i) {
        get_path_evaluate(g[i]);
        if (g[i]->gate_val == -1) {
            errno = ECANCELED;

            // Powrot do stanu poczatkowego.
            for (unsigned j = 0; j < m; ++j) {
                reset_evaluate(g[j]);
            }

            return -1;
        }

        s[i] = g[i]->gate_val;
    }

    // Wskazanie maksymalnej sciezki krytycznej.
    unsigned max = 0;
    for (unsigned i = 0; i < m; ++i) {
        if (g[i]->path_len > max) {
            max = g[i]->path_len;
        }
    }

    // Powrot do stanu poczatkowego.
    for (unsigned i = 0; i < m; ++i) {
        reset_evaluate(g[i]);
    }

    return max;
}

// Funkcja sprawdzajaca liczbe wejsc do ktorej podlaczona jest bramka.
ssize_t nand_fan_out(nand_t const* g) {
    errno = 0;
    if (g == NULL) {
        errno = EINVAL;
        return -1;
    }

    ssize_t count = 0;
    node_t* current = g->outputs;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

// Funkcja zwracajaca wskaznik do sygnaly lub bramki NAND
// podlaczonej do k-tego wejscia.
void* nand_input(nand_t const* g, unsigned k) {
    errno = 0;
    if (g == NULL || k >= g->num_inputs) {
        errno = EINVAL;
        return NULL;
    }

    // Sprawdzenie czy wejście k jest podłączone
    if (g->inputs[k] == NULL) {
        errno = 0;
        return NULL;
    }

    // Zwracanie wskaznika do boola lub bramki NAND
    if (g->inputs[k]->from == NULL) {
        return g->inputs[k]->pointerToSignal;
    }
    else {
        return g->inputs[k]->from;
    }
}

// Funkcja zwracjaca wskaznik do bramki NAND podlaczonej do k-tego wejscia.
nand_t* nand_output(nand_t const* g, ssize_t k) {
    if (g == NULL || k >= nand_fan_out(g) || k < 0) {
        return NULL;
    }

    node_t* current = g->outputs;
    int i = 0;
    while (current != NULL) {
        if (i == k) {
            return current->signal->to;
        }
        current = current->next;
        i++;
    }

    // Zwrocenie NULL, jesli nie znaleziono k-tego wyjscia
    return NULL;
}

int main (void) {
    return 0;
}
