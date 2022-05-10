//
// Created by mateusz on 24.04.2021.
//

#include "http.h"

std::string http::create_header(const std::string &field_name, const std::string &field_value) {
    return field_name + ":" + field_value + http::SP + http::CRLF;
}

void Response::add_header(const std::string &field_name, const std::string &field_value) {
    headers.append(field_name + http::COLON + field_value + http::SP + http::CRLF);
}

void Response::set_file_descriptor(int file_descriptor, size_t file_size) {
    this->file_descriptor = file_descriptor;
    this->file_size = file_size;
    is_sending_file = true;
}

bool Response::send(int msg_sock) {
    std::string response_start =
            http::HTTP_VERSION + http::SP + std::to_string(status) + http::SP + reason_phrase + http::CRLF + headers + http::CRLF;
    if (write(msg_sock, response_start.c_str(), response_start.size()) != (ssize_t) response_start.size()) {
        std::cout << "Error sending response!" << std::endl;
        if (is_sending_file)
            close(file_descriptor);
        return false;
    }
    bool ret = true;
    if (is_sending_file) {
        size_t remaining = file_size;
        off_t offset = 0;
        std::cout << "Sending file, file size = " << file_size << std::endl;
        while (remaining > 0) {
            ssize_t sent_bytes = sendfile(msg_sock, file_descriptor, (off_t *) &offset, remaining);
            if (sent_bytes > 0) {
                remaining -= sent_bytes;
            } else if (sent_bytes == -1) {
                std::cout << "Error sending file!" << std::endl;
                ret = false;
                break;
            }
        }
        close(file_descriptor);
    }
    return ret;
}

bool Request::check_header(const http::vs_t &header) {
    if (header[0] == http::HEADER_CONTENT_LENGTH) {
        return header[1] == "0";
    } else if (header[0] == http::HEADER_CONNECTION) {
        return header[1] == http::CLOSE;
    }
    return true;
}

void Request::parse_and_add_req_line(const std::string &line) {
    http::vs_t parsed_request_line = parse_request_line(line);
    method = parsed_request_line[0], request_target = parsed_request_line[1];
    if (method != http::GET && method != http::HEAD) {
        throw http::InvalidMethodException();
    }
}

void Request::parse_and_add_header(const std::string &line) {
    http::vs_t header = parse_header_field(line);
    std::transform(header[0].begin(), header[0].end(), header[0].begin(), ::toupper);
    if (possible_headers.find(header[0]) == possible_headers.end()) {
        throw http::WrongHeaderException();
    }
    if (headers.find(header[0]) != headers.end())
        throw http::ParseException();
    if (!check_header(header))
        throw http::ParseException();
    headers.insert({header[0], header[1]});
}

http::vs_t Request::parse_header_field(const std::string &header) {
    static const std::regex not_whitespace_and_colon("[^\\s:]+");
    static const std::regex header_regex("([a-zA-Z0-9-_]+)[:][ ]*([^ ]+)[ ]*\r");
    if (std::regex_match(header, header_regex)) {
        auto regex_it = std::sregex_iterator(header.begin(), header.end(), not_whitespace_and_colon);
        return {regex_it->str(), std::next(regex_it)->str()};
    } else {
        throw http::ParseException();
    }
}

http::vs_t Request::parse_request_line(const std::string &raw_msg) {
    static const std::regex not_whitespace("[^\\s]+");
    static const std::regex request_line_regex("([A-Za-z]+) (/[^ ]*) (HTTP/1[.]1)\r");

    if (std::regex_match(raw_msg, request_line_regex)) {
        auto regex_it = std::sregex_iterator(raw_msg.begin(), raw_msg.end(), not_whitespace);
        return {regex_it->str(), std::next(regex_it)->str()};
    } else {
        throw http::ParseException();
    }
}
