global mdiv

; mdiv - funkcja dzielenia
; rdi - wskaznik na dzielna (x)
; rsi - ilosc elementow (n)
; rdx - dzielnik (y)

mdiv:
    ; Normalizacja dzielnika
    xor r10, r10                   ; Rejestr do przechowywania informacji o tym czy dzielnik jest znormalizowany
    cmp rdx, 0                     ; Sprawdz, czy dzielnik jest dodatni
    jge .dzielnik_dodatni          ; Jesli dzielnik jest nieujemny, przejdz do .dzielnik_dodatni

    ; Negacja dzielnika
    neg rdx                        ; W przeciwnym razie zaneguj dzielnik
    inc r10                        ; Ustawienie r10 na 1, gdy dzielnik jest zanegowany (czyli ujemny)

    ; Normalizacja dzielnej
    .dzielnik_dodatni:
    xor r11, r11                   ; Rejestr do przechowywania informacji o tym czy dzielna jest znormalizowany
    cmp qword [rdi+rsi*8-8], 0     ; Porównanie znaku dzielnej z zerem
    jge .dzielna_dodatnia          ; Jesli dzielna jest nieujemna, przejdŸ do .dzielna_dodatnia

    ; Negacja dzielnej
    mov rcx, rsi                   ; Wczytaj ilosc elementów do rcx
    xor r8, r8                     ; Normalizacja bêdzie siê odbywaæ od cyfr najmiej znacz¹cych; ustawienie licznika
    stc                            ; Ustaw flagê CF na 1, aby zainicjowac operacjê negacji
    .rozpoczecie_negacji:
    not qword [rdi + r8*8]         ; Zaneguj wartosc w komórce dzielnej
    adc qword [rdi + r8*8], 0      ; Dodaj przeniesienie z poprzedniej operacji do aktualnej
    inc r8                         ; Zwieksz licznik
    loop .rozpoczecie_negacji      ; Powtorz, dopoki nie zostan¹ zanegowane wszystkie elementy
    inc r11                        ; Ustawienie r11 na 1, gdy dzielna jest zanegowana (czyli ujemna)

    .dzielna_dodatnia:
    mov r9, rdx                    ; Wczytaj dzielnik do r9 aby uniknac nadpisania przez rdx
    xor rdx, rdx                   ; Wyzeruj reszte
    mov rcx, rsi                   ; Wczytaj ilosc elementow do rcx

    .dzielenie:
    mov rax, qword [rdi+rcx*8-8]   ; Wczytaj wartosc z dzielnej
    div r9                         ; Podziel wartosc przez dzielnik
    mov qword [rdi+rcx*8-8], rax   ; Zapisz wynik dzielenia w dzielnej
    loop .dzielenie                ; Powtorz dla kazdego elementu
    mov rax, rdx                   ; Wczytaj resztê do rax

    ; Ustalenie znakow na podstawie normalizacji
    cmp r11, r10                   ; Sprawdz, czy dzielna miala taki sam znak co dzielnik
    jne .neguj_wartosc             ; Jesli tak, przejdz do .neguj_wartosc

    ; Obs³uga przepe³nienia
    cmp qword [rdi+rsi*8-8], 0     ; Sprawdz, czy wynik dzielenia jest dodatni
    jge .wykonane                  ; Jesli tak, przejdz do wykonane
    div rcx                        ; Jesli nie, wywolaj wyj¹tek dzielenia przez zero

    .neguj_wartosc:
    mov rcx, rsi                   ; Wczytaj ilosc elementów do rcx
    xor r8, r8                     ; Wyzeruj licznik
    stc                            ; Ustaw flage CF na 1, aby zainicjowac operacje negacji
    .negacja_wartosci:
    not qword [rdi + r8*8]         ; Zaneguj wartosc w komorce dzielnej
    adc qword [rdi + r8*8], 0      ; Dodaj przeniesienie z poprzedniej operacji do aktualnej
    inc r8                         ; Zwieksz licznik
    loop .negacja_wartosci         ; Powtorz, dopoki nie zostana zanegowane wszystkie elementy

    .wykonane:                     ; Koniec negacji dzielnej
    test r11, r11                  ; Testuj r11 aby ustawic Zero Flag na 1 dla r11=0 albo na 0 dla r11=1
    jne .neguj_reszte              ; Skacze do .neguj_reszte jeœli flaga ZF (Zero Flag) jest wyzerowana,
    ret                            ; co oznacza, ¿e r11 nie jest zerowe (ró¿ne od 0).
    .neguj_reszte:
    neg rax                        ; Zaneguj resztê
    ret
