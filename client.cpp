#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <expected>
#include <string>
#include <thread>
#include <print>

#include "meta.cpp"

auto handle_server(SOCKET clientSocket) -> void;

auto connect_to_server(SOCKET clientSocket) -> std::expected<Unit, std::string>;

using namespace std;


const int MESSAGE_CHECK_DELAY_MS = 250;

bool client_active = true;


auto handle_server(SOCKET clientSocket) -> void {
    u_long socket_is_non_blocking = true;
    ioctlsocket(clientSocket, FIONBIO, &socket_is_non_blocking);

    Message received_msg;
    int byte_count = 0;
    while (client_active && clientSocket != (unsigned long long)SOCKET_ERROR) {
        byte_count = recv(clientSocket, (char*)&received_msg, sizeof(Message), 0);

        if (byte_count > 0) {
            if (received_msg.type == MessageType::System && strcmp(received_msg.content, SERVER_DISCONNECT) == 0) {
                print("{}server disconnected{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
                client_active = false;

                break;
            }

            print("{}{}:{} {}\n", 
                (received_msg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE),
                (received_msg.type == MessageType::System ? string("server") : (string("user ") + to_string(received_msg.author))),
                ANSI_COLORS_DEFAULT,
                received_msg.content);
        }

        this_thread::sleep_for(chrono::milliseconds(MESSAGE_CHECK_DELAY_MS));
    }
}


int main() {
    #ifndef DEV
    get_ip_port();
    #endif

    SOCKET clientSocket;
    {
        expected<SOCKET, string> res = init_wsa_and_get_socket();
        if (res) {
        clientSocket = res.value();
        } else {
            print("{}\n", res.error());
            return 1;
        }
    }

    {
        expected<Unit, string> res = connect_to_server(clientSocket);
        if (!res) {
            print("{}\n", res.error());
            clientSocket = SOCKET_ERROR;
        } else {
            /* send_message(clientSocket, Message(MessageTypes::System, CLIENT_CONNECT)); */

            print("{}type your message, up to {} characters\nuse :dis to disconnect{}\n",
                ANSI_COLORS_GREEN, MAX_MESSAGE_LENGTH, ANSI_COLORS_DEFAULT);
        }
    }

    thread receive_thread = thread(handle_server, clientSocket);
    
    char msg[MAX_MESSAGE_LENGTH];

    while (client_active && clientSocket != (unsigned long long)SOCKET_ERROR) {
        cin.getline(msg, MAX_MESSAGE_LENGTH);

        if (strcmp(msg, ":disconnect") == 0 || strcmp(msg, ":dis") == 0 || !client_active) {
            break;
        }

        expected<Unit, string> res = send_message(clientSocket, Message(MessageType::User, msg));
        if (!res) {
            WSACleanup();
            print("{}\n", res.error());
            clientSocket = SOCKET_ERROR;
            break;
        }
    }

    client_active = false;

    print("{}closing socket & client{}\n",
        ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

    receive_thread.join();

    if (clientSocket != (unsigned long long)SOCKET_ERROR) { // server will close connection if error occurs
        auto res = send_message(clientSocket, Message(MessageType::System, CLIENT_DISCONNECT));
        if (res) { // let me close the stupid client
            closesocket(clientSocket);
        }

        WSACleanup();
    }
}



auto connect_to_server(SOCKET clientSocket) -> expected<Unit, string> {
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        string err = to_string(WSAGetLastError());
        WSACleanup();
        return unexpected(string(ANSI_COLORS_RED) + "client connect failed: " + err + ANSI_COLORS_DEFAULT);
    } else {
        print("{}== connected =={}\n",
            ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    }

    return Unit();
}