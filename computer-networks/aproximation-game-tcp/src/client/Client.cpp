#include "Client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cerrno>
#include <cmath>
#include <sstream>
#include <poll.h>
#include <algorithm>
#include <cctype>

Client::Client(const std::string& host, unsigned short port, 
               const std::string& playerId, bool autoMode, int ipVersion)
    : host_(host), port_(port), playerId_(playerId), 
      autoMode_(autoMode), ipVersion_(ipVersion) {
        if (ipVersion == 4) {
            ipVersion_ = AF_INET;
        } else if (ipVersion == 6) {
            ipVersion_ = AF_INET6;
        } else {
            ipVersion_ = AF_UNSPEC; // Obsługa domyślna
        }
      }

void Client::resolveHost() {
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipVersion_;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "ERROR: getaddrinfo - " << gai_strerror(status) << std::endl;
        exit(1);
    }

    // Próbuj połączyć się z każdym adresem z listy
    for (p = res; p != nullptr; p = p->ai_next) {
        sock_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock_ == -1) {
            perror("socket");
            continue;
        }

        // Ustaw non-blocking
        int flags = fcntl(sock_, F_GETFL, 0);
        fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

        if (connect(sock_, p->ai_addr, p->ai_addrlen) == -1) {
            if (errno == EINPROGRESS) {
                // Czekaj na połączenie z użyciem poll
                struct pollfd pf;
                pf.fd = sock_;
                pf.events = POLLOUT;

                int ret = poll(&pf, 1, 0); // 5 sekund timeout
                if (ret > 0) {
                    int so_error;
                    socklen_t len = sizeof(so_error);
                    getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &len);
                    if (so_error == 0) {
                        // Połączenie udane
                        break;
                    }
                }
            }
        } else {
            // Połączenie natychmiastowe
            break;
        }

        close(sock_);
    }

    if (p == nullptr) {
        std::cerr << "ERROR: Failed to connect to " << host_ << ":" << port_ << std::endl;
        exit(1);
    }
    std::cout << "Connected to " << host_ << ":" << port_ << " as " << playerId_ << std::endl;

    freeaddrinfo(res);
}

void Client::connectToServer() {
    resolveHost();
    queueHello(); // Wyślij HELLO natychmiast po połączeniu
}

void Client::run() {
    connectToServer();

    struct pollfd fds[2];
    fds[0].fd = sock_;
    fds[0].events = POLLIN | POLLERR | POLLHUP;
    
    fds[1].fd = STDIN_FILENO; // stdin
    fds[1].events = POLLIN;
    
    while (true) {
        int ret = poll(fds, 2, 0);
        if (ret < 0) {
            perror("poll");
            exit(1);
        }

        // Sprawdź błędy połączenia
        if (fds[0].revents & (POLLERR | POLLHUP)) {
            std::cerr << "ERROR: Connection error or hang up" << std::endl;
            exit(1);
        }

        // Odbierz wiadomości z serwera
        if (fds[0].revents & POLLIN) {
            readMessages();
            handleReceivedMessages();
        }

        // Obsłuż wejście z klawiatury (tylko w trybie interaktywnym)
        if (!autoMode_ && (fds[1].revents & POLLIN)) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                if (std::cin.eof()) continue; // Ignoruj EOF
                std::cerr << "ERROR: Failed to read input" << std::endl;
                continue;
            }
            
            std::istringstream iss(line);
            int point;
            double value;
            if (iss >> point >> value) {
                // Walidacja podstawowa
                if (point < 0) {
                    std::cerr << "ERROR: Point must be non-negative" << std::endl;
                    continue;
                }
                if (value < -5.0 || value > 5.0) {
                    std::cerr << "ERROR: Value must be in [-5, 5]" << std::endl;
                    continue;
                }

                if (has_coeff_) {
                    queuePut(point, value);
                } else {
                    pending_puts_.push({point, value});
                    std::cout << "Command queued (waiting for COEFF)" << std::endl;
                }
            } else {
                std::cerr << "ERROR: Invalid input - expected <point> <value>" << std::endl;
            }
        }

        // Wyślij buforowane wiadomości
        if (sendBufferedMessages() < 0) {
            std::cerr << "ERROR: Send error, disconnecting" << std::endl;
            exit(1);
        }
    }
}

void Client::readMessages() {
    char buffer[1024];
    ssize_t bytes_read = recv(sock_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return;
        perror("recv");
        exit(1);
    } else if (bytes_read == 0) {
        std::cerr << "ERROR: Connection closed by server" << std::endl;
        exit(1);
    }
    std::cout << "Received " << bytes_read << " bytes from server" << std::endl;
    rec_buffer_.append(buffer, bytes_read);

    size_t pos;
    while ((pos = rec_buffer_.find("\r\n")) != std::string::npos) {
        std::string line = rec_buffer_.substr(0, pos);
        rec_buffer_.erase(0, pos + 2);
        recQueue_.push(line);
    }
}

void Client::handleReceivedMessages() {
    std::cout << "Handling received messages..." << std::endl;
    while (!recQueue_.empty()) {
        std::cout << "Processing message: " << recQueue_.front() << std::endl;
        handleMessage(recQueue_.front());
        recQueue_.pop();
    }
}

void Client::handleMessage(const std::string& line) {
    if (line.find("COEFF") == 0) {        
        handleCoeff(line);
    } else if (line.find("BAD_PUT") == 0) {
        handleBadPut(line);
    } else if (line.find("STATE") == 0) {
        handleState(line);
    } else if (line.find("SCORING") == 0) {
        handleScoring(line);
    } else if (line.find("PENALTY") == 0) {
        handlePenalty(line);
    } else {
        std::cerr << "ERROR: Unknown message: " << line << std::endl;
    }
}

void Client::handleCoeff(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "COEFF"
    
    coeffs_.clear();
    double val;
    while (iss >> val) {
        coeffs_.push_back(val);
    }

    has_coeff_ = true;
    std::cout << "Received COEFF:";
    for (double c : coeffs_) std::cout << " " << c;
    std::cout << std::endl;

    // Inicjalizuj stan aproksymacji (K+1 punktów)
    approx_state_.resize(K_ + 1, 0.0);
    
    // Wyślij oczekujące polecenia PUT
    sendPendingPuts();
    
    // W trybie auto wygeneruj pierwsze PUT
    if (autoMode_) {
        auto [point, value] = autoStrategy();
        queuePut(point, value);
    }
}

void Client::sendPendingPuts() {
    while (!pending_puts_.empty()) {
        auto [point, value] = pending_puts_.front();
        pending_puts_.pop();
        queuePut(point, value);
    }
}

void Client::handleBadPut(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    int point;
    double value;
    iss >> token >> point >> value;

    std::cerr << "Received BAD_PUT for point " << point << " value " << value << std::endl;

    if (autoMode_) {
        auto [new_point, new_value] = autoStrategy();
        queuePut(new_point, new_value);
    }
}

void Client::handlePenalty(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    int point;
    double value;
    iss >> token >> point >> value;

    std::cerr << "Received PENALTY for point " << point << " value " << value << std::endl;

    // W trybie auto, zignoruj PENALTY, w trybie interaktywnym można by to obsłużyć
    if (autoMode_) {
        auto [new_point, new_value] = autoStrategy();
        queuePut(new_point, new_value);
    }
}

void Client::handleState(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "STATE"
    
    approx_state_.clear();
    double val;
    while (iss >> val) {
        approx_state_.push_back(val);
    }

    // Aktualizuj K na podstawie odebranych danych
    if (approx_state_.size() > 0) {
        K_ = approx_state_.size() - 1;
    }

    std::cout << "Received STATE:";
    for (double s : approx_state_) std::cout << " " << s;
    std::cout << std::endl;

    if (autoMode_) {
        auto [point, value] = autoStrategy();
        queuePut(point, value);
    }
}

void Client::handleScoring(const std::string& line) {
    std::cout << "Received SCORING: " << line << std::endl;
    std::cout << "Game finished" << std::endl;
    exit(0);
}

int Client::sendBufferedMessages() {
    if (sendQueue_.empty()) return 0;
    std::string& msg = sendQueue_.front();
    std::cout << "Sending buffered message:" << msg << std::endl;
    ssize_t bytes_sent = send(sock_, msg.data(), msg.size(), MSG_NOSIGNAL | MSG_DONTWAIT);

    if (bytes_sent < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
        perror("send");
        return -1;
    }

    std::cout << "Sent " << bytes_sent << " bytes";

    if (static_cast<size_t>(bytes_sent) == msg.size()) {
        std::cout << " (full send)" << std::endl;
        sendQueue_.pop();
    } else {
        std::cout << " (partial send: " << bytes_sent << " bytes)" << std::endl;
        msg = msg.substr(bytes_sent);
    }
    
    return bytes_sent;
}

std::pair<int, double> Client::autoStrategy() {
    if (approx_state_.empty() || coeffs_.empty()) {
        return {0, 0.0}; // Domyślne wartości
    }

    // Wybierz następny punkt (cyklicznie 0..K)
    int point = currentPoint_ % (K_ + 1);
    currentPoint_ = (currentPoint_ + 1) % (K_ + 1);

    // Oblicz wartość oczekiwaną
    double expected = 0.0;
    double x = point;
    double term = 1.0;
    for (double coeff : coeffs_) {
        expected += coeff * term;
        term *= x;
    }

    // Oblicz różnicę i ogranicz do zakresu [-5,5]
    double diff = expected - approx_state_[point];
    double value = std::max(-5.0, std::min(5.0, diff));

    return {point, value};
}

void Client::queueHello() {
    sendQueue_.push("HELLO " + playerId_ + "\r\n");
    std::cout << "Sent HELLO as " << playerId_ << std::endl;
}

void Client::queuePut(int point, double value) {
    // Formatuj wartość - pomiń część dziesiętną dla liczb całkowitych
    std::ostringstream oss;
    oss << "PUT " << point << " ";
    
    if (value == static_cast<int>(value)) {
        oss << static_cast<int>(value);
    } else {
        oss << value;
    }
    
    oss << "\r\n";
    sendQueue_.push(oss.str());
    std::cout << "Queued PUT: point=" << point << " value=" << value << std::endl;
}