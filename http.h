//
// Created by mateusz on 24.04.2021.
//

#include <iostream>
#include <vector>
#include <regex>
#include <sys/types.h>
#include <sys/socket.h>
#include <set>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

#ifndef ZADANIE_1_HTTP_H
#define ZADANIE_1_HTTP_H

namespace http {
    using vs_t = std::vector<std::string>;
    inline const std::string GET = "GET";
    inline const std::string HEAD = "HEAD";
    inline const std::string SP = " ";
    inline const std::string COLON = ":";
    inline const std::string PROT = "http://";
    inline const std::string CLOSE = "close";
    inline const std::string HTTP_VERSION = "HTTP/1.1";
    inline const std::string CRLF = "\r\n";

    inline const std::string HEADER_CONNECTION = "CONNECTION";
    inline const std::string HEADER_CONTENT_TYPE = "CONTENT-TYPE";
    inline const std::string HEADER_CONTENT_LENGTH = "CONTENT-LENGTH";
    inline const std::string HEADER_SERVER = "SERVER";

    inline const std::string ALL_OK = "All OK!";
    inline const std::string INPUT_STREAM_TYPE = "application/octet-stream";
    inline const std::string ERROR_400 = "Error 400 occured, Client did something wrong!";
    inline const std::string NOT_FOUND = "Resource not found!";
    inline const std::string REMOTE_FOUND = "Resource found elsewhere!";
    inline const std::string INVALID_METHOD = "Invalid method!";

    // Thrown when client's request has invalid format.
    // Server should response with error 400 when this has been thrown.
    class ParseException : public std::exception {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Parse exception occurred!";
        }
    };

    // Thrown when some header has field name that has not been recognised.
    // When called during parsing message, should be ignored.
    class WrongHeaderException : public std::exception {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Wrong header!";
        }
    };

    // Exception called client request has invalid method.
    class InvalidMethodException : public std::exception {
    public:
        [[nodiscard]] const char *what() const noexcept override {
            return "Invalid method!";
        }
    };

    // Creates and returns string representing header with appropriate field name and field value.
    std::string create_header(const std::string &field_name, const std::string &field_value);
}

class Response {
    int status;
    std::string reason_phrase, headers;
    bool is_sending_file;
    int file_descriptor;
    size_t file_size;
public:
    Response() = default;

    Response(int status, std::string reason_phrase, std::string headers = "") : status(status),
                                                                                reason_phrase(std::move(reason_phrase)),
                                                                                headers(std::move(headers)),
                                                                                is_sending_file(false) {}
    // Creates header with appropriate field name
    // and field value and appends it to "headers".
    void add_header(const std::string &field_name, const std::string &field_value);

    // Calling this function with file_descriptor makes it send file using this descriptor
    // when 'send' is called.
    void set_file_descriptor(int file_descriptor, size_t file_size);

    // Sends Response to client.
    // Sends the start line and headers to msg_sock.
    // If file descriptor was set, sends to client content of the file and closes it.
    // Returns 'true' if Response was sent successfully, 'false' otherwise.
    bool send(int msg_sock);

    [[nodiscard]] int get_status_code() const {
        return status;
    }

    static const Response &create_404_response() {
        const static Response response(404, http::NOT_FOUND);
        return response;
    }

    static const Response &create_400_response() {
        const static Response response(400, http::ERROR_400, http::create_header(http::HEADER_CONNECTION, http::CLOSE));
        return response;
    }

    static const Response &create_501_response() {
        const static Response response(501, http::INVALID_METHOD, http::create_header(http::HEADER_CONNECTION, http::CLOSE));
        return response;
    }
};

class Request {
    std::string method, request_target;
    std::map<std::string, std::string> headers; // Maps field names to its field values.
    // Possible field names in client request.
    const std::set<std::string> possible_headers = {http::HEADER_CONTENT_LENGTH, http::HEADER_CONNECTION};

    // Checks if field value (header[1]) of header is correct value of header
    // with field name equal to header[0].
    // Returns 'true' if header is correct, 'false' otherwise.
    static bool check_header(const http::vs_t &header);

    // Parses request line. Throws ParseException if parsing was unsuccessful.
    // Expects the line to be without the last, '\n' sing.
    // When parsing was successful, returns structure with it's first element
    // equal to method and second element equal to request target.
    static http::vs_t parse_request_line(const std::string &raw_msg);

    // Parses request header. Throws ParseException if parsing was unsuccessful.
    // Expects the line to be without the last, '\n' sing.
    // When parsing was successful, returns structure with it's first element
    // equal to field name, and second equal to field value.
    static http::vs_t parse_header_field(const std::string &header);

public:
    Request() = default;

    // Parses starting line of the request.
    // Expects the line to be without the last, '\n' character.
    // Throws ParseException if line doesn't have expected request line format
    // and InvalidMethodException if format is correct but method
    // is different from GET and HEAD. When there were no errors
    // updates fields "method" and "request_target".
    void parse_and_add_req_line(const std::string &line);

    // Parses request header line.
    // Expects the line to be without the last, '\n' character.
    // Throws ParseException if line doesn't have expected header line format
    // or the header with same field name was already parsed.
    // Throws WrongHeaderException if header is not recognized.
    // When parsing was successful, updates 'headers' map.
    void parse_and_add_header(const std::string &line);

    // Returns field value corresponding to field name in some of the request headers.
    // If request doesn't have field_name header, throws WrongHeaderException.
    [[nodiscard]] const std::string &get_field_value(const std::string &field_name) const {
        auto it = headers.find(field_name);
        if (it == headers.end()) {
            throw http::WrongHeaderException();
        } else {
            return it->second;
        }
    }

    [[nodiscard]] const std::string &get_method() const {
        return method;
    }

    // Checks if request target contains only valid characters.
    static bool check_req_target(const std::string &req_target) {
        static std::regex correct_path("(/[a-zA-Z0-9.-]*)+");
        return std::regex_match(req_target, correct_path);
    }

    [[nodiscard]] const std::string &get_request_target() const {
        return request_target;
    }
    // Returns 'true' if request has a header with
    // its field name equal to 'field_name' - the parameter.
    bool is_field_value_set(const std::string &field_name) {
        return headers.find(field_name) != headers.end();
    }
};

#endif //ZADANIE_1_HTTP_H
