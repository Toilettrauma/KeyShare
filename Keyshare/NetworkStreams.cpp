#include "NetworkStreams.hpp"
#include <array>
#include <cassert>

#ifndef FD_COPY
#define FD_COPY(dest,src) memcpy((dest),(src),sizeof *(dest))
#endif


WSADATA wsa_data = {};

// This struct used internally in NetworkStream ancestors
struct NetworkStreamResponse {
    enum Code {
        Unknown = 0,
        Heartbeat = 1
    };
    static constexpr int MAGIC = 0xFFAAFFAA;
    const int magic = 0xFFAAFFAA;
    Code code = Code::Unknown;

    constexpr bool is_valid() const noexcept {
        return magic == MAGIC;
    }
};

//std::istream& operator>>(std::istream& is, NetworkStreamResponse& response) {
//    int magic = is.peek();
//    if (magic != NetworkStreamResponse::MAGIC) {
//        // Maybe throw error. Now just make it invalid
//        response.code = NetworkStreamResponse::Code::Unknown;
//        return is;
//    }
//    // unsafe
//    is.read(reinterpret_cast<char*>(&response), sizeof(response));
//    return is;
//}

std::ostream& operator<<(std::ostream& os, const NetworkStreamResponse& response) {
    assert(response.is_valid());
    // unsafe
    os.write(reinterpret_cast<const char*>(&response), sizeof(response));
    return os;
}

NetworkErrorCode NetworkStreamBuf::init() {
    if(!WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
        return NetworkErrorCode::InitFailed;
    }
    return NetworkErrorCode::Success;
}

NetworkStreamBuf::NetworkStreamBuf(SOCKET socket, std::streamsize buffer_size) :
    buffer_size(buffer_size),
    _out_buffer(buffer_size, 0),
    _in_buffer(buffer_size, 0),
    _socket(socket),
    _last_overflow_char(invalid_char),
    _available_in_bytes(0)
{
    FD_ZERO(&_read_set);
    FD_SET(_socket, &_read_set);

    setp(_out_buffer.data(), _out_buffer.data() + _out_buffer.size());
    setg(_in_buffer.data(), _in_buffer.data(), _in_buffer.data() + _available_in_bytes);
}

NetworkStreamBuf::~NetworkStreamBuf() noexcept {
    closesocket(_socket);
}

int NetworkStreamBuf::overflow(int c) {
    if (c != traits_type::eof()) {
        _last_overflow_char = c;
        return sync() == 0 ? c : traits_type::eof();
    }
    return traits_type::eof();
}

int NetworkStreamBuf::underflow() {
    // not working
    if (gptr() != egptr()) {
        return traits_type::eof();
    }
    if (recv_available() != 0) {
        return traits_type::eof();
    }
    return traits_type::not_eof(traits_type::to_int_type(*gptr()));
}

int NetworkStreamBuf::sync() {
    return send_available();
}

int NetworkStreamBuf::send_available() {
    if (pptr() == pbase()) {
        return 0;
    }

    ptrdiff_t size = pptr() - pbase(), written = 0;
    while (written < size) {
        int now_written = send(_socket, pbase() + written, size - written, 0);
        if (now_written < 0) {
            return -1; // or exception?
        }
        written += now_written;
    }
    if (_last_overflow_char != invalid_char) {
        int now_written = send(_socket, reinterpret_cast<char*>(&_last_overflow_char), 1, 0);
        _last_overflow_char = invalid_char;
        if (now_written < 0) {
            return -1; // or exception?
        }
    }

    setp(pbase(), epptr());
}

int NetworkStreamBuf::recv_available() {
    int now_readed = recv(_socket, _in_buffer.data(), _in_buffer.size(), 0);
    if (now_readed <= 0) {
        return -1;
    }

    _available_in_bytes = now_readed;
    setg(_in_buffer.data(), _in_buffer.data(), _in_buffer.data() + now_readed);

    return 0;
}

NetworkStream::NetworkStream() :
    _socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
    _address(),
    _stream_buf(_socket, 256),
    std::iostream(&_stream_buf)
{
}

NetworkStream::NetworkStream(const SOCKET& socket, const sockaddr& address) :
    _socket(socket),
    _address(),
    _stream_buf(_socket, 256),
    std::iostream(&_stream_buf)
{
}

sockaddr NetworkStream::get_address() const {
    return _address;
}

NetworkClientStream::NetworkClientStream(std::string_view ip, std::string_view port) {
    addrinfo* result = nullptr, hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(ip.data(), port.data(), &hints, &result) != 0 || result == nullptr) {
        throw std::runtime_error("Failed to getaddrinfo(...)");
    }


    if (connect(_socket, result->ai_addr, result->ai_addrlen) != 0) {
        throw std::runtime_error("Failed to connect(...)");
    }

    // TODO: change
    _address = *result->ai_addr;

    freeaddrinfo(result);
}

void NetworkClientStream::wait_for_data() {
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(_socket, &read_set);
    if (select(0, &read_set, nullptr, nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to select(...)");
    }
}

NetworkServerStreamBuf::NetworkServerStreamBuf(const std::vector<std::shared_ptr<NetworkStream>>& streams) : _streams(streams) {
}

int NetworkServerStreamBuf::overflow(int c) {
    for (auto& stream : _streams) {
        stream->put(c);
    }
    return c;
}

int NetworkServerStreamBuf::sync() {
    for (auto& stream : _streams) {
        stream->sync();
    }
    return 0;
}

NetworkServerStream::NetworkServerStream(std::string_view ip, std::string_view port) :
    _socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
    _clients(),
    _stream_buf(_clients),
    std::ostream(&_stream_buf)
{
    addrinfo* result = nullptr, hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(ip.data(), port.data(), &hints, &result) != 0 || result == nullptr) {
        throw std::runtime_error("Failed to getaddrinfo(...)");
    }

    if (bind(_socket, result->ai_addr, result->ai_addrlen) != 0) {
        throw std::runtime_error("Failed to bind(...)");
    }
    if (listen(_socket, 100) != 0) {
        throw std::runtime_error("Failed to bind(...)");
    }

    // TODO: change
    _address = *result->ai_addr;

    freeaddrinfo(result);

    FD_ZERO(&_socket_read_set);
    FD_ZERO(&_socket_read_all_set);
    FD_ZERO(&_socket_write_set);
    FD_ZERO(&_socket_write_all_set);
    FD_SET(_socket, &_socket_read_all_set);
}

sockaddr NetworkServerStream::get_address() const {
    return _address;
}

void NetworkServerStream::check_sockets(const std::optional<std::chrono::system_clock::duration>& timeout) {
    using namespace std::chrono;
    FD_COPY(&_socket_read_set, &_socket_read_all_set);
    FD_COPY(&_socket_write_set, &_socket_write_all_set);

    if (!timeout.has_value()) {
        if (select(0, &_socket_read_set, &_socket_write_set, nullptr, nullptr) < 0) {
            throw std::runtime_error("Failed to select(...)");
        }
        return;
    }
    seconds timeout_secs = duration_cast<seconds>(timeout.value());
    microseconds timeout_usecs = duration_cast<microseconds>(timeout.value() - timeout_secs);

    timeval timeout_val;
    timeout_val.tv_sec = timeout_secs.count();
    timeout_val.tv_usec = timeout_usecs.count();
    if (select(0, &_socket_read_set, &_socket_write_set, nullptr, &timeout_val) < 0) {
        throw std::runtime_error("Failed to select(...)");
    }
}

bool NetworkServerStream::has_incoming_connection(const std::optional<std::chrono::system_clock::duration>& timeout) {
    check_sockets(timeout);
    return FD_ISSET(_socket, &_socket_read_set);
}

bool NetworkServerStream::has_incoming_data(const std::optional<std::chrono::system_clock::duration>& timeout) {
    check_sockets(timeout);
    return _socket_read_set.fd_count > 0 || _socket_write_set.fd_count > 0;
}

void NetworkServerStream::accept_all() {
    while (FD_ISSET(_socket, &_socket_read_set)) {
        sockaddr address = { 0 };
        int address_length = sizeof(sockaddr);
        SOCKET client_socket = accept(_socket, &address, &address_length);
        if (client_socket == INVALID_SOCKET) {
            return;
        }

        std::shared_ptr<NetworkStream>& client = _clients.emplace_back(std::make_shared<NetworkStream>(client_socket, address));
        FD_SET(client->_socket, &_socket_read_all_set);
        FD_SET(client->_socket, &_socket_write_all_set);
        FD_CLR(_socket, &_socket_read_set);

        check_sockets(std::chrono::seconds(0));
    }
}

void NetworkServerStream::accept_all_if_needed() {
    if (has_incoming_connection(std::chrono::seconds(0))) {
        accept_all();
    }
}

void NetworkServerStream::accept_all_and_wait_data() {
    while (!has_incoming_data(std::nullopt) && !has_incoming_connection(std::chrono::seconds(0))) {
        if (has_incoming_connection(std::chrono::seconds(0))) {
            accept_all();
        }
    }
}

std::shared_ptr<NetworkStream> NetworkServerStream::get_first_client_for_read() {
    if (_socket_read_set.fd_count <= 0) {
        return nullptr;
    }
    size_t i = 0;
    while (i < _clients.size()) {
        std::shared_ptr<NetworkStream>& client = _clients[i];
        if (client->eof()) {
            // disconnected
            FD_CLR(client->_socket, &_socket_read_all_set);
            FD_CLR(client->_socket, &_socket_write_all_set);
            _clients.erase(_clients.begin() + i);
            continue;
        }
        if (FD_ISSET(client->_socket, &_socket_read_set)) {
            FD_CLR(client->_socket, &_socket_read_set);
            return client;
        }
        ++i;
    }
    return nullptr;
}

std::shared_ptr<NetworkStream> NetworkServerStream::get_first_client_for_write() {
    if (_socket_write_set.fd_count <= 0) {
        return nullptr;
    }
    for (const std::shared_ptr<NetworkStream>& client : _clients) {
        if (FD_ISSET(client->_socket, &_socket_write_set)) {
            FD_CLR(client->_socket, &_socket_write_set);
            return client;
        }
    }
    return nullptr;
}
