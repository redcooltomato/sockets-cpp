#include <iostream>
#include <expected>
#include <string>
#include <thread>
#include <print>

#include "meta.cpp"

auto handle_server(SOCKET clientSocket) -> void;

auto connect_to_server(SOCKET clientSocket) -> std::expected<Unit, FancyError>;

using std::string, std::to_string, std::thread, std::expected, std::unexpected;


const int MESSAGE_CHECK_DELAY_MS = 250;

bool client_active = true;

bool got_named = false;


auto handle_server(SOCKET clientSocket) -> void {
    set_socket_blocking(clientSocket, false);

    Message received_msg;
    int byte_count = 0;
    while (client_active && clientSocket != (unsigned long long)SOCKET_ERROR) {
        byte_count = recv(clientSocket, (char*)&received_msg, sizeof(Message), 0);

        if (byte_count > 0) {
            if (received_msg.type == MessageType::System) {
                if (strcmp(received_msg.content, SERVER_DISCONNECT) == 0) {
                    std::print("{}server disconnected{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
                    client_active = false;

                    #ifdef DEV
                    std::print("server killed itself\n");
                    #endif

                    break;
                } else if (strcmp(received_msg.content, NAME_ACCEPTED) == 0) {
                    std::print("{}your name was accepted!{}\n",
                    ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

                    got_named = true;

                    std::print("{}type your message, up to {} characters\nuse :dis to disconnect{}\n",
                        ANSI_COLORS_GREEN, MAX_MESSAGE_LENGTH, ANSI_COLORS_DEFAULT);
                } else if (strcmp(received_msg.content, NAME_REJECTED) == 0) {
                    std::print("{}your name was rejected, try another one{}\n",
                    ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
                } else {
                    std::print("{}{}:{} {}\n", 
                        (received_msg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE),
                        (received_msg.type == MessageType::System ? string("server") : (string("user ") + received_msg.author)),
                        ANSI_COLORS_DEFAULT,
                        received_msg.content);
                }
            } else {
                std::print("{}{}:{} {}\n", 
                    (received_msg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE),
                    (received_msg.type == MessageType::System ? string("server") : (string("user ") + received_msg.author)),
                    ANSI_COLORS_DEFAULT,
                    received_msg.content);
            }
        } /* else if (byte_count == SOCKET_ERROR) {
            int err = getlasterror();
            WSACleanup();
            std::print("{}\n", string(ANSI_COLORS_RED) + "failed to listen to server. this is fatal\n: " + to_string(err) + ANSI_COLORS_DEFAULT, err);
            client_active = false;

            break;
        } */

        std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_CHECK_DELAY_MS));
    }

    #ifdef DEV
    std::print("stopped listetning to server\n");
    #endif
}


int main() {
    #ifndef DEV
    get_ip_port();
    #endif

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
        expected<Unit, FancyError> res = connect_to_server(clientSocket);
        if (!res) {
            std::print("{}\n", res.error().text);
            clientSocket = SOCKET_ERROR;
        } else {
            /* send_message(clientSocket, Message(MessageTypes::System, CLIENT_CONNECT)); */

            std::print("{}give yourself a name, up to {} characters long{}\n",
                ANSI_COLORS_GREEN, MAX_AUTHOR_LENGTH - 1, ANSI_COLORS_DEFAULT);
        }
    }

    thread receive_thread = thread(handle_server, clientSocket);
    
    char msg[MAX_MESSAGE_LENGTH];

    while (client_active && clientSocket != (unsigned long long)SOCKET_ERROR) {
        if (!got_named) {
            std::cin.getline(msg, MAX_AUTHOR_LENGTH);
        } else {
            std::cin.getline(msg, MAX_MESSAGE_LENGTH);
        }

        if (strcmp(msg, ":disconnect") == 0 || strcmp(msg, ":dis") == 0 || !client_active) {
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

    client_active = false;

    std::print("{}closing socket & client{}\n",
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



auto connect_to_server(SOCKET clientSocket) -> expected<Unit, FancyError> {
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (sockaddr*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        int err = getlasterror();
        WSACleanup();
        return unexpected(FancyError(string(ANSI_COLORS_RED) + "client connect failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err));
    } else {
        std::print("{}== connected =={}\n",
            ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    }

    return Unit();
}