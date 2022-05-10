//
// Created by mateusz on 24.04.2021.
//

#ifndef ZADANIE_1_SERVER_H
#define ZADANIE_1_SERVER_H

#include <experimental/filesystem>
#include <cstdlib>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <ext/stdio_filebuf.h>
#include <fcntl.h>
#include <csignal>
#include "http.h"

#define QUEUE_LENGTH 5

// Prints message to stderr and exits program with code EXIT_FAILURE.
void exit_error(const std::string &message);

// Contains some useful definitions and methods that can be used when dealing
// with remote servers.
namespace remote {
    // First element - servers IP address, second element - server port.
    using server_t = std::pair<std::string, int>;
    // Maps resources to servers.
    using rservers_t = std::map<std::string, server_t>;

    // Returns structure representing server containing 'res_path', searches the server in
    // remote_resources.
    // If no server was found, throws NoDirException.
    server_t get_resource(const std::string &res_path, const rservers_t &remote_resources);

    // Parses file with remote resources.
    // If parsing failed exits the program with EXIT_FAILURE.
    // If parsing was successfully returns rservers_t structure representing remote servers in
    // the file.
    auto parse_remote_resources(const std::string &server_dir);
}

namespace file_utils {
    namespace fs = std::experimental::filesystem;

    // Returns string containing canonized path.
    // Does it using filesystem::canonize() method.
    // Throws NoDirException if canonizing failed (path doesn't exist).
    std::string canonize(const std::string &path);

    // Returns 'true' if directory uncanonized_sub is contained in the 'canonized_base'.
    // As the parameter name suggest, canonized_base should contain canonicalized version of path.
    bool is_subpath_of(const std::string &canonized_base, const std::string &uncanonized_sub);

    class NoDirException : public std::exception {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Directory not found!";
        }
    };
}

class InputReader {
    // When read_request() failed for some reason, this flag is set to the appropriate response.
    Response errorResponse;

public:
    InputReader() = default;

    const Response &get_error_response() {
        return errorResponse;
    }

    // Reads request (reads until two CRLF in a row)
    // from 'input_stream' and changes 'request' to represent request that has been read.
    // If some error during parsing has occurred returns 'false' and changes 'errorResponse'
    // to represent the appropriate server response to the error.
    // If the request has been read successfully returns 'true'.
    bool read_request(Request &request, std::istream &input_stream);
};

class Server {
    struct sockaddr_in server_address{};
    int sock{};
    std::string base_directory, remote_servers_path;

    void create_socket() {
        sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
        if (sock < 0)
            exit_error("socket error");
    }

    void bind_socket() {
        if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
            exit_error("bind error");
    }

    void switch_to_listen() {
        if (listen(sock, QUEUE_LENGTH) < 0)
            exit_error("listen");
    }

    // Uses open() to get descriptor of the file represented by 'full_path'.
    // If open succeeded, returns file descriptor. Otherwise returns -1.
    int get_descriptor(const file_utils::fs::path &full_path);

    // Takes correct request and creates a response based on it.
    // Can return response with code 200, 302 or 404.
    // Doesn't check if "Connection: close" header appears in the request, so the response won't contain
    // this header either.
    Response get_file_response(const Request &request, const remote::rservers_t &remote_resources);

public:
    // Initializes the server by on port_num by creating socket, binding and switching to listen.
    // Updates other class fields.
    Server(const std::string &base_dir_arg, std::string server_path_arg, uint32_t port_num);

    // Runs the server in endless loop.
    void run();
};

#endif //ZADANIE_1_SERVER_H
