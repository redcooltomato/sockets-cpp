#include "server.h"

using namespace std;


const int CONNECTION_QUEUE_SIZE = 5;
const int CLIENT_MESSAGE_CHECK_DELAY_MS = 250;
const int CLIENT_WAIT_TIME_S = 60;

char IP[20] = "127.0.0.1"; // defaults
int port = 30000;

bool server_active = true;

vector<clientConnection> clients;


auto handle_client(SOCKET clientSocket, int clientID) -> void {
    printf("%sclient with clientID %d connected!%s\n", ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);

    Message received_msg;
    int byteCount = 0;
    auto time_since_last_msg = chrono::steady_clock::now();

    u_long why_do_i_have_to_pass_reference = 1;
    ioctlsocket(clientSocket, FIONBIO, &why_do_i_have_to_pass_reference);
    
    while (clientSocket != SOCKET_ERROR && server_active) {
        int byteCount = recv(clientSocket, (char*)&received_msg, sizeof(Message), 0);

        if (byteCount > 0) {
            if (received_msg.type == msgType::System && strcmp(received_msg.content, CLIENT_DISCONNECT) == 0) {
                break;
            }

            printf("%sclient %d: %s%s\n", 
                (received_msg.type == msgType::System ? ANSI_COLORS_GREEN : ANSI_COLORS_BLUE), clientID,
                ANSI_COLORS_DEFAULT, received_msg.content);

            if (received_msg.type == msgType::User) {
                received_msg.author = clientID;
                for (auto client_ptr = clients.begin(); client_ptr != clients.end(); client_ptr++) {
                    if (client_ptr->clientID == clientID) continue;
                    auto res = send_message(client_ptr->socket, received_msg);
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

    printf("%sclient with clientID %d disconnected%s\n", ANSI_COLORS_CYAN, clientID, ANSI_COLORS_DEFAULT);
}

int main() {
    /* get_ip_port(); */

    SOCKET serverSocket;
    {
        expected<SOCKET, string> res = init_wsa_and_get_socket();
        if (res) {
            serverSocket = res.value();
        } else {
            cout << res.error();
            return -1;
        }
    }

    {
        expected<Unit, string> res = bind_and_listen(serverSocket);
        if (!res) {
            cout << res.error();
            return -1;
        }
    }

    signal(SIGINT, handle_sigint_cleanup);

    SOCKET acceptSocket;

    while (acceptSocket != INVALID_SOCKET && server_active) {
        acceptSocket = accept(serverSocket, NULL, NULL);

        if (acceptSocket == INVALID_SOCKET) break;

        clients.push_back(clientConnection(acceptSocket, clients.size(), thread(handle_client, acceptSocket, clients.size())));
    }
    
    if (acceptSocket == INVALID_SOCKET && server_active) {
        cout << ANSI_COLORS_RED << "accept failed: " << WSAGetLastError() << ANSI_COLORS_DEFAULT << endl;
    }

    for (auto client_ptr = clients.begin(); client_ptr != clients.end(); client_ptr++) {
        client_ptr->thr.join();
    }

    printf("%sclosing socket & server%s\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    closesocket(serverSocket);
    WSACleanup();
}



// stuff i dont want to look at

auto get_ip_port() -> void {
    printf("enter ip:\n");
    cin >> IP;
    printf("enter port:\n");
    cin >> port;
}

auto handle_sigint_cleanup(int sig) -> void {
    printf("%sctrl-c :(%s\n", ANSI_COLORS_RED, ANSI_COLORS_DEFAULT);
    server_active = false;
}

auto bind_and_listen(SOCKET serverSocket) -> expected<Unit, string> {
    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        string err = to_string(WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return unexpected(string(ANSI_COLORS_RED) + "bind failed: " + err + ANSI_COLORS_DEFAULT + "\n");
    } else {
        cout << "bind is ok!" << endl;
    }

    if (listen(serverSocket, CONNECTION_QUEUE_SIZE) == SOCKET_ERROR) {
        string err = to_string(WSAGetLastError());
        WSACleanup();
        return unexpected(string(ANSI_COLORS_RED) + "listen failed: " + err + ANSI_COLORS_DEFAULT + '\n');
    } else {
        printf("%s== listening with big rabbit ears ==%s\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    }

    return Unit();
}