// Keyshare.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "NetworkStreams.hpp"
#include "Logger.hpp"
#include "KeyExchanger.hpp"
#include "KeyEventHandler.hpp"

#include <iostream>
#include <array>
#include <vector>
#include <string>

constexpr std::string_view payload =
    "GET / HTTP/1.1\n"
    "Host: 10.244.235.0:7860\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:128.0) Gecko/20100101 Firefox/128.0\n"
    "Accept: */*\n"
    "Connection: keep-alive\n"
    "Accept-Encoding: utf-8\n"
    "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3\n\n";

// For some reason operator>> sets failbit on stream

void test1() {
    NetworkClientStream client_stream("10.244.235.0", "7860");

    int i = 0;
    std::string buf;
    client_stream << payload << std::flush;
    while (!client_stream.eof()) {
        std::getline(client_stream, buf);
        std::cout << buf << std::endl;
    }
}

int test2() {
    std::string buf;

    NetworkServerStream server("0.0.0.0", "9999");
    NetworkClientStream client("127.0.0.1", "9999");
    client << "Hello world!" << std::endl;

    server.accept_all_and_wait_data();
    std::shared_ptr<NetworkStream> stream = server.get_first_client_for_read();
    if (!stream) {
        return -1;
    }

    std::getline(*stream, buf);
    std::cout << "Client sent: " << buf << std::endl;
}

void test3() {
    KeyExchangerServer server("0.0.0.0", "9999");
    KeyExchangerClient client1("127.0.0.1", "9999");

    KeyboardMessage keyboard_message;
    keyboard_message.key_code = KeyboardKeyCode::W;
    keyboard_message.key_state = KeyboardKeyState::KeyDown;

    KeyExchangeMessage exchange_message(keyboard_message);
    {
        KeyExchangerClient client2("127.0.0.1", "9999");

        client1.send_message(exchange_message);

        exchange_message = server.get_message();
        server.send_message(exchange_message);

        exchange_message = client2.get_message();

        // ...
    }

    server.wait();
    client1.send_message(exchange_message);
    server.get_message();
    server.get_message(); // infinite block
}

static int program_main(int argc, char** argv) {
    int code = 0;

    KeyEventHandler::set_handler(
        [](KeyExchangeMessage message) {
            Logger::log_info(message.get_event_type() == KeyExchangeMessage::EventType::Keyboard ? "Keyboard message" : "Mouse message");
        }
    );
    KeyEventHandler::enable_keyboard_hook();
    //KeyEventHandler::enable_mouse_hook();

    while (KeyEventHandler::process());

    return code;
}

int main(int argc, char** argv) {
    NetworkStreamBuf::init();

    try {
        return program_main(argc, argv);
    }
    catch (const std::exception& e) {
        Logger::log_error(e.what());
        return -1;
    }
    catch (...) {
        Logger::log_error("Unknown error");
        return -1;
    }

    return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
