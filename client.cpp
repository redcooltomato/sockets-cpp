#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include "types.h"
using namespace std;

char IP[] = "127.0.0.1"; // defaults
int port = 30000;

void get_ip_port() {
    printf("enter ip:\n");
    cin >> IP;
    printf("enter port:\n");
    cin >> port;
    cin.ignore(256, '\n');
}

int send_message(SOCKET socket, Message msg) { // 0 if ok, -1 if err
    int byteCount = send(socket, (char*)&msg, sizeof(msg), 0);
    if (byteCount == SOCKET_ERROR) {
        cout << "send error: " << WSAGetLastError() << endl;
        return -1;
    }
    return 0;
}

int main() {
    get_ip_port();

    WSADATA wsaData;
    int wsaerr;
    WORD wVersion = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersion, &wsaData);
    if (wsaerr) {
        cout << "win sock dll not found" << endl;
        return 0;
    } else {
        cout << "win sock dll found" << endl;
    }

    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "error at socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    } else {
        cout << "socket is ok!" << endl;
    }

    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        cout << "client connect failed: " << endl;
        WSACleanup();
        return 0;
    } else {
        cout << "client connect ok" << endl;
    }

    /* send_message(clientSocket, Message(msgTypes::System, CLIENT_CONNECT)); */
    
    char msg[MAX_MESSAGE_LENGTH];

    for (int i = 0; i < 3; i++) {
        printf("type your message, upto %d characters\n> ", MAX_MESSAGE_LENGTH);
        cin.getline(msg, MAX_MESSAGE_LENGTH);

        if (send_message(clientSocket, Message(msgType::User, msg)) == -1) {
            cout << "error occured while sending message: " << WSAGetLastError() << endl;
            WSACleanup();
            return 0;
        } else {
            printf("sent!\n");
        }
    }

    send_message(clientSocket, Message(msgType::System, CLIENT_DISCONNECT));
    
    printf("closing socket & client\n");
    closesocket(clientSocket);
    WSACleanup();
}