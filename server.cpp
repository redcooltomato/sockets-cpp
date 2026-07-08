#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <signal.h>
#include "types.h"
using namespace std;

const int CONNECTION_QUEUE_SIZE = 5;
const int USER_MESSAGE_CHECK_DELAY_MS = 250;
const int CLIENT_WAIT_TIME_S = 60;

char IP[20] = "127.0.0.1"; // defaults
int port = 30000;

bool server_active = true;

void handle_sigint_cleanup(int sig) {
    printf("ctrl-c!\n");
    server_active = false;
}

int handle_user(SOCKET userSocket, int sessionID) { // 0 if ok, -1 if err
    printf("user with sessionID %d connected!\n", sessionID);

    Message received_msg;
    int byteCount = 0;
    auto time_since_last_msg = chrono::steady_clock::now();
    while (userSocket != SOCKET_ERROR && byteCount != SOCKET_ERROR && server_active) {
        int byteCount = recv(userSocket, (char*)&received_msg, 200, 0);

        if (byteCount != SOCKET_ERROR && byteCount != 0) {
            printf("received %ld bytes of data:\n", byteCount);
            cout << received_msg.type << " " << received_msg.content << endl;

            time_since_last_msg = chrono::steady_clock::now();
        }

        if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - time_since_last_msg)
             > chrono::seconds(CLIENT_WAIT_TIME_S)) {
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(USER_MESSAGE_CHECK_DELAY_MS));
    }

    printf("user with sessionID %d disconnected\n", sessionID);

    return 0;
}

int main() {
    WSADATA wsaData;
    int wsaerr;
    WORD wVersion = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersion, &wsaData);
    if (wsaerr) {
        cout << "win sock dll not found" << endl;
        return -1;
    } else {
        cout << "win sock dll found" << endl;
    }

    SOCKET serverSocket = INVALID_SOCKET;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "error at socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    } else {
        cout << "socket is ok!" << endl;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPtonA(AF_INET, IP, &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        cout << "bind failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    } else {
        cout << "bind is ok!" << endl;
    }

    if (listen(serverSocket, CONNECTION_QUEUE_SIZE) == SOCKET_ERROR) {
        cout << "listen failed: " << WSAGetLastError() << endl;
    } else {
        cout << "listen succeded, listening with big rabbit ears" << endl;
    }

    signal(SIGINT, handle_sigint_cleanup);

    vector<thread> threads;
    SOCKET acceptSocket;
    while (acceptSocket != INVALID_SOCKET && server_active) {
        acceptSocket = accept(serverSocket, NULL, NULL);

        if (acceptSocket == INVALID_SOCKET) break;

        threads.push_back(thread(handle_user, acceptSocket, threads.size()));
    }
    
    if (acceptSocket == INVALID_SOCKET && server_active) {
        cout << "accept failed: " << WSAGetLastError() << endl;
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    printf("closing socket & server\n");
    closesocket(serverSocket);
    WSACleanup();
}