#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ma.h"

typedef struct connection {
    size_t in;             // Numer wejścia        
    size_t out;            // Numer wyjścia   
    moore_t *a_input;      // Wskaźnik do automatu wejściowego
    moore_t *a_output;     // Wskaźnik do automatu wyjściowego
} connection_t;

typedef struct list {
    struct list *next;        
    connection_t *conn;               
} list_t;

/* Definicja struktury wewnętrznej automatu Moore’a. */
typedef struct moore {
    size_t n;                           // Liczba wejść
    size_t m;                           // Liczba wyjść
    size_t s;                           // Liczba bitów stanu
    transition_function_t t;            // Funkcja przejść
    output_function_t y;                // Funkcja wyjść
    uint64_t *state;                    // Aktualny stan automatu
    uint64_t *input;                    // Tablica z sygnałem wejściowym 
    uint64_t *output;                   // Tablica z sygnałem wyjściowym 
    connection_t **input_signal_table;  // Tablica połączeń wejściowych
    list_t *output_signal_list;         // Lista połączeń wyjściowych
} moore_t;

/* Funkcje pomocnicze */

// Funkcja ta jest tożsamosciowa.
static void ma_simple_output_function(uint64_t *output,
                         uint64_t const *state, size_t m, size_t s) {
    (void)m; // Oznacz zmienną jako nieużywaną
    size_t elems = (s + 63) / 64;
    for (size_t i = 0; i < elems; i++)
        output[i] = state[i];
}

// Wyciąga bit o indeksie 'i' z upakowanego ciągu bitów 'arr'.
static inline bool get_bit(const uint64_t *arr, size_t i) {
    return (arr[i / 64] >> (i % 64)) & 1ULL;
}

// Ustawia bit o indeksie 'i' w ciągu 'arr' 
// na wartość 'value' (true -> 1, false -> 0).
static inline void set_bit(uint64_t *arr, size_t i, bool value) {
    size_t word = i / 64;
    size_t bit  = i % 64;
    if (value)
        arr[word] |= (1ULL << bit);
    else
        arr[word] &= ~(1ULL << bit);
}

// Funkcja do usuwania połączenia (connection_t) z listy.
static void remove_from_list(list_t **list_ptr, connection_t *conn) {
    if (!list_ptr || !conn) return;

    list_t *prev = NULL;
    list_t *current = *list_ptr;

    while (current != NULL) {
        if (current->conn == conn) {
            if (prev) {
                prev->next = current->next;
            } else {
                *list_ptr = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Funkcja dodająca połączenie (connection_t) do listy.
// Zwraca 0 w przypadku sukcesu, -1 w przypadku błędu
static int add_to_list(list_t **list_ptr, connection_t *conn) {
    if (!list_ptr || !conn) return -1;

    list_t *new_node = malloc(sizeof(list_t));
    if (!new_node) {
        return -1; // Błąd alokacji
    }
    new_node->conn = conn;
    new_node->next = NULL;

    if (*list_ptr == NULL) {
        *list_ptr = new_node;
    } else {
        list_t *current = *list_ptr;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    return 0;
}

// Funkcja do zwalniania połączenia (connection_t).
// Usuwa połączenie zarówno z listy wyjściowej, jak i z tablicy wejściowej.
static inline void free_connection(connection_t *conn) {
    if (!conn) return;

    if (conn->a_output && conn->a_output->output_signal_list) {
        remove_from_list(&(conn->a_output->output_signal_list), conn);
    }

    if (conn->a_input && conn->in < conn->a_input->n) {
        conn->a_input->input_signal_table[conn->in] = NULL;
    }

    free(conn);
}

// Funkcja do usuwania całej listy połączeń (connection_t).
static void remove_list(list_t **list_ptr) {
    if (!list_ptr) return;

    list_t *current = *list_ptr;
    while (current != NULL) {
        list_t *next = current->next;
        free_connection(current->conn);
        current = next;
    }
    *list_ptr = NULL;
}

/* Implementacja funkcji z ma.h */

// Funkcja do tworzenia automatu Moore’a.
moore_t *ma_create_full(size_t n, size_t m, size_t s, 
            transition_function_t t, output_function_t y, uint64_t const *q) {
    if (m == 0 || s == 0 || !t || !y || !q) {
        errno = EINVAL;
        return NULL;
    }
    
    moore_t *a = malloc(sizeof(*a));
    if (!a) {
        errno = ENOMEM;
        return NULL;
    }
    // Inicjalizacja pól struktury moore_t
    a->n = n;
    a->m = m;
    a->s = s;
    a->t = t;
    a->y = y;

    size_t state_elems = (s + 63) / 64;
    a->state = calloc(state_elems, sizeof(uint64_t));

    size_t input_elems = (n + 63) / 64;
    a->input = calloc(input_elems, sizeof(uint64_t));

    size_t output_elems = (m + 63) / 64;
    a->output = calloc(output_elems, sizeof(uint64_t));

    a->input_signal_table = calloc(n, sizeof(connection_t *));
    
    a->output_signal_list = NULL;

    if (!a->state  || !a->input || !a->output
                             || !a->input_signal_table ) {
        free(a->state);
        free(a->input);
        free(a->output);
        free(a->input_signal_table);
        free(a);
        errno = ENOMEM;
        return NULL;
    }

    // Ustawienie początkowego stanu automatu
    for (size_t i = 0; i < state_elems; i++) a->state[i] = q[i];

    // Ustawienie początkowego sygnału wyjściowego
    a->y(a->output, a->state, m, s);
    
    return a;
}

// Funkcja do tworzenia prostego automatu Moore’a.
moore_t *ma_create_simple(size_t n, size_t s, transition_function_t t) {
    if (s == 0 || !t) {
        errno = EINVAL;
        return NULL;
    }
    
    size_t m = s;
    size_t state_elems = (s + 63) / 64;
    uint64_t *zero_state = calloc(state_elems, sizeof(uint64_t));
    if (!zero_state) {
        errno = ENOMEM;
        return NULL;
    }

    moore_t *a = ma_create_full(n, m, s, t,
                             ma_simple_output_function, zero_state);

    free(zero_state);
    return a;
}

// Funkcja zwalniająca pamięć zajmowaną przez automat Moore’a.
// Zwalnia wszystkie jego zasoby.
void ma_delete(moore_t *a) {
    if (!a) return;

    remove_list(&(a->output_signal_list));

    for (size_t i = 0; i < a->n; i++) {
        if (a->input_signal_table[i]) {
            free_connection(a->input_signal_table[i]);
        }
    }
    free(a->state);
    free(a->input);
    free(a->output);
    free(a->input_signal_table);
    free(a);
}

// Funkcja do łączenia automatu wejściowego z automatem wyjściowym.
// Łączy 'num' sygnałów wejściowych z automatu wejściowego
// z sygnałami wyjściowymi automatu wyjściowego,
// zaczynając od indeksu 'in' w automacie wejściowym
// i indeksu 'out' w automacie wyjściowym.
int ma_connect(moore_t *a_in, size_t in, moore_t *a_out, size_t out, size_t num) {
    if (!a_in || !a_out || num == 0 || in + num > a_in->n || out + num > a_out->m) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < num; i++) {
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn) {
            errno = ENOMEM;
            return -1;
        }
        conn->in = in + i;
        conn->out = out + i;
        conn->a_input = a_in;
        conn->a_output = a_out;

        // Zwalniam istniejące połączenie, jeżeli istnieje.
        if (a_in->input_signal_table[in + i]) {
            connection_t *old_conn = a_in->input_signal_table[in + i];
            free_connection(old_conn);
        }

        // Sprawdzam, czy alokacja pamięci się powiodła
        if (add_to_list(&(a_out->output_signal_list), conn) == -1) {
            free(conn); 
            a_in->input_signal_table[in + i] = NULL; 
            errno = ENOMEM;
            return -1;
        }

        a_in->input_signal_table[in + i] = conn;
    }
    return 0;
}

// Funkcja odłączająca sygnały wejściowe od automatu wejściowego.
// Odłącza 'num' sygnałów wejściowych zaczynając od indeksu 'in'.
int ma_disconnect(moore_t *a_in, size_t in, size_t num) {
    if (!a_in || num == 0 || in + num > a_in->n) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < num; i++) {
        connection_t *old_conn = a_in->input_signal_table[in + i];
        if (old_conn) free_connection(old_conn);
    } 

    return 0;
}

// Funkcja do ustawiania sygnału wejściowego automatu.
// Pomija wejścia do których podłączone są inne automaty.
int ma_set_input(moore_t *a, uint64_t const *input) {
    if (!a || !input) {
        errno = EINVAL;
        return -1;
    }
    
    size_t num_imputs = a->n;

    for (size_t i = 0; i < num_imputs; i++) {
        if (!a->input_signal_table[i]) {
            bool bit_val = get_bit(input, i);
            set_bit(a->input, i, bit_val);
        }
    }
    return 0;
}

// Funkcja do ustawiania stanu automatu.
int ma_set_state(moore_t *a, uint64_t const *state) {
    if (!a || !state) {
        errno = EINVAL;
        return -1;
    }
    
    size_t elems = (a->s + 63) / 64;
    for (size_t i = 0; i < elems; i++)
        a->state[i] = state[i];
    
    // Ustala nowe wyjście na podstawie nowego stanu
    a->y(a->output, a->state, a->m, a->s);

    return 0;
}


// Funkcja zwraca wskaźnik, pod którym znajduje się ciąg bitów
// reprezentujący wyjście automatu.
uint64_t const *ma_get_output(moore_t const *a) {
    if (!a) {
        errno = EINVAL;
        return NULL;
    }

    return a->output;
}

// Funkcja wykonuje krok automatu.
int ma_step(moore_t *at[], size_t num) {
    if (!at || num == 0) {
        errno = EINVAL;
        return -1;
    }
    
    // Sprawdź poprawność automatów.
    for (size_t i = 0; i < num; i++) {
        if (!at[i]) {
            errno = EINVAL;
            return -1;
        }
    }   

    // Ustawiamy sygnały wejściowe dla każdego automatu (jeżeli to konieczne).
    for (size_t a_idx = 0; a_idx < num; a_idx++) {
        moore_t *a = at[a_idx];
        for (size_t i = 0; i < a->n; i++) {
            // Pobieranie sygnału wejściowego z automatu wyjściowego.
            if (a->input_signal_table[i]) {
                connection_t *conn = a->input_signal_table[i];
                if (!conn->a_output || conn->out >= conn->a_output->m) {
                    free_connection(conn);
                    a->input_signal_table[i] = NULL; 
                    continue;
                }
                bool bit_val = get_bit(conn->a_output->output, conn->out);
                set_bit(a->input, i, bit_val);
            }
        }
    }

    // Dla każdego automatu alokujemy tymczasowy bufor next_state.
    // Obliczeamy next_state na podstawie a->input i a->state.
    for (size_t a_idx = 0; a_idx < num; a_idx++) {
        moore_t *a = at[a_idx];
        size_t state_elems = (a->s + 63) / 64;
        uint64_t *next_state = calloc(state_elems, sizeof(uint64_t));
        if (!next_state) {
            errno = ENOMEM;
            return -1;
        }

        // Użycie funkcji przejścia do obliczenia next_state
        a->t(next_state, a->input, a->state, a->n, a->s);
        
        for (size_t j = 0; j < state_elems; j++) 
            a->state[j] = next_state[j];
        
        free(next_state);
    }
    
    // Po uaktualnieniu stanu, przelicz wyjścia dla każdego automatu
    for (size_t a_idx = 0; a_idx < num; a_idx++) {
        moore_t *a = at[a_idx];
        a->y(a->output, a->state, a->m, a->s);
    }
    
    return 0;
}
