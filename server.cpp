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

    SOCKET serverSocket = INVALID_SOCKET;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "error at socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    } else {
        cout << "socket is ok!" << endl;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPtonA(AF_INET, "127.0.0.1", &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        cout << "bind failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    } else {
        cout << "bind is ok!" << endl;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        cout << "listen failed: " << WSAGetLastError() << endl;
    } else {
        cout << "listen succeded, listening with big rabbit ears" << endl;
    }

    SOCKET acceptSocket;
    acceptSocket = accept(serverSocket, NULL, NULL);
    if (acceptSocket == INVALID_SOCKET) {
        cout << "accept failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    char receiveBuffer[200] = "";
    int byteCount = recv(acceptSocket, receiveBuffer, 200, 0);
    if (byteCount == SOCKET_ERROR) {
        cout << "receive error: " << WSAGetLastError() << endl;
        return 0;
    } else {
        printf("received %ld bytes of data:\n", byteCount);
        cout << receiveBuffer << endl;
    }

    printf("closing socket & server\n");
    closesocket(serverSocket);
    WSACleanup();
}