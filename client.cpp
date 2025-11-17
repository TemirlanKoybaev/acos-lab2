// Клиент
// 1) Подключается к 127.0.0.1:5000.
// 2) Читает строки с клавиатуры и отправляет на сервер.
// 3) Показывает ответ сервера.
// Поддерживаемые "команды":
//   ping      -> сервер отвечает "pong"
//   /history  -> сервер отправляет историю из файла
//   любой текст -> сервер отвечает "echo: <текст>"
//   quit      -> выход из клиента

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
    const int   PORT      = 5000;
    const char* SERVER_IP = "127.0.0.1";
    const int   BUF_SIZE  = 4096;

    // 1. Создаём сокет
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return 1;
    }

    // 2. Заполняем структуру с адресом сервера
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    // 3. Подключаемся к серверу
    if (connect(sock_fd,
                reinterpret_cast<sockaddr*>(&server_addr),
                sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    cout << "Client: connected to " << SERVER_IP << ":" << PORT << "\n\n";
    cout << "Commands:\n";
    cout << "  ping      -> server replies pong\n";
    cout << "  /history  -> server sends chat history\n";
    cout << "  any text  -> echo from server\n";
    cout << "  quit      -> exit client\n\n";

    char buffer[BUF_SIZE];

    // 4. Основной цикл: читаем ввод пользователя и общаемся с сервером
    while (true) {
        cout << "You: ";
        string input;
        if (!getline(cin, input)) {
            // EOF или ошибка ввода
            break;
        }

        if (input == "quit") {
            break;
        }

        // Добавляем перевод строки, чтобы серверу было проще
        input += "\n";

        // Отправляем строку серверу
        ssize_t sent = send(sock_fd, input.c_str(), input.size(), 0);
        if (sent == -1) {
            perror("send");
            break;
        }

        // Ждём ответ (для простоты читаем один буфер)
        ssize_t received = recv(sock_fd, buffer, BUF_SIZE - 1, 0);
        if (received == -1) {
            perror("recv");
            break;
        }
        if (received == 0) {
            cout << "Client: server closed connection\n";
            break;
        }

        buffer[received] = '\0';
        cout << "Server: " << buffer;
    }

    close(sock_fd);
    cout << "Client: shutdown\n";
    return 0;
}
