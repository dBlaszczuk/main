#include "Server.h"
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char* argv[]) {
    unsigned short port = 0; // Domyślnie 0
    int K = 100;             // Domyślnie 100
    int N = 4;               // Domyślnie 4
    int M = 131;             // Domyślnie 131
    std::string coeffs_file;

    int opt;
    while ((opt = getopt(argc, argv, "p:k:n:m:f:")) != -1) {
        switch (opt) {
            case 'p':
                port = static_cast<unsigned short>(std::stoi(optarg));
                if (port < 0 || port > 65535) {
                    std::cerr << "ERROR: Port must be in range 0-65535.\n";
                    return 1;
                }
                break;
            case 'k':
                K = std::stoi(optarg);
                if (K < 1 || K > 10000) {
                    std::cerr << "ERROR: K must be in range 1-10000.\n";
                    return 1;
                }
                break;
            case 'n':
                N = std::stoi(optarg);
                if (N < 1 || N > 8) {
                    std::cerr << "ERROR: N must be in range 1-8.\n";
                    return 1;
                }
                break;
            case 'm':
                M = std::stoi(optarg);
                if (M < 1 || M > 12341234) {
                    std::cerr << "ERROR: M must be in range 1-12341234.\n";
                    return 1;
                }
                break;
            case 'f':
                coeffs_file = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p PORT -k K -n N -m M -f COEFFS_FILE\n";
                return 1;
        }
    }

    if (coeffs_file.empty()) {
        std::cerr << "ERROR: Coefficients file (-f) is required.\n";
        return 1;
    }

    Server server(port, K, N, M, coeffs_file);
    server.run();

    return 0;
}
