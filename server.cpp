// Сервер
// Делает следующее:
// 1) Слушает порт 5000 по TCP.
// 2) Принимает одного клиента за раз.
// 3) Для каждого сообщения клиента:
//    - если "ping" -> отправляет "pong"
//    - если "/history" -> отправляет историю из файла
//    - иначе -> просто эхо "echo: <сообщение>"
// 4) Все сообщения записывает в history.txt (клиент и сервер).

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

const int PORT = 5000;
const int BACKLOG = 5;
const int BUF_SIZE = 1024;
const char HISTORY_FILE[] = "history.txt";

// Функция для записи строки в файл истории.
void append_history(const string& prefix, const string& msg) {
    ofstream out(HISTORY_FILE, ios::app);
    if (!out.is_open()) {
        cerr << "Server: не удалось открыть history.txt для записи\n";
        return;
    }
    out << prefix << msg << '\n';
}

// Чтение всей истории из файла в одну строку.
string read_history() {
    ifstream in(HISTORY_FILE);
    if (!in.is_open()) {
        return "[no history yet]\n";
    }

    string result;
    string line;
    while (getline(in, line)) {
        result += line + '\n';
    }

    if (result.empty()) {
        result = "[history is empty]\n";
    }

    return result;
}

// Обрезаем \n и \r в конце строки (после конверсии из буфера).
void trim_newline(string& s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
}

int main() {
    // 1. Создаём слушающий сокет
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    // Разрешаем быстро пересоздавать сервер на том же порту
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }

    // 2. Заполняем структуру с адресом сервера
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 3. Привязка к порту
    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    // 4. Переводим сокет в режим прослушивания
    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    cout << "Server: listening on port " << PORT << "...\n";

    // Основной цикл сервера: принимаем клиентов один за другим
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // 5. accept блокируется, пока не придёт клиент
        int client_fd = accept(listen_fd,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &client_len);
        if (client_fd == -1) {
            perror("accept");
            // не выходим, а продолжаем ждать других клиентов
            continue;
        }

        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));
        cout << "Server: client connected from " << ipbuf
             << ":" << ntohs(client_addr.sin_port) << "\n";

        // 6. Цикл общения с одним клиентом
        char buffer[BUF_SIZE];

        while (true) {
            // читаем данные от клиента
            ssize_t received = recv(client_fd, buffer, BUF_SIZE - 1, 0);
            if (received == -1) {
                perror("recv");
                break; // ошибка, выходим из цикла для этого клиента
            }
            if (received == 0) {
                // клиент закрыл соединение
                cout << "Server: client disconnected\n";
                break;
            }

            buffer[received] = '\0'; // делаем C-строку
            string msg = buffer;
            trim_newline(msg);

            cout << "Server: received: [" << msg << "]\n";
            append_history("CLIENT: ", msg);

            // Обрабатываем команды
            if (msg == "ping") {
                const char* reply = "pong\n";
                if (send(client_fd, reply, strlen(reply), 0) == -1) {
                    perror("send");
                    break;
                }
                append_history("SERVER: ", "pong");
                cout << "Server: sent pong\n";
            } else if (msg == "/history") {
                string hist = read_history();
                if (send(client_fd, hist.c_str(), hist.size(), 0) == -1) {
                    perror("send history");
                    break;
                }
                append_history("SERVER: ", "[history sent]");
                cout << "Server: sent history\n";
            } else {
                // обычный чат: отправляем обратно то, что пришло
                string reply = "echo: " + msg + "\n";
                if (send(client_fd, reply.c_str(), reply.size(), 0) == -1) {
                    perror("send echo");
                    break;
                }
                append_history("SERVER: ", "echo: " + msg);
                cout << "Server: echoed message\n";
            }
        }

        close(client_fd);
        // переходим к ожиданию следующего клиента
    }

    close(listen_fd);
    cout << "Server: shutdown\n";
    return 0;
}
