#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <algorithm>
#include <iomanip>

// --- Stałe protokołu ---
#define HELLO           1
#define HELLO_REPLY     2
#define CONNECT         3
#define ACK_CONNECT     4
#define SYNC_START     11
#define DELAY_REQUEST  12
#define DELAY_RESPONSE 13
#define LEADER         21
#define GET_TIME       31
#define TIME           32

static constexpr int POLL_TIMEOUT_MS   = 1000;
static constexpr size_t MAX_UDP_PAYLOAD = 65507;

// --- Funkcje pomocnicze ---
// Funkcja pomocnicza do wypisywania komunikatów błędów.
void print_error_msg(const uint8_t* data, size_t len) {
    std::cerr << "ERROR MSG ";
    if (len == 0) {
        std::cerr << "00\n";
        return;
    }
    std::ios old(nullptr);
    old.copyfmt(std::cerr);

    std::cerr << std::hex << std::nouppercase << std::setfill('0');
    for (size_t i = 0; i < len && i < 10; ++i) {
        std::cerr << std::setw(2) << (int)data[i];
    }
    std::cerr << std::dec << "\n";

    std::cerr.copyfmt(old);
}


// Rozwiązanie nazwy hosta lub kropkowanego IPv4
bool resolve_ipv4(const char* host, in_addr& out) {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0) return false;
    out = ((sockaddr_in*)res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return true;
}

struct Peer {
    // IPv4 + port peer-a
    sockaddr_in addr;
    // Poziom synchronizacji tego peer-a [0..254] lub 255 = niesynchronizowany.
    uint8_t sync_level;
    // Ostatni moment, kiedy otrzymaliśmy od niego jakikolwiek
    // pakiet (ms od startu).
    uint64_t last_seen_ms;
    // Parametry pomiaru opóźnienia:
    //  - t1: czas peer-a przy wysyłaniu SYNC_START
    //  - t2: lokalny czas przy odbiorze SYNC_START
    //  - t3: lokalny czas przy wysłaniu DELAY_REQUEST
    uint64_t t1, t2, t3;
};

class PeerNode {
public:
    // Konstruktor klasy PeerNode, inicjalizuje węzeł z podanymi parametrami.
    // bind_port: port, na którym węzeł nasłuchuje.
    // bind_addr: adres IPv4, na którym węzeł nasłuchuje.
    // peer_port: port początkowego peer-a.
    // peer_addr: adres IPv4 początkowego peer-a.
    PeerNode(uint16_t bind_port,
             const in_addr* bind_addr,
             uint16_t peer_port,
             const in_addr* peer_addr)
      : bind_port_(bind_port)
      , sync_level_(255) // Domyślnie niesynchronizowany.
      , clock_offset_(0)
      , last_sync_send_(0)
      , last_sync_recv_(0)
      , leader_mode_(false) // Domyślnie węzeł nie jest liderem.
    {
        // Jeśli podano adres początkowego peer-a, ustaw go.
        if (peer_addr) {
            initial_peer_.sin_family = AF_INET;
            initial_peer_.sin_port   = htons(peer_port);
            initial_peer_.sin_addr   = *peer_addr;
            have_initial_peer_ = true;
        }
        // Ustaw adres nasłuchiwania (domyślnie INADDR_ANY).
        if (bind_addr) bind_addr_ = *bind_addr;
        else           bind_addr_.s_addr = htonl(INADDR_ANY);
    }

    // Główna metoda uruchamiająca węzeł.
    void run() {
        std::signal(SIGPIPE, SIG_IGN); // Ignoruj sygnał SIGPIPE.
        setup_socket();               // Konfiguracja gniazda.
        if (have_initial_peer_) send_hello(); // Wyślij HELLO do początkowego peer-a.
        main_loop();                  // Rozpocznij główną pętlę programu.
    }

private:
    // Peer, z którym aktualnie jesteśmy zsynchronizowani.
    // nullptr oznacza, że nie jesteśmy zsynchronizowani.
    Peer *synchronized_with_ = nullptr;
    // Losowy odstęp (w ms) pomiędzy wysyłkami kolejnych SYNC_START.
    uint64_t next_sync_interval_ = 5000 + (rand() % 5001);
    // Port, na którym nasłuchuje ten węzeł (0 → dowolny dostępny port).
    uint16_t bind_port_;
    // Adres IPv4, na którym nasłuchuje ten węzeł.
    in_addr bind_addr_;
    // Adres i port początkowego peer-a.
    sockaddr_in initial_peer_{};
    // Flaga mówiąca, czy mamy zdefiniowanego initial_peer_.
    bool have_initial_peer_ = false;
    // Deskryptor UDP socketu używanego do komunikacji.
    int sockfd_;
    // Lista znanych peerów w sieci.
    std::vector<Peer> peers_;
    // Mój bieżący poziom synchronizacji:
    // 255 = niesynchronizowany, 0 = lider, 1..254 = głębokość drzewa.
    uint8_t sync_level_;
    // Różnica (offset) w ms między moim naturalnym zegarem
    // a zsynchronizowanym czasem.
    int64_t clock_offset_;
    // Ostatnie czasy wysłania i odebrania SYNC_START (w ms od startu programu)
    uint64_t last_sync_send_;
    uint64_t last_sync_recv_;
    // Czy węzeł pełni obecnie rolę lidera?
    bool leader_mode_;
    // czy czekamy na DELAY_RESPONSE?
    bool     waiting_delay_       = false;    
    // timestamp wysłania DELAY_REQUEST
    uint64_t delay_request_time_  = 0;       
    static constexpr uint64_t EXCHANGE_TIMEOUT_MS = 10000; // 10 s

    static uint64_t now_ms() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
                   steady_clock::now().time_since_epoch()
               ).count();
    }

    // Konfiguruje gniazdo UDP do komunikacji.
    void setup_socket() {
        sockfd_ = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (sockfd_ < 0) perror_exit("socket");

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port   = htons(bind_port_);
        local.sin_addr   = bind_addr_;

        if (bind(sockfd_, (sockaddr*)&local, sizeof(local)) < 0)
            perror_exit("bind");

        // Jeśli port to 0, pobierz rzeczywisty port po bindowaniu.
        if (bind_port_ == 0) {
            socklen_t sz = sizeof(local);
            getsockname(sockfd_, (sockaddr*)&local, &sz);
            bind_port_ = ntohs(local.sin_port);
        }
        std::cout << "Listening on "
                  << inet_ntoa(local.sin_addr) << ":"
                  << bind_port_ << std::endl;
    }

    // Wysyła wiadomość HELLO do początkowego peer-a.
    void send_hello() {
        uint8_t msg = HELLO;
        sendto(sockfd_, &msg, 1, 0,
               (sockaddr*)&initial_peer_, sizeof(initial_peer_));
    }

    // Wysyła wiadomość SYNC_START do wszystkich znanych peer-ów.
    void send_sync_start() {
        std::vector<uint8_t> o;
        o.push_back(SYNC_START);
        o.push_back(sync_level_); // Aktualny poziom synchronizacji.
        uint64_t t1 = now_ms() + clock_offset_; // Zsynchronizowany czas.
        uint64_t be = htobe64(t1);
        auto ptr = (uint8_t*)&be;
        o.insert(o.end(), ptr, ptr+8);
        for (auto& p : peers_)
            sendto(sockfd_, o.data(), o.size(), 0,
                   (sockaddr*)&p.addr, sizeof(p.addr));
    }

    // Główna pętla programu, obsługuje komunikację i synchronizację.
    void main_loop() {
        std::vector<pollfd> fds(1);
        fds[0] = { sockfd_, POLLIN, 0 };
        std::vector<uint8_t> buf(MAX_UDP_PAYLOAD);

        while (true) {
            int ret = poll(fds.data(), 1, POLL_TIMEOUT_MS);
            if (ret < 0) perror_exit("poll");

            // Obsługa przychodzących wiadomości.
            if (fds[0].revents & POLLIN) {
                sockaddr_in s{};
                socklen_t    sl = sizeof(s);
                ssize_t n = recvfrom(sockfd_,
                                     buf.data(), buf.size(),
                                     0, (sockaddr*)&s, &sl);
                if (n > 0) {
                    handle_msg(s, buf.data(), (size_t)n);
                }
                else {
                    print_error_msg(buf.data(), (size_t)n);
                }
            }

            // Obsługa timeoutów i synchronizacji.
            if (waiting_delay_ && now_ms() - delay_request_time_ > EXCHANGE_TIMEOUT_MS) {
              waiting_delay_ = false;
              sync_level_    = 255;
            }

            if (sync_level_ != 255 && now_ms() - last_sync_recv_ > 30000) {
                sync_level_ = 255;
                std::cout << "Lost sync coz time → level=255" << std::endl;
            }

            if (leader_mode_ && (now_ms() - last_sync_send_) >= 2000) {
                send_sync_start();
                last_sync_send_ = now_ms();
                next_sync_interval_ = 5000 + (rand() % 5001);
            } else if (!waiting_delay_ && sync_level_ < 254 &&
                       (now_ms() - last_sync_send_) >= next_sync_interval_) {
                send_sync_start();
                last_sync_send_ = now_ms();
                next_sync_interval_ = 5000 + (rand() % 5001);
            }
        }
    }

    // Obsługuje przychodzące wiadomości w zależności od ich typu.
    void handle_msg(const sockaddr_in& s,
                    const uint8_t* d, size_t len) {
        if (len < 1) {
            printf("ERROR: empty message\n");
            print_error_msg(d, len);
            return;
        }
        switch (d[0]) {
          case HELLO:
            handle_hello(s);
            break;
          case HELLO_REPLY:
            handle_hello_reply(s, d, len);
            break;
          case CONNECT:
            handle_connect(s);
            break;
          case ACK_CONNECT:
            handle_ack(s);
            break;
          case SYNC_START:
            handle_sync_start(s, d, len);
            break;
          case DELAY_REQUEST:
            handle_delay_request(s);
            break;
          case DELAY_RESPONSE:
            handle_delay_response(s, d, len);
            break;
          case LEADER:
            handle_leader(s, d, len);
            break;
          case GET_TIME:
            handle_get_time(s);
            break;
          case TIME:
            handle_time(s, d, len);
            break;
          default:
            printf("ERROR: unknown message type %d\n", d[0]);
            print_error_msg(d, len);
        }
    }

    // Dodaje nowego peer-a, jeśli jeszcze go nie znamy.
    bool add_peer_if_new(const sockaddr_in& a) {
        for (auto& p : peers_)
            if (p.addr.sin_addr.s_addr == a.sin_addr.s_addr &&
                p.addr.sin_port       == a.sin_port)
                return false;
        peers_.push_back({a, 255, now_ms(),0,0,0});
        return true;
    }

    // Znajduje peer-a na podstawie adresu.
    Peer* find_peer(const sockaddr_in& a) {
        for (auto& p : peers_)
            if (p.addr.sin_addr.s_addr == a.sin_addr.s_addr &&
                p.addr.sin_port       == a.sin_port)
                return &p;
        return nullptr;
    }

    // Obsługuje wiadomość HELLO.
    void handle_hello(const sockaddr_in& s) {
        add_peer_if_new(s);
        // Wysyła HELLO_REPLY z listą znanych peer-ów.
        std::vector<uint8_t> o;
        o.push_back(HELLO_REPLY);
        uint16_t cnt = peers_.size() - 1;
        size_t needed = 1 + 2 + size_t(cnt) * (1 + 4 + 2);
        if (needed > MAX_UDP_PAYLOAD) return;
        uint16_t nc = htons(cnt);
        uint8_t* nc_ptr = (uint8_t*)&nc;
        o.push_back(nc_ptr[0]);
        o.push_back(nc_ptr[1]);
        for (auto& q : peers_) {
            if (q.addr.sin_addr.s_addr == s.sin_addr.s_addr &&
                q.addr.sin_port       == s.sin_port)
                continue;
            o.push_back(4);
            auto ip = (uint8_t*)&q.addr.sin_addr.s_addr;
            o.insert(o.end(), ip, ip+4);
            uint16_t pr = q.addr.sin_port;
            uint8_t* pr_ptr = (uint8_t*)&pr;
            o.push_back(pr_ptr[0]);
            o.push_back(pr_ptr[1]);
        }
        sendto(sockfd_, o.data(), o.size(), 0,
               (sockaddr*)&s, sizeof(s));
    }

    // Obsługuje wiadomość HELLO_REPLY.
    void handle_hello_reply(const sockaddr_in& s, const uint8_t* d, size_t l) {
        // Dodaje nadawcę jako peer-a i przetwarza listę peer-ów.
        size_t off = 1;
        if (l < off + 2) return;
        add_peer_if_new(s);
        sockaddr_in self_addr{};
        self_addr.sin_family = AF_INET;
        self_addr.sin_port = htons(bind_port_);
        self_addr.sin_addr = bind_addr_;
        uint16_t netc;
        memcpy(&netc, d + off, 2);
        uint16_t cnt = ntohs(netc);
        off += 2;
        for (int i = 0; i < (int)cnt; ++i) {
            if (off + 1 > l) return;
            uint8_t al = d[off++];
            if (al != 4 || off + 6 > l) {
                off += al + 2;
                continue;
            }
            sockaddr_in pr{};
            pr.sin_family = AF_INET;
            memcpy(&pr.sin_addr.s_addr, d + off, 4);
            off += 4;
            memcpy(&pr.sin_port, d + off, 2);
            off += 2;
            if (pr.sin_addr.s_addr == self_addr.sin_addr.s_addr &&
                pr.sin_port == self_addr.sin_port) continue;
            if (pr.sin_addr.s_addr == s.sin_addr.s_addr &&
                pr.sin_port == s.sin_port) continue;
            add_peer_if_new(pr);
        }
        for (auto& q : peers_) {
            if (q.addr.sin_addr.s_addr == s.sin_addr.s_addr &&
                q.addr.sin_port == s.sin_port) continue;
            uint8_t m = CONNECT;
            sendto(sockfd_, &m, 1, 0, (sockaddr*)&q.addr, sizeof(q.addr));
        }
    }

    // Obsługuje wiadomość CONNECT.
    void handle_connect(const sockaddr_in& s) {
        if (add_peer_if_new(s)) {
            uint8_t m = ACK_CONNECT;
            sendto(sockfd_, &m, 1, 0,
                   (sockaddr*)&s, sizeof(s));
        }
    }

    // Obsługuje wiadomość ACK_CONNECT.
    void handle_ack(const sockaddr_in& s) {
        add_peer_if_new(s);
    }

    // Obsługuje wiadomość SYNC_START.
    void handle_sync_start(const sockaddr_in &s, const uint8_t *d, size_t l) {
        // Przetwarza synchronizację czasu z peer-em.
        size_t off = 1;
        if (l < off + 9) return;
        Peer *p = find_peer(s);
        if (!p) return;
        uint8_t lvl = d[off++];
        uint64_t nett;
        memcpy(&nett, d + off, 8);
        uint64_t T1 = be64toh(nett);
        if (lvl == 255) return;
        bool should_sync = false;
        if (synchronized_with_) {
            if (p == synchronized_with_) {
                if (sync_level_ >= lvl) {
                    sync_level_ = 255;
                    return;
                }
                if (lvl < p->sync_level) {
                    should_sync = true;
                }
            } else {
                if (lvl <= static_cast<uint8_t>(p->sync_level - 2)) {
                    should_sync = true;
                }
            }
        } else {
            should_sync = true;
        }
        if (!should_sync) return;
        p->last_seen_ms = now_ms();
        uint64_t T2 = now_ms();
        uint8_t  r  = DELAY_REQUEST;
        sendto(sockfd_, &r, 1, 0, (sockaddr*)&s, sizeof(s));
        waiting_delay_      = true;
        delay_request_time_ = now_ms();
        uint64_t T3 = now_ms();
        p->t1 = T1; p->t2 = T2; p->t3 = T3;
        sync_level_     = lvl + 1;
        last_sync_recv_ = now_ms();
    }

    // Obsługuje wiadomość DELAY_REQUEST.
    void handle_delay_request(const sockaddr_in& s) {
        Peer* p = find_peer(s);
        if (!p) return;
        std::vector<uint8_t> o;
        o.push_back(DELAY_RESPONSE);
        o.push_back(p->sync_level);
        uint64_t T4 = now_ms() + clock_offset_;
        uint64_t be = htobe64(T4);
        uint8_t* ptr = (uint8_t*)&be;
        o.insert(o.end(), ptr, ptr+8);
        sendto(sockfd_, o.data(), o.size(), 0, (sockaddr*)&s, sizeof(s));
    }

    // Obsługuje wiadomość DELAY_RESPONSE.
    void handle_delay_response(const sockaddr_in& s, const uint8_t* d, size_t l) {
        size_t off = 1;
        if (l < off + 9) return;
        uint8_t lvl = d[off++];
        uint64_t nett; memcpy(&nett, d + off, 8);
        uint64_t T4 = be64toh(nett);
        Peer* p = find_peer(s);
        if (!p || p != synchronized_with_) return;
        int64_t offset = (int64_t)(p->t2 - p->t1 + p->t3 - T4)/2;
        clock_offset_ = offset;
        sync_level_   = lvl + 1;
        waiting_delay_ = false;
    }

    // Obsługuje wiadomość LEADER.
    void handle_leader(const sockaddr_in& s, const uint8_t* d, size_t l) {
        if (l < 2) return;
        uint8_t lvl = d[1];
        if (lvl == 0) {
            sync_level_ = 0;
            leader_mode_ = true;
            last_sync_send_ = now_ms() - (5000 - 2000);
        } else if (lvl == 255 && leader_mode_) {
            sync_level_ = 255;
            leader_mode_ = false;
        }
    }

    // Obsługuje wiadomość GET_TIME.
    void handle_get_time(const sockaddr_in& s) {
        uint64_t t = now_ms();
        if (sync_level_ != 255) t += clock_offset_;
        std::vector<uint8_t> o;
        o.push_back(TIME);
        o.push_back(sync_level_);
        uint64_t be = htobe64(t);
        auto ptr = (uint8_t*)&be;
        o.insert(o.end(), ptr, ptr+8);
        sendto(sockfd_, o.data(), o.size(), 0,
               (sockaddr*)&s, sizeof(s));
    }

    // Obsługuje wiadomość TIME (nie wymaga akcji).
    void handle_time(const sockaddr_in& /*s*/,
                     const uint8_t* /*d*/, size_t /*l*/) {
        // Nie musimy nic robić.
    }

    void perror_exit(const char* msg) {
        std::perror(msg);
        std::exit(EXIT_FAILURE);
    }
};

int main(int argc, char* argv[]) {
    uint16_t bind_port = 0, peer_port = 0;
    in_addr   bind_addr{}, peer_addr{};
    bool      b_used = false, p_used = false;
    bool      a_used = false, r_used = false;

    int opt;
    while ((opt = getopt(argc, argv, "b:p:a:r:")) != -1) {
        switch (opt) {
          case 'b': {
            if (b_used) { std::cerr<<"ERROR: duplicate -b\n"; return EXIT_FAILURE; }
            if (!resolve_ipv4(optarg, bind_addr)) {
                std::cerr<<"ERROR: Invalid bind address: "<<optarg<<std::endl;
                return EXIT_FAILURE;
            }
            b_used = true;
            break;
          }
          case 'p': {
            if (p_used) { std::cerr<<"ERROR: duplicate -p\n"; return EXIT_FAILURE; }
            char* end; long v = strtol(optarg, &end, 10);
            if (*end || v < 0 || v > 65535) {
                std::cerr<<"ERROR: Port must be 0–65535\n";
                return EXIT_FAILURE;
            }
            bind_port = (uint16_t)v;
            p_used    = true;
            break;
          }
          case 'a': {
            if (a_used) { std::cerr<<"ERROR: duplicate -a\n"; return EXIT_FAILURE; }
            if (!resolve_ipv4(optarg, peer_addr)) {
                std::cerr<<"ERROR: Invalid peer address: "<<optarg<<std::endl;
                return EXIT_FAILURE;
            }
            a_used = true;
            break;
          }
          case 'r': {
            if (r_used) { std::cerr<<"ERROR: duplicate -r\n"; return EXIT_FAILURE; }
            char* end; long v = strtol(optarg, &end, 10);
            if (*end || v < 1 || v > 65535) {
                std::cerr<<"ERROR: Peer port must be 1–65535\n";
                return EXIT_FAILURE;
            }
            peer_port = (uint16_t)v;
            r_used    = true;
            break;
          }
          default:
            std::cerr<<"Usage: "<<argv[0]<<" [-b bind_addr] [-p port] [-a peer_addr -r peer_port]\n";
            return EXIT_FAILURE;
        }
    }

    // -a i -r razem lub wcale; ten sam dla -b/-p nie jest wymagany
    if (a_used ^ r_used) {
        std::cerr<<"ERROR: -a and -r must be used together\n";
        return EXIT_FAILURE;
    }

    PeerNode node(bind_port,
                  b_used ? &bind_addr : nullptr,
                  peer_port,
                  a_used ? &peer_addr : nullptr);
    node.run();
    return EXIT_SUCCESS;
}
