//
// Created by mateusz on 24.04.2021.
//

#include "server.h"
#include <stdexcept>

#define MAX_PORT_NUM 65535
#define DEF_PORT_NUM  8080
#define INVALID_PORT_NUM "Invalid port number!"

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        exit_error("Usage: serwer <nazwa-katalogu-z-plikami> <plik-z-serwerami-skorelowanymi> [<numer-portu-serwera>]");
    }
    std::string FILE_DIR = argv[1];
    const std::string SERVER_DIR = argv[2];
    uint32_t server_port_num = DEF_PORT_NUM;
    if (argc == 4) {
        try {
            size_t size;
            std::string arg_3 = argv[3];
            server_port_num = std::stoi(arg_3, &size);
            if (size != arg_3.size())
                throw std::invalid_argument(INVALID_PORT_NUM);
        } catch (...) {
            exit_error(INVALID_PORT_NUM);
        }
        if (server_port_num > MAX_PORT_NUM)
            exit_error(INVALID_PORT_NUM);
    }

    Server server(FILE_DIR, SERVER_DIR, server_port_num);
    server.run();
}
