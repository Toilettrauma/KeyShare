#pragma once
#include "NetworkStreams.hpp"
#include "KeyExchangerMessage.hpp"

class KeyExchanger {
public:
	virtual ~KeyExchanger() = default;

	virtual void wait() = 0;
	virtual KeyExchangeMessage get_message() = 0;
	virtual void send_message(KeyExchangeMessage& message) = 0;

private:

};

class KeyExchangerServer : public NetworkServerStream, public KeyExchanger {
public:
	KeyExchangerServer(std::string_view ip, std::string_view port);
	virtual ~KeyExchangerServer() override = default;

	virtual void wait() override;
	virtual KeyExchangeMessage get_message() override;
	virtual void send_message(KeyExchangeMessage& message) override;

private:

};

class KeyExchangerClient : public NetworkClientStream, public KeyExchanger {
public:
	KeyExchangerClient(std::string_view ip, std::string_view port);
	virtual ~KeyExchangerClient() override = default;

	virtual void wait() override;
	virtual KeyExchangeMessage get_message() override;
	virtual void send_message(KeyExchangeMessage& message) override;

private:



};

