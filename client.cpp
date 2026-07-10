#include "client.h"

using namespace std;

char IP[] = "127.0.0.1"; // defaults
int port = 30000;



auto send_message(SOCKET socket, Message msg) -> expected<Unit, string> {
    int byteCount = send(socket, (char*)&msg, sizeof(msg), 0);
    if (byteCount == SOCKET_ERROR) {
        string err = to_string(WSAGetLastError());
        WSACleanup();
        return unexpected(string(ANSI_COLORS_RED) + "error occued when sending: " + err + "\n" + ANSI_COLORS_DEFAULT);
    }
    return Unit();
}

int main() {
    /* get_ip_port(); */

    SOCKET clientSocket;
    {
        expected<SOCKET, string> res = init_wsa_and_get_socket();
        if (res) {
        clientSocket = res.value();
        } else {
            cout << res.error();
            return -1;
        }
    }

    {
        expected<Unit, string> res = connect_to_server(clientSocket);
        if (!res) {
            cout << res.error();
            clientSocket = SOCKET_ERROR;
        } else {
            /* send_message(clientSocket, Message(msgTypes::System, CLIENT_CONNECT)); */

            printf("type your message, up to %d characters\nuse :disconnect to disconnect\n", MAX_MESSAGE_LENGTH);
        }
    }
    
    char msg[MAX_MESSAGE_LENGTH];

    while (true && clientSocket != SOCKET_ERROR) {
        printf("%s>%s ", ANSI_COLORS_BLUE, ANSI_COLORS_DEFAULT);
        cin.getline(msg, MAX_MESSAGE_LENGTH);

        if (strcmp(msg, ":disconnect") == 0) {
            break;
        }

        expected<Unit, string> res = send_message(clientSocket, Message(msgType::User, msg));
        if (!res) {
            cout << res.error();
            clientSocket = SOCKET_ERROR;
            break;
        }
    }

    printf("%sclosing socket & server%s\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);

    if (clientSocket != SOCKET_ERROR) { // server will close connection if error occurs
        auto res = send_message(clientSocket, Message(msgType::System, CLIENT_DISCONNECT));
        if (res) { // let me close the stupid client
            closesocket(clientSocket);
        }

        WSACleanup();
    }
}



auto get_ip_port() -> void {
    printf("enter ip:\n");
    cin >> IP;
    printf("enter port:\n");
    cin >> port;
    cin.ignore(256, '\n');
}

auto connect_to_server(SOCKET clientSocket) -> expected<Unit, string> {
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        string err = to_string(WSAGetLastError());
        WSACleanup();
        return unexpected(string(ANSI_COLORS_RED) + "client connect failed: " + err + ANSI_COLORS_DEFAULT + '\n');
    } else {
        printf("%s== connected ==%s\n", ANSI_COLORS_GREEN, ANSI_COLORS_DEFAULT);
    }

    return Unit();
}