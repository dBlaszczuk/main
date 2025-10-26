#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <poll.h>
#include <ctime>
#include <queue>

struct DelayedMessage {
    std::string txt;
    time_t added_at = time(nullptr);
    time_t delay_by;
    DelayedMessage(const std::string& txt, time_t delay)
    : txt(txt), delay_by(delay)  {}
};

struct ClientSession {
    int fd;                     // Deskryptor klienta
    std::string playerId;       // Nazwa gracza (identyfikator)
    std::vector<double> approx;     // Funkcja aproksymująca 
    std::vector<double> og_coeffs;
    std::string rcv_buffer;         // Bufor do przechowywania danych klienta
    std::queue<std::string> handle_buffer_q; // Bufor do przechowywania wiadomości do przetworzenia
    std::queue<std::string> send_buffer_q;    // Bufor do przechowywania wiadomości do wysłania

    std::vector<DelayedMessage> DelayedMessages; // Używany do wysyłania wiadomości, które mają być opóźnione o daną liczbę sekund
    time_t connected_at = time(nullptr);

    /* Liczniki */
    int score = 0;              // Wynik gracza
    int client_put_count = 0;   // Licznik PUTów wysłanych przez klienta

    /* Falgi sesji gracza */
    bool put_allowed = false;   // Czy gracz może wysłać PUT
    bool COEFF_or_STATE_is_being_sent = false; // Czy gracz rozpoczął wysyłanie COEFF lub STATE
    bool hello_recived = false;

    /* Flagi końca sesji */
    bool scoring_is_being_sent = false; // Czy serwer wysyła komunikat SCORING
    bool scoring_sent = false; // Czy gracz otrzymał komunikat SCORING
};

class Server {
public:
    Server(unsigned short port, int K, int N, int M, const std::string& coeffs_file);
    void run();

private:
    void sendScoringFromBuffer(ClientSession& client);
    std::string getClientAddressAndPort(int fd);
    void clearQ(ClientSession& client);
    void handleClientRunLogic(int fd);
    std::vector<double> parseCoeffsLine();
    double calculatePlayerScore(const ClientSession& client);
    std::string generateScoringMsg();
    void sendScoringToAllClients();
    void cleanupAndRestart();
    /* Metody pomocnicze */
    void acceptNewClient();
    void handleClientData(size_t index);
    void processClientLine(ClientSession& client, const std::string& line);
    void loadNextCoeff();
    void disconnectClient(int fd);
    void sendFromBuffer(ClientSession& client);
    bool hasMax7DecimalPlaces(double value);
    void badMessage(ClientSession& client, const std::string& message);
    void handleReceivedLine(ClientSession& client);
    void finishGame();

    /* Metody do przetwarzania komunikatów od klienta */
    void parseAndProcessClientMessage(ClientSession& client, const std::string& line);
    void handleHello(ClientSession& client, const std::string playerId);
    void handlePut(ClientSession& client, int point, double value);
    void handleDelayedMessages(ClientSession& client);

    /* Metody do dodawania komunikatów do bufora wyjściowego */
    void addCoeffsToBuffer(ClientSession& client);
    void addStateToBuffer(ClientSession& client);
    void addBadPutToBuffer(ClientSession& client, int point, int value);
    void addPenaltyToBuffer(ClientSession& client, int point, int value);
    void addStateDelayedSend(ClientSession& client, time_t delay_by);

    int listenFd_;              // Deskryptor gniazda nasłuchującego
    int K_, N_, M_;             // Parametry gry
    std::ifstream coeffsStream_;// Strumień do pliku z współczynnikami  
    std::string coeffsLine_;    // Komunikat z współczynnikami
    int putCount_ = 0;          // Licznik PUTów

    std::vector<struct pollfd> pollFds_;
    std::unordered_map<int, ClientSession> clients_;

    /* Do restartu */
    bool restart_ = false;
    time_t server_closed_at_;
};
