#include <iostream>
#include <expected>
#include <string>
#include <thread>
#include <print>

#include "meta.cpp"

using std::string, std::to_string, std::thread, std::expected, std::unexpected;

auto handle_server(SOCKET clientSocket) -> void;

auto connect_to_server(SOCKET clientSocket, ConnectionConfig cfg) -> std::expected<Unit, FancyError>;

auto client_cleanup(SOCKET& clientSocket, thread& receiveThread) -> void;


const int MESSAGE_CHECK_DELAY_MS = 250;

bool clientIsActive = true;

bool gotNamed = false;


auto handle_server(SOCKET connectedSocket) -> void {
    set_socket_blocking(connectedSocket, false);

    Message receivedMsg;
    int byteCount = 0;
    while (clientIsActive && connectedSocket != (unsigned long long)SOCKET_ERROR) {
        byteCount = recv(connectedSocket, (char*)&receivedMsg, sizeof(Message), 0);

        if (byteCount > 0) {
            if (receivedMsg.type == MessageType::System) {
                if (strcmp(receivedMsg.content, SERVER_DISCONNECT) == 0) {
                    std::print("{}server disconnected{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
                    clientIsActive = false;

                    print_dev("server killed itself\n");

                    break;
                } else if (strcmp(receivedMsg.content, NAME_ACCEPTED) == 0) {
                    std::print("{}your name was accepted!{}\n",
                    ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

                    gotNamed = true;

                    std::print("{}type your message, up to {} characters\nuse :dis to disconnect{}\n",
                        ANSI_COLORS_GREEN, MAX_MESSAGE_LENGTH, ANSI_COLORS_DEFAULT);
                } else if (strcmp(receivedMsg.content, NAME_REJECTED) == 0) {
                    std::print("{}your name was rejected, try another one{}\n",
                    ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
                } else {
                    std::print("{}{}:{} {}\n", 
                        (receivedMsg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE),
                        (receivedMsg.type == MessageType::System ? string("server") : (string("user ") + receivedMsg.author)),
                        ANSI_COLORS_DEFAULT,
                        receivedMsg.content);
                }
            } else {
                std::print("{}{}:{} {}\n", 
                    (receivedMsg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE),
                    (receivedMsg.type == MessageType::System ? string("server") : (string("user ") + receivedMsg.author)),
                    ANSI_COLORS_DEFAULT,
                    receivedMsg.content);
            }
        } /* else if (byteCount == SOCKET_ERROR) {
            int err = getlasterror();
            WSACleanup();
            std::print("{}\n", string(ANSI_COLORS_RED) + "failed to listen to server. this is fatal\n: " + to_string(err) + ANSI_COLORS_DEFAULT, err);
            clientIsActive = false;

            break;
        } */

        std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_CHECK_DELAY_MS));
    }

    print_dev("stopped listetning to server\nclient is active: {}, connection socket: {}\n", clientIsActive, connectedSocket);
}


int main() {
    ConnectionConfig connectionCfg = get_ip_port();

    SOCKET clientSocket;
    {
        expected<SOCKET, FancyError> res = init_wsa_and_get_socket();
        if (res) {
        clientSocket = res.value();
        } else {
            std::print("{}\n", res.error().text);
            return 1;
        }
    }

    {
        expected<Unit, FancyError> res = connect_to_server(clientSocket, connectionCfg);
        if (!res) {
            std::print("{}\n", res.error().text);
            clientSocket = SOCKET_ERROR;
        } else {
            /* send_message(clientSocket, Message(MessageTypes::System, CLIENT_CONNECT)); */
        }
    }

    thread receiveThread = thread(handle_server, clientSocket);
    
    char msg[MAX_MESSAGE_LENGTH];

    while (clientIsActive && clientSocket != (unsigned long long)SOCKET_ERROR) {
        if (!gotNamed) {
            std::print("{}give yourself a name, up to {} characters long{}\n",
                ANSI_COLORS_GREEN, MAX_AUTHOR_LENGTH - 1, ANSI_COLORS_DEFAULT);

            std::cin.getline(msg, MAX_AUTHOR_LENGTH);
        } else {
            std::cin.getline(msg, MAX_MESSAGE_LENGTH);
        }

        if (!strcmp(msg, ":disconnect") || !strcmp(msg, ":dis") || !clientIsActive) {
            break;
        }

        expected<Unit, FancyError> res = send_message(clientSocket, Message(MessageType::User, msg));
        if (!res) {
            WSACleanup();

            if (res.error().code == 10054) { // yupee arbitrary numbers
                std::print("{}error occured when sending the message. server has likely disconnected.{}\n",
                    ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
            } else {
                std::print("{}\n", res.error().text);
            }
            
            clientSocket = SOCKET_ERROR;
            break;
        }
    }

    client_cleanup(clientSocket, receiveThread);
}

auto connect_to_server(SOCKET clientSocket, ConnectionConfig cfg) -> expected<Unit, FancyError> {
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    inet_pton(AF_INET, cfg.ip, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(cfg.port);
    if (connect(clientSocket, (sockaddr*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        int err = get_last_error();
        WSACleanup();
        return unexpected(FancyError(string(ANSI_COLORS_RED) + "client connect failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err));
    } else {
        std::print("{}== connected =={}\n",
            ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    }

    return Unit();
}

auto client_cleanup(SOCKET& clientSocket, thread& receiveThread) -> void {

    clientIsActive = false;

    std::print("{}closing socket & client{}\n",
        ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

    receiveThread.join();

    if (clientSocket != (unsigned long long)SOCKET_ERROR) { // server will close connection if error occurs
        auto res = send_message(clientSocket, Message(MessageType::System, CLIENT_DISCONNECT));
        if (res) { // let me close the stupid client
            closesocket(clientSocket);
        }

        WSACleanup();
    }
}