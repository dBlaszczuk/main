#include "Client.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char* argv[]) {
    std::string host;
    unsigned short port = 0; 
    std::string playerId;
    bool autoMode = false;
    bool forceIPv4 = false;
    bool forceIPv6 = false;

    int opt;
    while ((opt = getopt(argc, argv, "s:p:u:46a")) != -1) {
        switch (opt) {
            case 's':
                host = optarg;
                break;
            case 'p':
                port = static_cast<unsigned short>(std::stoi(optarg));
                if (port < 1 || port > 65535) {
                    std::cerr << "ERROR: Port must be in range 1-65535.\n";
                    return 1;
                }
                break;
            case 'u':
                playerId = optarg;
                break;
            case '4':
                forceIPv4 = true;
                break;
            case '6':
                forceIPv6 = true;
                break;
            case 'a':
                autoMode = true;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -s <host> -p <port> -u <player_id> [-4] [-6] [-a]\n";
                return 1;
        }
    }

    // Sprawdzenie wymaganych parametrów
    if (host.empty()) {
        std::cerr << "ERROR: Host (-s) is required.\n";
        return 1;
    }
    if (playerId.empty()) {
        std::cerr << "ERROR: Player ID (-u) is required.\n";
        return 1;
    }
    if (!port) {
        std::cerr << "ERROR: Port (-p) is required.\n";
        return 1;
    }
    
    int ipVersion = 0; // 4 = IPv4, 6 = IPv6
    // Podanie obu albo żadnej opcji -4 i -6
    if (!(forceIPv4 ^ forceIPv6)) {
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;

        getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);

        if (res->ai_family == AF_INET) {
            ipVersion = 4;
        } else if (res->ai_family == AF_INET6) {
            ipVersion = 6;
        } else {
            std::cerr << "ERROR: Unknown address family returned by getaddrinfo in main.\n";
            freeaddrinfo(res);
            return 1;
        }

        freeaddrinfo(res);
    }
    else if (forceIPv4) {
        ipVersion = 4;
    }
    else if (forceIPv6) {
        ipVersion = 6;
    }
    else {
        std::cerr << "ERROR: Invalid IP version.\n";
        return 1;
    }

    // Utworzenie klienta i uruchomienie
    Client client(host, port, playerId, autoMode, ipVersion);
    client.run();

    return 0;
}
