#include "Server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <poll.h>

/*──────────────────────────────────────────────────────────────────────────────────
│ Konstruktor i inicjalizacja serwera
└──────────────────────────────────────────────────────────────────────────────────*/
Server::Server(unsigned short port, int K, int N, int M, const std::string& coeffs_file)
    : K_(K), N_(N), M_(M), coeffsStream_(coeffs_file)
{
    // 1) Utworzenie gniazda IPv6, ustawienie opcji SO_REUSEADDR i wyłączenie IPV6_V6ONLY
    listenFd_ = socket(AF_INET6, SOCK_STREAM, 0);
    if (listenFd_ < 0) perror("socket");

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    opt = 0;
    setsockopt(listenFd_, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    // 2) Powiązanie z adresem in6addr_any i podanym portem, uruchomienie listen()
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port   = htons(port);
    addr.sin6_addr   = in6addr_any;

    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0)  perror("bind");
    if (listen(listenFd_, SOMAXCONN) < 0)                      perror("listen");

    // 3) Ustawienie non-blocking i rejestracja w pollFds_
    fcntl(listenFd_, F_SETFL, fcntl(listenFd_, F_GETFL, 0) | O_NONBLOCK);
    pollFds_.push_back({ listenFd_, POLLIN, 0 });
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Metody główne: pętla serwera, kończenie gry i restart
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::run()
{
    std::cout << "Server started, waiting for clients..." << std::endl;
    for (;;)
    {
        //std::cout << "----- Main loop iteration -----" << std::endl;
        if (restart_) {
            restart_ = false;
            sleep(1);
            std::cout << "----- Starting new game -----" << std::endl;

        }
        int n = poll(pollFds_.data(), pollFds_.size(), 0);
        if (n < 0) perror("poll");
        //std::cout << "Poll returned: " << n << " events" << std::endl;
        // Przejdź od końca, by bezpiecznie usuwać gniazda
        for (int idx = (int)pollFds_.size() - 1; idx >= 0; --idx)
        {
            //std::cout << "Processing pollFds_[" << idx << "] with fd: " 
            //          << pollFds_[idx].fd << std::endl;
            // 1) Sprawdzenie warunku zakończenia po M_ valid PUT-ów
            if (putCount_ >= M_)
            {
                std::cout << "Reached M_ valid PUTs, finishing game..." << std::endl;
                finishGame();
                break; // Serwer się restartuje przy następnej iterazji pętli głównej
            }

            struct pollfd& pfd = pollFds_[idx];

            // 2) 
            if ((pfd.revents & POLLIN) && !restart_)
            {
                std::cout << "Processing events for fd: " << pfd.fd << std::endl;
                if (pfd.fd == listenFd_)
                {
                    acceptNewClient();
                    continue;
                }
                std::cout << "Handling client received data for fd: " << pfd.fd << std::endl;
                // 3) Dane od istniejącego klienta
                handleClientData(idx);
                /*
                auto it = clients_.find(pfd.fd);
                if (it != clients_.end())
                {
                    sendFromBuffer(it->second);
                }*/
            }
            /*
            // 3) Obsługa POLLOUT: wysyłanie wiadomości z send_buffer_q
            if ((pfd.revents & POLLOUT) && !restart_)
            {
                //std::cout << "Handling client send buffer for fd: " << pfd.fd << std::endl;
                auto it = clients_.find(pfd.fd);
                if (it != clients_.end())
                {
                    sendFromBuffer(it->second);
                }
            }*/

            // 4) Logic: odczyt gotowych linii i opóźnione wiadomości
            if (pfd.fd != listenFd_)
            {
                //std::cout << "Handling client logic for fd: " << pfd.fd << std::endl;
                handleClientRunLogic(pfd.fd);
            }
        }
    }
}

void Server::clearQ(ClientSession& client) {
    while (!client.send_buffer_q.empty()) {
        client.send_buffer_q.pop();
    }
}

void Server::sendScoringToAllClients()
{
    std::cout << "Sending SCORING to all clients..." << std::endl;
    std::string scoring_msg = generateScoringMsg();

    for (auto& entry : clients_)
    {
        ClientSession& client = entry.second;
        clearQ(client);
        client.send_buffer_q.push(scoring_msg);
    }
    std::cout << "SCORING message generated: " << scoring_msg 
                << "and queued to send to all clients" << std::endl;
    // 3) Pętla do momentu, aż wszyscy klienci będą mieli scoring_sent == true
    // Wysyłam do każdego ziomka
    for (;;) {
        bool all_done = true;
        for (auto& kv : clients_) {
            ClientSession& client = kv.second;
            if (!client.scoring_sent) {
                all_done = false;
                sendScoringFromBuffer(client);
            }
        }
        if (all_done) {
            std::cout << "All clients have received SCORING." << std::endl;
            break;
        }
    }
}

void Server::handleClientRunLogic(int fd) 
{
    ClientSession& client = clients_[fd];

    // toDO DODAŁEM LINIE CIEKAWE CZY DZIAŁA
    sendFromBuffer(client);

    handleReceivedLine(client);
    handleDelayedMessages(client);

    // 5) Timeout na HELLO (3 s)
    if (!client.hello_recived &&
        difftime(time(nullptr), client.connected_at) > 3)
    {
        disconnectClient(fd);
    }
}

void Server::cleanupAndRestart()
{
    // 1) Zamknij połączenia klientów i wyczyść ich stan
    for (auto& client : clients_) {
        close(client.first);
    }
    clients_.clear();

    // 2) Przywróć początkową listę pollfd (tylko gniazdo nasłuchujące)
    for (auto it = pollFds_.begin(); it != pollFds_.end(); ) {
        if (it->fd != listenFd_) {
            it = pollFds_.erase(it);
        } else {
            ++it;
        }
    }

    // 3) Resetuj licznik PUT-ów
    putCount_ = 0;
    /*
    // 4) Przewiń plik ze współczynnikami do początku (NIE ZAMYKAJ!)
    coeffsStream_.clear(); // Wyczyść flagi błędów
    coeffsStream_.seekg(0); // Przewiń do początku pliku
    */
    // 5) Ustaw flagę restartu
    restart_ = true;
    server_closed_at_ = time(nullptr);
    
    std::cout << "Server reset. Starting new game..." << std::endl;
}

/*
void Server::cleanupAndRestart()
{
    // 1) Wyczyść wszystkie dane klientów i pollFds_
    clients_.clear();
    
    pollFds_.clear();
    coeffsStream_.close();
    
    if (!pollFds_.empty())
    {
        // Zachowujemy tylko pollfd dla listenFd_
        auto listenPfd = pollFds_.front();
        pollFds_.clear();
        pollFds_.push_back(listenPfd);
    }

    // 2) Wyślij komunikat restartu (opcjonalnie log)
    std::cout << "Server closed resources. Restarting in 1s..." << std::endl;
    restart_ = true;
    server_closed_at_ = time(nullptr);
    // —logika restartu (np. nowa instancja) zostanie wykonana na zewnątrz
}*/

void Server::finishGame()
{
    // 1) Wyślij SCORING do wszystkich
    sendScoringToAllClients();

    // 2) Wyczyść zasoby i przygotuj restart
    cleanupAndRestart();
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Obliczanie wyniku gracza
└──────────────────────────────────────────────────────────────────────────────────*/
double Server::calculatePlayerScore(const ClientSession& client)
{
    // 1) Suma kwadratów różnic
    double sumSq = 0.0;
    for (int x = 0; x <= K_; ++x)
    {
        // Oblicz f(x) = ∑_{i=0..n} a_i * x^i
        double fx = 0.0;
        double xi = 1.0;
        for (size_t i = 0; i < client.og_coeffs.size(); ++i)
        {
            fx += client.og_coeffs[i] * xi;
            xi *= static_cast<double>(x);
        }

        // Pobierz aproksymowaną wartość
        double fhat = client.approx[x];
        double diff = fhat - fx;
        sumSq += diff * diff;
    }

    // 2) Dodaj sumę kar (client.score)
    return sumSq + static_cast<double>(client.score);
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Generowanie komunikatu SCORING
└──────────────────────────────────────────────────────────────────────────────────*/
std::string Server::generateScoringMsg()
{
    std::ostringstream msg;
    msg << "SCORING";

    // 1) Utwórz wektor par <player_id, wynik>
    std::vector<std::pair<std::string, double>> scores;
    for (const auto& entry : clients_)
    {
        const ClientSession& client   = entry.second;
        double               playerSc = calculatePlayerScore(client);
        scores.emplace_back(client.playerId, playerSc);
    }

    // 2) Posortuj leksykograficznie po player_id
    std::sort(scores.begin(), scores.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });

    // 3) Dołącz do komunikatu po kolei " playerId wynik"
    for (const auto& pr : scores)
    {
        msg << " " << pr.first << " "
            << std::fixed << std::setprecision(7) << pr.second;
    }

    msg << "\r\n";
    return msg.str();
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Obsługa opóźnionych wiadomości (STATE po liczbie sekund delay_by)
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::handleDelayedMessages(ClientSession& client)
{
    //std::cout << "Handling delayed messages for client " << client.fd << std::endl;
    auto it = client.DelayedMessages.begin();
    while (it != client.DelayedMessages.end())
    {
        if (difftime(time(nullptr), it->added_at) >= it->delay_by)
        {
            client.send_buffer_q.push(it->txt);
            it = client.DelayedMessages.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Sprawdzenie, czy liczba ma maksymalnie 7 miejsc po przecinku
└──────────────────────────────────────────────────────────────────────────────────*/
bool Server::hasMax7DecimalPlaces(double value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(16) << value;
    std::string str = oss.str();

    size_t dotPos = str.find('.');
    if (dotPos == std::string::npos)
    {
        return true; // brak części ułamkowej
    }

    // Oblicz liczbę cyfr po kropce, pomijając końcowe zera
    size_t decimals = str.size() - dotPos - 1;
    while (decimals > 0 && str.back() == '0')
    {
        str.pop_back();
        decimals--;
    }

    return decimals <= 7;
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Jeżeli komunikat jest błędny, wyświetl ERROR
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::badMessage(ClientSession& client, const std::string& line)
{
    std::string clientAddress = getClientAddressAndPort(client.fd);
    std::cerr << "ERROR: bad message from: [" << clientAddress << "], "
              << client.playerId << ": " << line << std::endl;
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Akceptowanie nowego klienta
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::acceptNewClient()
{
    sockaddr_storage clientAddr;
    socklen_t         clientAddrLen = sizeof(clientAddr);

    int clientFd = accept(listenFd_, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientFd < 0)
    {
        perror("accept");
        return;
    }

    // 1) Rozpoznaj IPv4 vs IPv6, wypisz na stdout
    if (clientAddr.ss_family == AF_INET)
    {
        std::cout << "Client connected using IPv4." << std::endl;
        sockaddr_in* addr4 = (sockaddr_in*)&clientAddr;
        char          ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr4->sin_addr), ip, sizeof(ip));
        std::cout << "  IPv4 Address: " << ip << std::endl;
    }
    else if (clientAddr.ss_family == AF_INET6)
    {
        std::cout << "Client connected using IPv6." << std::endl;
        sockaddr_in6* addr6 = (sockaddr_in6*)&clientAddr;
        char           ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(addr6->sin6_addr), ip, sizeof(ip));
        std::cout << "  IPv6 Address: " << ip << std::endl;
    }
    else
    {
        std::cerr << "Unknown address family." << std::endl;
        close(clientFd);
        return;
    }

    // 2) Non-blocking, rejestracja w pollFds_ i init sesji
    fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL, 0) | O_NONBLOCK);
    pollFds_.push_back({ clientFd, POLLIN, 0 });

    ClientSession sess;
    sess.fd     = clientFd;
    sess.approx.resize(K_ + 1, 0.0);
    clients_[clientFd] = sess;

    // 3) Opcjonalny log: "New client connected <IP:port>"
    char host[NI_MAXHOST], service[NI_MAXSERV];
    if (getnameinfo((sockaddr*)&clientAddr, clientAddrLen,
                    host, sizeof(host), service, sizeof(service),
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    {
        std::cout << "New client connected: " << host << ":" << service
                  << " (FD: " << clientFd << ")" << std::endl;
    }
    else
    {
        std::cout << "New client connected (FD: " << clientFd << ")" << std::endl;
    }
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Parsowanie i przetwarzanie pojedynczych linii otrzymanych od klienta
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::parseAndProcessClientMessage(ClientSession& client,
                                          const std::string& line)
{
    std::string type = line.substr(0, line.find(' '));

    if (type == "HELLO")
    {
        std::cout << "Received HELLO from client: " << client.fd << std::endl;
        // 1) Sprawdź składnię „HELLO <playerId>”
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos || spacePos + 1 >= line.size())
        {
            badMessage(client, line);
            return;
        }

        std::string playerId = line.substr(spacePos + 1);
        if (playerId.find(' ') != std::string::npos)
        {
            badMessage(client, line);
            return;
        }

        // 2) Zarejestruj playerId i wyślij COEFF
        handleHello(client, playerId);
        return;
    }
    else if (type == "PUT")
    {
        std::cout << "Received PUT from client: " << client.fd << std::endl;
        // 1) Sprawdź składnię: „PUT <point> <value>”
        std::istringstream iss(line);
        std::string      cmd;
        int              point;
        double           value;
        if (!(iss >> cmd >> point >> value))
        {
            badMessage(client, line);
            return;
        }
        // 2) Upewnij się, że nie ma „dodatkowego tekstu” po value
        std::string extra;
        if (iss >> extra)
        {
            badMessage(client, line);
            return;
        }

        // 3) Przekaż dalej do obsługi PUT
        handlePut(client, point, value);
        return;
    }

    // Jeśli nie HELLO ani PUT → błąd
    badMessage(client, line);
    disconnectClient(client.fd);
}

void Server::handleHello(ClientSession& client, std::string playerId)
{
    // 1) Ustaw playerId i flagę hello_received
    client.playerId     = playerId;
    client.hello_recived = true;

    // 2) Pobierz następną linię COEFF, parsuj og_coeffs, i wyślij COEFF do klienta
    addCoeffsToBuffer(client);

    std::cout << "Client " << client.playerId << " connected." << std::endl;
}

void Server::handlePut(ClientSession& client, int point, double value)
{
    // 1) Jeśli PUT niedozwolony → kara +20, od razu wyślij PENALTY
    if (!client.put_allowed)
    {
        client.score += 20;
        addPenaltyToBuffer(client, point, value);
        return;
    }

    // 2) Sprawdź poprawność range(point) i range(value) i precyzji
    if (!(point >= 0 && point <= K_) ||
        !(value >= -5 && value <= 5) ||
        !hasMax7DecimalPlaces(value))
    {
        client.score += 10;
        addBadPutToBuffer(client, point, value);
        return;
    }

    // 3) Poprawny PUT: wyłącz natychmiast kolejne PUT-y, zaktualizuj aproksymację
    client.put_allowed = false;
    client.approx[point] += value;
    client.client_put_count++;
    putCount_++;

    // 4) Wylicz opóźnienie wg liczby małych liter w playerId i dodaj STATE do opóźnionych
    time_t delay = std::count_if(
        client.playerId.begin(),
        client.playerId.end(),
        [](unsigned char c) { return std::islower(c); }
    );
    std::cout << "Sending state with delay of " << delay
              << " seconds for client " << client.fd << std::endl;
    addStateDelayedSend(client, delay);
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Odczyt danych od klienta: budowanie linii zakończonych "\r\n"
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::handleClientData(size_t index)
{
    int    fd        = pollFds_[index].fd;
    char   buf[1024];
    ssize_t bytesRead = recv(fd, buf, sizeof(buf), 0);

    // 1) bytesRead == 0 → klient zamknął połączenie
    if (bytesRead == 0)
    {
        disconnectClient(fd);
        return;
    }
    // 2) bytesRead < 0 → błąd (EOF lub EAGAIN) → rozłącz
    if (bytesRead < 0)
    {
        disconnectClient(fd);
        return;
    }

    // 3) Doklej odebrane bajty do rcv_buffer
    ClientSession& client = clients_.at(fd);
    client.rcv_buffer.append(buf, bytesRead);
    std::cout << "Received data from client " << fd << ": "
              << client.rcv_buffer << std::endl;
    // 4) Jeśli bufory zbyt duże → podejrzenie ataku → rozłącz
    if (client.rcv_buffer.size() > 4096)
    {
        badMessage(client, "Buffer overflow");
        disconnectClient(fd);
        return;
    }

    // 5) Wyciągnij pełne linie „<treść>\r\n” i wrzuć do handle_buffer_q
    size_t pos;
    while ((pos = client.rcv_buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = client.rcv_buffer.substr(0, pos);
        client.rcv_buffer.erase(0, pos + 2);
        client.handle_buffer_q.push(line);
    }
}

void Server::handleReceivedLine(ClientSession& client)
{
    if (!client.handle_buffer_q.empty())
    {
        std::string line = client.handle_buffer_q.front();
        client.handle_buffer_q.pop();
        parseAndProcessClientMessage(client, line);
    }
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Wysyłanie danych do klienta: kolejka send_buffer_q
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::sendScoringFromBuffer(ClientSession& client)
{
    if (client.scoring_sent) 
                        return;
    if (client.send_buffer_q.empty())
    {
        std::cout << "No scoring to send to client " << client.fd << std::endl;
        return;
    }

    // 1) Pobierz komunikat SCORING z send_buffer_q
    std::string& msg = client.send_buffer_q.front();
    std::cout << "Sending SCORING message to client " << client.fd << ": " << msg;

    // 2) Wyślij komunikat
    ssize_t bytes_sent = send(client.fd, msg.c_str(), msg.size(), 0);
    if (bytes_sent < 0)
    {
        perror("send");
        disconnectClient(client.fd);
        return;
    }

    // 3) Sprawdź, czy wysłano całość
    if ((size_t)bytes_sent < msg.size())
    {
        std::cout << " (partial send: " << bytes_sent << " bytes)" << std::endl;
        msg.erase(0, bytes_sent);
    }
    else
    {
        std::cout << " (full send)" << std::endl;
        client.send_buffer_q.pop();
        client.scoring_is_being_sent = false;
        client.scoring_sent = true; // Oznacz jako wysłane
    }
}


void Server::sendFromBuffer(ClientSession& client)
{
    if (client.send_buffer_q.empty())
    {
        //std::cout << "No messages to send to client " << client.fd << std::endl;
        return;
    }

    // 1) Weź pierwszą wiadomość (może być fragmentowana)
    std::string& msg = client.send_buffer_q.front();
    std::cout << "Sending message to client " << client.fd << ": " << msg;
    // 2) Sprawdź, czy zaczyna się od "COEFF" lub "STATE" → ustaw flagę
    bool startsWithCoeffOrState =
        (msg.rfind("COEFF", 0) == 0) || (msg.rfind("STATE", 0) == 0);

    if (startsWithCoeffOrState)
    {
        client.COEFF_or_STATE_is_being_sent = true;
    }

    // 3) Wyślij jak najwięcej bajtów
    ssize_t bytes_sent = send(client.fd, msg.c_str(), msg.size(), 0);
    if (bytes_sent < 0)
    {
        perror("send");
        disconnectClient(client.fd);
        return;
    }

    // 4) Jeśli nie wszystko wysłane, usuń wysłaną część; inaczej usuń całość i ewentualnie włącz put_allowed
    if ((size_t)bytes_sent < msg.size())
    {
        std::cout << " (partial send: " << bytes_sent << " bytes)" << std::endl;
        msg.erase(0, bytes_sent);
    }
    else
    {
        std::cout << " (full send)" << std::endl;
        client.send_buffer_q.pop();
        if (startsWithCoeffOrState)
        {
            client.COEFF_or_STATE_is_being_sent = false;
            client.put_allowed = true;
        }
    }
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Budowanie i wysyłanie poszczególnych komunikatów do wysłania
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::addStateToBuffer(ClientSession& client)
{
    std::cout << "Adding STATE to buffer for client " << client.fd << std::endl;
    std::string state_msg = "STATE";
    for (double aproxVal : client.approx)
    {
        state_msg += " " + std::to_string(aproxVal);
    }
    state_msg += "\r\n";
    client.send_buffer_q.push(state_msg);
}

void Server::addBadPutToBuffer(ClientSession& client, int point, int value)
{
    std::cout << "Adding BAD_PUT for client " << client.fd
              << ": point=" << point << ", value=" << value << std::endl;
    std::string bad_put_msg = "BAD_PUT " + std::to_string(point) + " " + std::to_string(value) + "\r\n";
    client.send_buffer_q.push(bad_put_msg);
}

void Server::addPenaltyToBuffer(ClientSession& client, int point, int value)
{
    std::cout << "Adding PENALTY for client " << client.fd
              << ": point=" << point << ", value=" << value << std::endl;
    std::string penalty_msg = "PENALTY " + std::to_string(point) + " " + std::to_string(value) + "\r\n";
    client.send_buffer_q.push(penalty_msg);
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Parsowanie współczynników: loadNextCoeff + parseCoeffsLine + addCoeffsToBuffer
└──────────────────────────────────────────────────────────────────────────────────*/
std::vector<double> Server::parseCoeffsLine()
{
    std::istringstream iss(coeffsLine_);
    std::vector<double> coeffs;
    double              value;
    while (iss >> value)
    {
        coeffs.push_back(value);
    }
    return coeffs;
}

void Server::addCoeffsToBuffer(ClientSession& client)
{
    // 1) Wczytaj rawLine, odetnij "COEFF", zapisz w coeffsLine_
    loadNextCoeff();

    // 2) Parsuj wartości double do og_coeffs
    client.og_coeffs = parseCoeffsLine();

    // 3) Zbuduj "COEFF <coeffsLine_>\r\n" i wrzuć do kolejki
    client.send_buffer_q.push("COEFF " + coeffsLine_ + "\r\n");
    std::cout << "Added COEFF to buffer for client " << client.fd
              << ":(" << coeffsLine_ << ")" << std::endl;
}

void Server::loadNextCoeff()
{
    std::string rawLine;
    if (!std::getline(coeffsStream_, rawLine))
    {
        std::cerr << "ERROR: failed to read coefficients line from file." << std::endl;
        coeffsLine_.clear();
        return;
    }

    std::istringstream iss(rawLine);
    std::string        prefix;
    iss >> prefix; // odrzuć "COEFF"

    std::ostringstream oss;
    double             val;
    bool               first = true;
    while (iss >> val)
    {
        if (!first) oss << ' ';
        oss << val;
        first = false;
    }
    coeffsLine_ = oss.str();
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Estado opóźnionego wysyłania "STATE" (wg delay_by sekund)
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::addStateDelayedSend(ClientSession& client, time_t delayed_by)
{
    std::ostringstream oss;
    oss << "STATE";
    for (double aproxVal : client.approx)
    {
        oss << " " << aproxVal;
    }
    oss << "\r\n";

    DelayedMessage delayedMsg(oss.str(), delayed_by);
    client.DelayedMessages.push_back(delayedMsg);
}

/*──────────────────────────────────────────────────────────────────────────────────
│ Rozłączanie klienta i sprzątanie po nim
└──────────────────────────────────────────────────────────────────────────────────*/
void Server::disconnectClient(int fd)
{
    // 1) Odejmij zliczone PUT-y klienta od putCount_
    putCount_ -= clients_[fd].client_put_count;

    // 2) Zamknij gniazdo i usuń z mapy clients_
    close(fd);
    clients_.erase(fd);

    // 3) Usuń fd z listy pollFds_
    for (auto it = pollFds_.begin(); it != pollFds_.end(); ++it)
    {
        if (it->fd == fd)
        {
            pollFds_.erase(it);
            break;
        }
    }

    std::cout << "Client disconnected: FD " << fd << std::endl;
}

std::string Server::getClientAddressAndPort(int fd)
{
    sockaddr_storage addr;
    socklen_t         addrLen = sizeof(addr);

    if (getpeername(fd, (sockaddr*)&addr, &addrLen) == 0)
    {
        char host[NI_MAXHOST], service[NI_MAXSERV];
        if (getnameinfo((sockaddr*)&addr, addrLen,
                        host, sizeof(host), service, sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        {
            return std::string(host) + ":" + std::string(service);
        }
    }
    return "unknown";
}
