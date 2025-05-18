#include "KeyExchanger.hpp"

std::istream& operator>>(std::istream& is, KeyboardMessage& msg) {
	is.read(reinterpret_cast<char*>(&msg), sizeof(msg));
	return is;
}
std::istream& operator>>(std::istream& is, MouseMessage& msg) {
	is.read(reinterpret_cast<char*>(&msg), sizeof(msg));
	return is;
}
std::istream& operator>>(std::istream& is, KeyboardMessage&& msg) {
	return is >> msg;
}
std::istream& operator>>(std::istream& is, MouseMessage&& msg) {
	return is >> msg;
}

std::ostream& operator<<(std::ostream& os, KeyboardMessage& msg) {
	os.write(reinterpret_cast<char*>(&msg), sizeof(msg));
	return os;
}
std::ostream& operator<<(std::ostream& os, MouseMessage& msg) {
	os.write(reinterpret_cast<char*>(&msg), sizeof(msg));
	return os;
}
std::ostream& operator<<(std::ostream& os, KeyboardMessage&& msg) {
	return os << msg;
}
std::ostream& operator<<(std::ostream& os, MouseMessage&& msg) {
	return os << msg;
}

// TODO: move all responsibility to KeyExchangeMessage
void try_send_message_to(std::ostream& os, const KeyExchangeMessage& message) {
	os.write(reinterpret_cast<const char*>(&message.magic), sizeof(message.magic));

	KeyExchangeMessage::EventType event_type = message.get_event_type();
	os << static_cast<int>(event_type);
	if (event_type == KeyExchangeMessage::EventType::Keyboard) {
		os << message.get_keyboard_message();
	}
	else if (event_type == KeyExchangeMessage::EventType::Mouse) {
		os << message.get_mouse_message();
	}
	os << std::flush;
}

// TODO: move all responsibility to KeyExchangeMessage
std::optional<KeyExchangeMessage> try_read_message_from(std::istream& is) {
	int magic = 0;
	is.read(reinterpret_cast<char*>(&magic), sizeof(magic));
	if (magic != KeyExchangeMessage::magic) {
		return std::nullopt;
	}

	KeyExchangeMessage::EventType event_type;
	// unsafe
	is >> reinterpret_cast<int&>(event_type);
	if (event_type == KeyExchangeMessage::EventType::Keyboard) {
		KeyboardMessage keyboard_msg;
		is >> keyboard_msg;
		return KeyExchangeMessage(keyboard_msg);
	}
	else if (event_type == KeyExchangeMessage::EventType::Mouse) {
		MouseMessage mouse_msg;
		is >> mouse_msg;
		return KeyExchangeMessage(mouse_msg);
	}
	return KeyExchangeMessage();
}

KeyExchangerServer::KeyExchangerServer(std::string_view ip, std::string_view port) : NetworkServerStream(ip, port) {

}

void KeyExchangerServer::wait() {
	NetworkServerStream::accept_all_and_wait_data();
}

KeyExchangeMessage KeyExchangerServer::get_message() {
	accept_all_if_needed();
	bool done = false;
	while (!done) {
		std::shared_ptr<NetworkStream> client = get_first_client_for_read();
		while (!client) {
			wait();
			client = get_first_client_for_read();
		}

		std::optional<KeyExchangeMessage> msg = try_read_message_from(*client);
		if (msg.has_value()) {
			done = true;
			return msg.value();
		}
	}
}

void KeyExchangerServer::send_message(KeyExchangeMessage& message) {
	accept_all_if_needed();
	return try_send_message_to(*this, message);
}

KeyExchangerClient::KeyExchangerClient(std::string_view ip, std::string_view port) : NetworkClientStream(ip, port) {

}

void KeyExchangerClient::wait() {
	NetworkClientStream::wait_for_data();
}

KeyExchangeMessage KeyExchangerClient::get_message() {
	bool done = false;
	while (!done) {
		std::optional<KeyExchangeMessage> msg = try_read_message_from(*this);
		if (msg.has_value()) {
			done = true;
			return msg.value();
		}
	}
}

void KeyExchangerClient::send_message(KeyExchangeMessage& message) {
	return try_send_message_to(*this, message);
}
