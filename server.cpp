#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
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

struct clientConnection;

auto bind_and_listen(SOCKET serverSocket) -> std::expected<Unit, FancyError>;

auto handle_sigint_cleanup(int sig) -> void;

auto handle_client(SOCKET clientSocket, int sessionID) -> void;

using namespace std;


const int CONNECTION_QUEUE_SIZE = 5; // whatever this is for
const int CLIENT_MESSAGE_CHECK_DELAY_MS = 250;
const int CLIENT_WAIT_TIME_S = 180;

bool server_active = true;

thread commands_thread;
unordered_map<int, clientConnection> clients;
int lucid = 0; // least unused client id

unordered_map<int, string> clientIDtoName;

struct clientConnection {
    SOCKET socket;
    int clientID;
    std::thread thr;

    clientConnection(SOCKET s, int id, std::thread t): socket(s), clientID(id), thr(move(t)) {}
};


auto handle_client(SOCKET clientSocket, int clientID) -> void {
    print("{}client with clientID {} connected!{}\n",
        ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);

    Message received_msg;
    int byteCount = 0;
    auto time_since_last_msg = chrono::steady_clock::now();

    u_long socket_is_non_blocking = true;
    ioctlsocket(clientSocket, FIONBIO, &socket_is_non_blocking);
    
    while (clientSocket != (unsigned long long)SOCKET_ERROR && server_active) {
        byteCount = recv(clientSocket, (char*)&received_msg, sizeof(Message), 0);

        if (byteCount > 0) {
            if (received_msg.type == MessageType::System && strcmp(received_msg.content, CLIENT_DISCONNECT) == 0) {
                clientSocket = (unsigned long long)SOCKET_ERROR;
                
                break;
            }

            print("{}client {}: {}{}\n", 
                (received_msg.type == MessageType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE), 
                (clientIDtoName.find(clientID) != clientIDtoName.end() ? clientIDtoName[clientID] : to_string(clientID)),
                ANSI_COLORS_DEFAULT, received_msg.content);

            if (received_msg.type == MessageType::User) {
                if (clientIDtoName.find(clientID) == clientIDtoName.end()) {
                    bool exists = false;
                    for (auto [id, name] : clientIDtoName) {
                        if (strcmp(received_msg.content, name.c_str()) == 0) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        clientIDtoName[clientID] = received_msg.content;
                    }

                    auto res = send_message(clientSocket, Message(
                        MessageType::System,
                        (!exists ? NAME_ACCEPTED : NAME_REJECTED),
                        AUTHOR_SERVER
                    ));

                    if (!res) {
                        clientSocket = SOCKET_ERROR; // todo do smth with error
                        break;
                    }
                } else {
                    strncpy(received_msg.author, clientIDtoName[clientID].c_str(), MAX_AUTHOR_LENGTH - 1);
                    for (auto& [lcID, lclient] : clients) {
                        if (lcID == clientID) continue;
                        auto res = send_message(lclient.socket, received_msg);
                    }
                }
            }
            
            time_since_last_msg = chrono::steady_clock::now();
        }

        if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - time_since_last_msg)
             > chrono::seconds(CLIENT_WAIT_TIME_S)) {
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(CLIENT_MESSAGE_CHECK_DELAY_MS));
    }

    if (clientSocket != (unsigned long long)SOCKET_ERROR) {
        socket_is_non_blocking = false;

        expected<Unit, FancyError> discard = send_message(clientSocket, Message(
            MessageType::System,
            SERVER_DISCONNECT,
            AUTHOR_SERVER
        ));

        #ifdef DEV
        print("disconnected client {} manually, {}\n", clientID, (discard ? "successfuly" : "unsuccessfuly"));
        #endif
    }

    clientIDtoName.erase(clientID);

    print("{}client with clientID {} disconnected{}\n",
        ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);
}

auto handle_server_commands() {
    string input;
    
    while (server_active) {
        getline(cin, input);

        if (input == ":close" || input == ":c") {
            server_active = false;
            print("{}shutting down & joining threads...{}\n",
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
            print("{}unknown command{}\n", ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
        }
    }
}

int main() {
    #ifndef DEV
    get_ip_port();
    #endif

    SOCKET serverSocket;
    {
        expected<SOCKET, FancyError> res = init_wsa_and_get_socket();
        if (res) {
            serverSocket = res.value();
        } else {
            print("{}\n", res.error().text);
            return 1;
        }
    }

    {
        expected<Unit, FancyError> res = bind_and_listen(serverSocket);
        if (!res) {
            print("{}\n", res.error().text);
            return 1;
        }
    }

    print("{}== server started =={}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    print("{}commands: :close, :broadcast [message]{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

    commands_thread = thread(handle_server_commands);

    u_long socket_is_non_blocking = true;
    ioctlsocket(serverSocket, FIONBIO, &socket_is_non_blocking);

    signal(SIGINT, handle_sigint_cleanup);

    SOCKET acceptSocket;

    while (server_active) {
        acceptSocket = accept(serverSocket, NULL, NULL);

        if (acceptSocket != (unsigned long long)SOCKET_ERROR) {
            clients.emplace(lucid, clientConnection(
                acceptSocket, 
                lucid, 
                thread(handle_client, acceptSocket, clients.size())
            ));

            lucid++;
        }
    }
    
    /* if (acceptSocket == INVALID_SOCKET && server_active) {
        cout << ANSI_COLORS_RED << "accept failed: " << WSAGetLastError() << ANSI_COLORS_DEFAULT << endl;
    } */

    commands_thread.join();

    for (auto& [cID, client] : clients) {
        client.thr.join();
    }

    print("{}closing socket & server{}\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    closesocket(serverSocket);
    WSACleanup();
}



// todo: remove
auto handle_sigint_cleanup(int sig) -> void {
    print("{}ctrl-c :({}\n",
        ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
    server_active = false;
}

auto bind_and_listen(SOCKET serverSocket) -> expected<Unit, FancyError> {
    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return unexpected(FancyError(
            string(ANSI_COLORS_RED) + "bind failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err
        ));
    } else {
        #ifdef DEV
        cout << "bind is ok!" << endl;
        #endif
    }

    if (listen(serverSocket, CONNECTION_QUEUE_SIZE) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        WSACleanup();
        return unexpected(FancyError(
            string(ANSI_COLORS_RED) + "listen failed: " + to_string(err) + ANSI_COLORS_DEFAULT, err
        ));
    } else {
        print("listening with big rabit ears\n");
    }

    return Unit();
}