section .data

statbuf         times 144 db 0                 ; Bufor dla struktury stat
errmsg          db "Error", 10                 ; Komunikat błędu + newline
errmsg_len      equ $ - errmsg                 ; Długość komunikatu błędu


section .text

global _start

_start:

                ;----------------------------------------
                ; Sprawdzenie liczby argumentów programu
                ;----------------------------------------

                mov     rax, [rsp]              ; argc (liczba argumentów)
                cmp     rax, 2                  ; Sprawdź czy argc == 2
                jne     exit_error              ; Jeśli nie — zakończ z błędem

                ;----------------------------------------
                ; Pobranie wskaźnika do argv[1] (nazwy pliku)
                ;----------------------------------------

                mov     rsi, [rsp + 16]         ; argv[1] → rsi

                ;----------------------------------------
                ; Otwórz plik w trybie O_RDWR
                ;----------------------------------------

                mov     rax, 2                  ; sys_open
                mov     rdi, rsi                ; filename
                mov     rsi, 2                  ; flags: O_RDWR
                xor     rdx, rdx                ; mode: 0 
                syscall
                cmp     rax, 0                  ; Sprawdź czy fd >= 0
                jl      exit_error              ; Jeśli fd < 0, zakończ z błędem
                mov     r12, rax                ; fd → r12

                ;----------------------------------------
                ; Pobierz informacje o pliku (sys_fstat)
                ;----------------------------------------

                mov     rax, 5                  ; sys_fstat
                mov     rdi, r12                ; fd
                lea     rsi, [rel statbuf]      ; wskaźnik do statbuf
                syscall
                cmp     rax, 0                  ; Sprawdź czy syscall powiódł się
                jne     close_and_exit_error    ; W razie błędu

                ;----------------------------------------
                ; Odczytaj rozmiar pliku z statbuf.st_size
                ;----------------------------------------

                mov     rax, [rel statbuf + 48] ; rax = st_size
                cmp     rax, 2                  ; Jeśli rozmiar < 2 bajty
                jb      close_and_exit_success  ; nic nie odwracaj, zakończ
                mov     r13, rax                ; Rozmiar pliku → r13

                ;----------------------------------------
                ; Mapuj plik do pamięci (sys_mmap)
                ;----------------------------------------

                mov     rax, 9                  ; sys_mmap
                xor     rdi, rdi                ; addr = NULL (automatyczny)
                mov     rsi, r13                ; length = rozmiar pliku
                mov     rdx, 3                  ; prot = PROT_READ | PROT_WRITE
                mov     r10, 1                  ; flags = MAP_SHARED
                mov     r8,  r12                ; fd
                xor     r9,  r9                 ; offset = 0
                syscall
                cmp     rax, -4095              ; Sprawdź czy mmap powiódł się
                jae     close_and_exit_error    ; Jeśli nie zamknij plik 
                mov     r14, rax                ; Adres mapowania → r14

                ;----------------------------------------
                ; Odwracanie zawartości pliku w pamięci
                ;----------------------------------------

                xor     r15, r15                ; i = 0

reverse_loop:
                mov     rax, r13                ; rax = size
                dec     rax                     ; rax = size - 1
                sub     rax, r15                ; rax = size - 1 - i
                cmp     r15, rax                ; czy i >= (size-1-i)
                jge     sync_and_unmap          ; jeśli tak — zakończ pętlę

                ; Zamiana bajtów: temp = mem[i]
                mov     rbx, r14                ; rbx = base_addr
                add     rbx, r15                ; rbx = base_addr + i
                mov     cl, [rbx]               ; cl = mem[i]

                ; mem[i] = mem[size-1-i]
                mov     rdx, r14                ; rdx = base_addr
                add     rdx, rax                ; rdx = base_addr + (size-1-i)
                mov     al, [rdx]               ; al = mem[size-1-i]
                mov     [rbx], al               ; mem[i] = al

                ; mem[size-1-i] = temp
                mov     [rdx], cl               ; mem[size-1-i] = cl

                inc     r15                     ; i++
                jmp     reverse_loop            ; powrót do pętli

                ;----------------------------------------
                ; Synchronizacja pamięci z plikiem (sys_msync)
                ;----------------------------------------

sync_and_unmap:
                mov     rax, 26                 ; sys_msync
                mov     rdi, r14                ; addr
                mov     rsi, r13                ; length
                mov     rdx, 0                  ; flags = MS_SYNC
                syscall
                cmp     rax, 0                  ; sprawdzenie błędu
                jne     unmap_and_exit_error    ; w razie błędu: 
                                                ; unmap i wyjście z błędem

                ; Odmapowanie pamięci (sys_munmap)

                mov     rax, 11                 ; sys_munmap
                mov     rdi, r14                ; addr
                mov     rsi, r13                ; length
                syscall
                cmp     rax, 0
                jne     close_and_exit_error    ; w razie błędu 
                                                ; — zamknij i wyjdź

                ; Zamknięcie pliku i zakończenie z sukcesem

                jmp     close_and_exit_success

                ;----------------------------------------
                ; Ścieżki wyjścia z błędem lub sukcesem
                ;----------------------------------------

close_and_exit_error:
                mov     rax, 3                  ; sys_close
                mov     rdi, r12                ; fd
                syscall

exit_error:
                mov     rax, 60                 ; sys_exit
                mov     rdi, 1                  ; kod wyjścia: 1 (błąd)
                syscall

unmap_and_exit_error:
                mov     rax, 11                 ; sys_munmap
                mov     rdi, r14                ; addr
                mov     rsi, r13                ; length
                syscall
                jmp     close_and_exit_error

close_and_exit_success:
                mov     rax, 3                  ; sys_close
                mov     rdi, r12                ; fd
                syscall
                mov     rax, 60                 ; sys_exit
                xor     rdi, rdi                ; kod wyjścia: 0 (sukces)
                syscall
