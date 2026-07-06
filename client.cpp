#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
using namespace std;

const int port = 30000;

int main() {
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
    InetPtonA(AF_INET, "127.0.0.1", &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        cout << "client connect failed: " << endl;
        WSACleanup();
        return 0;
    } else {
        cout << "client connect ok" << endl;
    }
    
    char sendBuffer[200];
    printf("enter your message (200 characters limit)\n");
    cin.getline(sendBuffer, 200);
    int byteCount = send(clientSocket, sendBuffer, 200, 0);
    if (byteCount == SOCKET_ERROR) {
        cout << "send error: " << WSAGetLastError() << endl;
        return 0;
    } else {
        printf("sent server %ld bytes\n", byteCount);
    }

    printf("closing socket & client\n");
    closesocket(clientSocket);
    WSACleanup();
}