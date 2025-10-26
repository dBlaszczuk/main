#pragma once

#include <string>
#include <queue>
#include <vector>
#include <utility> // for std::pair

class Client {
public:
    Client(const std::string& host, unsigned short port, 
           const std::string& playerId, bool autoMode, int ipVersion);
    void run();

private:
    int sock_;                          // Socket file descriptor
    std::string host_;                  // Hostname or IP address    
    unsigned short port_;               // Port number
    std::string playerId_;              // Player ID
    bool autoMode_;                     // Flag for automatic mode
    int ipVersion_;                     // IP version (AF_INET or AF_INET6)

    int currentPoint_ = 0;
    int K_ = 100; // Domyślne K, aktualizowane po pierwszym STATE

    std::vector<double> coeffs_;        // Współczynniki wielomianu
    std::vector<double> approx_state_;  // Aktualny stan aproksymacji
    std::queue<std::string> sendQueue_; // Kolejka wiadomości do wysłania
    std::queue<std::string> recQueue_;  // Kolejka odebranych wiadomości
    std::string rec_buffer_;            // Bufor odebranych danych
    bool has_coeff_ = false;            // Czy otrzymano COEFF?
    std::queue<std::pair<int, double>> pending_puts_; // Oczekujące PUTy w trybie interaktywnym

    // Metody obsługi komunikatów
    void handleCoeff(const std::string& line);
    void handleBadPut(const std::string& line);
    void handleState(const std::string& line);
    void handleScoring(const std::string& line);
    void handlePenalty(const std::string& line);

    // Metody komunikacji
    void resolveHost();
    void connectToServer();
    void readMessages();
    void handleReceivedMessages();
    void queueHello();
    void queuePut(int point, double value);
    int sendBufferedMessages();
    void handleMessage(const std::string& line);
    void sendPendingPuts();
    
    // Strategia
    std::pair<int, double> autoStrategy();
};