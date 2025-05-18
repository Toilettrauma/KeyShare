#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <iostream>
#include <string_view>
#include <optional>
#include <chrono>
#include <memory>

enum class NetworkErrorCode {
	Success = 0,
	InitFailed = 1
};

class NetworkStreamBuf : public std::streambuf {
public:
	static NetworkErrorCode init();

	const std::streamsize buffer_size;

	NetworkStreamBuf(SOCKET socket, const std::streamsize buffer_size);
	virtual ~NetworkStreamBuf() noexcept override;

protected:
	virtual int overflow(int c) override;
	virtual int underflow() override;

	virtual int sync() override;

private:
	static constexpr int invalid_char = -1;

	int send_available();
	int recv_available();

	SOCKET _socket;
	std::vector<char> _out_buffer;
	std::vector<char> _in_buffer;
	int _last_overflow_char;
	std::streamsize _available_in_bytes;
	fd_set _read_set;
};

class NetworkStream : public std::iostream {
	friend class NetworkServerStream;
public:
	NetworkStream();
	NetworkStream(const SOCKET& socket, const sockaddr& address);

	sockaddr get_address() const;

protected:
	SOCKET _socket;
	NetworkStreamBuf _stream_buf;
	sockaddr _address;
};

class NetworkClientStream : public NetworkStream {
	
public:
	NetworkClientStream(std::string_view ip, std::string_view port);

	void wait_for_data();

private:

};

class NetworkServerStreamBuf : public std::streambuf {
public:

	NetworkServerStreamBuf(const std::vector<std::shared_ptr<NetworkStream>>& streams);
	virtual ~NetworkServerStreamBuf() noexcept override = default;

	virtual int overflow(int c) override;
	virtual int sync() override;

private:
	const std::vector<std::shared_ptr<NetworkStream>>& _streams;
};

class NetworkServerStream : public std::ostream {
public:
	NetworkServerStream(std::string_view ip, std::string_view port);

	sockaddr get_address() const;

	void check_sockets(const std::optional<std::chrono::system_clock::duration>& timeout);

	bool has_incoming_connection(const std::optional<std::chrono::system_clock::duration>& timeout);
	bool has_incoming_data(const std::optional<std::chrono::system_clock::duration>& timeout);

	void accept_all();
	void accept_all_if_needed();
	void accept_all_and_wait_data();

	std::shared_ptr<NetworkStream> get_first_client_for_read();
	std::shared_ptr<NetworkStream> get_first_client_for_write();

private:
	std::vector<std::shared_ptr<NetworkStream>> _clients;
	SOCKET _socket;
	sockaddr _address;
	NetworkServerStreamBuf _stream_buf;

	fd_set _socket_read_set;
	fd_set _socket_read_all_set;
	fd_set _socket_write_set;
	fd_set _socket_write_all_set;

};
