#include <signal.h>

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <expected>
#include <string>
#include <print>
#include <unordered_map>

#include "meta.cpp"

using std::string, std::to_string, std::thread, std::expected, std::unexpected;

struct clientConnection;

auto bind_and_listen(SOCKET serverSocket, ConnectionConfig cfg) -> std::expected<Unit, FancyError>;

auto handle_sigint_cleanup(int sig) -> void;

auto handle_client(SOCKET clientSocket, int sessionID) -> void;


const int CONNECTION_QUEUE_SIZE = 5; // whatever this is for
const int CLIENT_MESSAGE_CHECK_DELAY_MS = 250;
const int CLIENT_WAIT_TIME_S = 180;

bool serverActive = true;

thread commands_thread;
std::unordered_map<int, clientConnection> clients;

std::unordered_map<int, string> clientIDtoName;

struct clientConnection {
    SOCKET socket;
    int clientID;
    std::thread thr;

    clientConnection(SOCKET s, int id, std::thread t): socket(s), clientID(id), thr(move(t)) {}
};


auto handle_client(SOCKET connectedSocket, int clientID) -> void {
    std::print("{}client with clientID {} connected!{}\n",
        ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);

    Message receivedMsg;
    int byteCount = 0;
    auto timeSinceLastMsg = std::chrono::steady_clock::now();

    set_socket_blocking(connectedSocket, false);
    
    while (connectedSocket != (unsigned long long)SOCKET_ERROR && serverActive) {
        byteCount = recv(connectedSocket, (char*)&receivedMsg, sizeof(Message), 0);

        if (byteCount > 0) {
            if (receivedMsg.type == MessageType::System && strcmp(receivedMsg.content, CLIENT_DISCONNECT) == 0) {
                connectedSocket = (unsigned long long)SOCKET_ERROR;
                
                break;
            }

            std::print("{}client {}: {}{}\n", 
                (receivedMsg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE), 
                (clientIDtoName.find(clientID) != clientIDtoName.end() ? clientIDtoName[clientID] : to_string(clientID)),
                ANSI_COLORS_DEFAULT, receivedMsg.content);

            if (receivedMsg.type == MessageType::User) {
                if (clientIDtoName.find(clientID) == clientIDtoName.end()) {
                    bool exists = false;
                    for (auto [id, name] : clientIDtoName) {
                        if (strcmp(receivedMsg.content, name.c_str()) == 0) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        clientIDtoName[clientID] = receivedMsg.content;
                    }

                    auto res = send_message(connectedSocket, Message(
                        MessageType::System,
                        (!exists ? NAME_ACCEPTED : NAME_REJECTED),
                        AUTHOR_SERVER
                    ));

                    if (!res) {
                        connectedSocket = SOCKET_ERROR; // todo do smth with error
                        break;
                    }
                } else {
                    strncpy(receivedMsg.author, clientIDtoName[clientID].c_str(), MAX_AUTHOR_LENGTH - 1);
                    for (auto& [lcID, lclient] : clients) {
                        if (lcID == clientID) continue;
                        auto res = send_message(lclient.socket, receivedMsg);
                    }
                }
            }
            
            timeSinceLastMsg = std::chrono::steady_clock::now();
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeSinceLastMsg)
             > std::chrono::seconds(CLIENT_WAIT_TIME_S)) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_MESSAGE_CHECK_DELAY_MS));
    }

    if (connectedSocket != (unsigned long long)SOCKET_ERROR) {
        set_socket_blocking(connectedSocket, false);

        expected<Unit, FancyError> discard = send_message(connectedSocket, Message(
            MessageType::System,
            SERVER_DISCONNECT,
            AUTHOR_SERVER
        ));

        print_dev("disconnected client {} manually, {}\n", clientID, (discard ? "successfuly" : "unsuccessfuly"));
    }

    clientIDtoName.erase(clientID);

    std::print("{}client with clientID {} disconnected{}\n",
        ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);
}

auto handle_server_commands() {
    string input;
    
    while (serverActive) {
        getline(std::cin, input);

        if (input == ":close" || input == ":c") {
            serverActive = false;
            std::print("{}shutting down & joining threads...{}\n",
                ANSI_COLORS_CYAN, ANSI_COLORS_DEFAULT);

            break;
        } else if (input.find(":broadcast") == 0) {
            if (input.length() > 12) {
                Message msg(MessageType::System, input.substr(11, 200).c_str(), AUTHOR_SERVER);

                for (auto& [cID, client] : clients) {
                    auto res = send_message(client.socket, msg);
                }
            }
        } else {
            std::print("{}unknown command{}\n", ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
        }
    }
}

int main() {
    ConnectionConfig connectionCfg = get_ip_port();

    SOCKET serverSocket;
    {
        expected<SOCKET, FancyError> res = init_wsa_and_get_socket();
        if (res) {
            serverSocket = res.value();
        } else {
            std::print("{}\n", res.error().text);
            return 1;
        }
    }

    {
        expected<Unit, FancyError> res = bind_and_listen(serverSocket, connectionCfg);
        if (!res) {
            std::print("{}\n", res.error().text);
            return 1;
        }
    }

    std::print("{}== server started =={}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    std::print("{}commands: :close, :broadcast [message]{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

    commands_thread = thread(handle_server_commands);

    set_socket_blocking(serverSocket, false);

    signal(SIGINT, handle_sigint_cleanup);

    SOCKET acceptSocket;

    unsigned int leastUnusedClientId = 0;

    while (serverActive) {
        acceptSocket = accept(serverSocket, NULL, NULL);

        if (acceptSocket != (unsigned long long)SOCKET_ERROR) {
            clients.emplace(leastUnusedClientId, clientConnection(
                acceptSocket, 
                leastUnusedClientId, 
                thread(handle_client, acceptSocket, clients.size())
            ));

            leastUnusedClientId++;
        }
    }
    
    /* if (acceptSocket == INVALID_SOCKET && serverActive) {
        cout << ANSI_COLORS_RED << "accept failed: " << WSAGetLastError() << ANSI_COLORS_DEFAULT << endl;
    } */

    commands_thread.join();

    for (auto& [cID, client] : clients) {
        client.thr.join();
    }

    std::print("{}closing socket & server{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    closesocket(serverSocket);
    WSACleanup();
}



// todo: remove
auto handle_sigint_cleanup(int sig) -> void {
    std::print("{}ctrl-c :({}\n",
        ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
    serverActive = false;
}

auto bind_and_listen(SOCKET serverSocket, ConnectionConfig cfg) -> expected<Unit, FancyError> {
    sockaddr_in service;
    service.sin_family = AF_INET;
    inet_pton(AF_INET, cfg.ip, &service.sin_addr.s_addr);
    service.sin_port = htons(cfg.port);
    if (bind(serverSocket, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR) {
        int err = get_last_error();
        closesocket(serverSocket);
        WSACleanup();
        return unexpected(FancyError(
            string(ANSI_COLORS_RED) + "bind failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err
        ));
    } else {
        print_dev("bind is ok!\n");
    }

    if (listen(serverSocket, CONNECTION_QUEUE_SIZE) == SOCKET_ERROR) {
        int err = get_last_error();
        WSACleanup();
        return unexpected(FancyError(
            string(ANSI_COLORS_RED) + "listen failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err
        ));
    } else {
        std::print("listening with big rabit ears\n");
    }

    return Unit();
}