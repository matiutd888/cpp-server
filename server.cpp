#include "server.h"

#include <utility>

remote::server_t remote::get_resource(const std::string &res_path, const rservers_t &remote_resources) {
    auto it = remote_resources.find(res_path);
    if (it == remote_resources.end()) {
        throw file_utils::NoDirException();
    }
    return it->second;
}

auto remote::parse_remote_resources(const std::string &server_dir) {
    remote::rservers_t remote_resources;
    std::ifstream f;
    f.open(server_dir);
    if (f.fail())
        exit_error("Opening file with remote resources failed!");

    std::string res_path, server_name;
    int port;
    while (f >> res_path >> server_name >> port) {
        remote_resources.insert({res_path, {server_name, port}});
    }
    f.close();
    return remote_resources;
}

bool file_utils::is_subpath_of(const std::string &canonized_base, const std::string &uncanonized_sub) {
    std::string canonized_sub;
    try {
        canonized_sub = file_utils::canonize(uncanonized_sub);
    } catch (const file_utils::NoDirException &noDirException) {
        return false;
    }
    auto m = std::mismatch(canonized_base.begin(), canonized_base.end(),
                           canonized_sub.begin(), canonized_sub.end());
    return m.first == canonized_base.end();
}

std::string file_utils::canonize(const std::string &path) {
    std::string p;
    try {
        p = file_utils::fs::canonical(path);
    } catch (...) {
        throw file_utils::NoDirException();
    }
    return p;
}


bool InputReader::read_request(Request &request, std::istream &input_stream) {
    std::string msg_fragment;
    bool is_eof = false;
    if (getline(input_stream, msg_fragment)) {
        try {
            request.parse_and_add_req_line(msg_fragment);
        } catch (const http::ParseException &p) {
            errorResponse = Response::create_400_response();
            std::cerr << p.what() << std::endl;
            return false;
        } catch (const http::InvalidMethodException &e) {
            errorResponse = Response::create_501_response();
            std::cerr << e.what() << std::endl;
            return false;
        }
    } else {
        is_eof = true;
    }
    while (!is_eof && getline(input_stream, msg_fragment)) {
        if (msg_fragment == "\r")
            return true;
        try {
            request.parse_and_add_header(msg_fragment);
        } catch (const http::ParseException &p) {
            std::cerr << p.what() << std::endl;
            errorResponse = Response::create_400_response();
            return false;
        } catch (const http::WrongHeaderException &p) {
            std::cerr << p.what() << std::endl;
        }
    }
    // If we are here getline failed, client most likely disconnected.
    std::cerr << "Error reading client lines!" << std::endl;
    errorResponse = Response::create_400_response(); // We probably won't be able to send that.
    return false;
}

int Server::get_descriptor(const file_utils::fs::path &full_path) {
    int file_descriptor = -1;
    file_utils::fs::path base_dir(base_directory);
    bool is_file_ok;
    try {
        is_file_ok = file_utils::fs::is_regular_file(full_path);
    } catch (...) {
        is_file_ok = false;
    }
    if (is_file_ok && file_utils::is_subpath_of(base_dir, full_path))
        file_descriptor = open(full_path.c_str(), O_RDONLY);

    return file_descriptor;
}

Response Server::get_file_response(const Request &request, const remote::rservers_t &remote_resources) {
    int file_descriptor;
    Response response;
    if (!Request::check_req_target(request.get_request_target()))
        return Response::create_404_response();

    file_utils::fs::path full_path = file_utils::fs::path(base_directory + request.get_request_target());
    file_descriptor = get_descriptor(full_path);
    if (file_descriptor != -1) {
        size_t file_size = file_utils::fs::file_size(full_path);

        response = Response(200, http::ALL_OK);
        response.add_header(http::HEADER_CONTENT_TYPE, http::INPUT_STREAM_TYPE);
        response.add_header(http::HEADER_CONTENT_LENGTH, std::to_string(file_size));

        std::cout << "Client resource found." << std::endl;
        if (request.get_method() == http::GET)
            response.set_file_descriptor(file_descriptor, file_size);
        else
            close(file_descriptor);
    } else {
        try {
            remote::server_t res = remote::get_resource(request.get_request_target(), remote_resources);
            std::cout << "Client resource found in remote servers." << std::endl;
            response = Response(302, http::REMOTE_FOUND, http::create_header("Location",
                                                                       http::PROT + res.first + http::COLON +
                                                                       std::to_string(res.second) +
                                                                       request.get_request_target()));
        } catch (const file_utils::NoDirException &n) {
            std::cerr << n.what() << std::endl;
            response = Response::create_404_response();
        }
    }
    return response;
}

Server::Server(const std::string &base_dir_arg, std::string server_path_arg, uint32_t port_num) :
        remote_servers_path(std::move(server_path_arg)) {

    try {
        this->base_directory = file_utils::canonize(base_dir_arg);
    } catch (const file_utils::NoDirException &noDirException) {
        exit_error("Problems with base directory!");
    }

    // after socket() call; we should CLOSE(sock) on any execution path;
    // since all execution paths exit immediately, sock would be closed when program terminates
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(port_num); // listening on port PORT_NUM

    create_socket();
    bind_socket();
    switch_to_listen();
}

void Server::run() {
    remote::rservers_t remote_resources = remote::parse_remote_resources(remote_servers_path);
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    signal(SIGPIPE, SIG_IGN);
    for (;;) {
        std::cout << "Accepting new clients!" << std::endl;
        int msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
        std::cout << "Connected to new client: " << msg_sock << std::endl;

        __gnu_cxx::stdio_filebuf<char> filebuf(msg_sock, std::ios::in);
        std::istream input_stream(&filebuf);

        InputReader inputReader;

        bool is_connection = true;
        while (is_connection) {
            Request request;
            Response response;
            is_connection = inputReader.read_request(request, input_stream);
            std::cout << "Client request read!" << std::endl;
            if (!is_connection) {
                response = inputReader.get_error_response();
                std::cout << "Client request wasn't valid! " << response.get_status_code() << std::endl;
            } else {
                std::cout << "Searching for client resource!" << std::endl;
                response = get_file_response(request, remote_resources);
                if (request.is_field_value_set(http::HEADER_CONNECTION) &&
                    request.get_field_value(http::HEADER_CONNECTION) == http::CLOSE) {
                    response.add_header(http::HEADER_CONNECTION, http::CLOSE);
                    is_connection = false;
                }
            }
            std::cout << "Sending response!" << std::endl;
            if (!response.send(msg_sock)) {
                std::cerr << "Client most likely disconnected." << std::endl;
                is_connection = false;
            }

            if (!is_connection) {
                std::cout << "Closing client connection!" << std::endl;
                if (close(msg_sock) < 0)
                    exit_error("Error closing connection!");
            }
        }
    }
}

void exit_error(const std::string &message) {
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}
