#pragma once

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <expected>
#include <string>
#include <cstring>


#define ANSI_COLORS_DEFAULT "\033[39m"
#define ANSI_COLORS_RED "\033[31m"
#define ANSI_COLORS_GREEN "\033[32m"
#define ANSI_COLORS_BLUE "\033[34m"
#define ANSI_COLORS_CYAN "\033[36m"

const char CLIENT_CONNECT[] = "connected!!!";
const char CLIENT_DISCONNECT[] = "disconnected!!!";

const int MAX_MESSAGE_LENGTH = 300;


struct Unit { // like the one in rust
    Unit() {}
};

enum msgType {
    System,
    User
};

struct Message {
    msgType type;
    char content[MAX_MESSAGE_LENGTH];
    int author;
    Message() {}
    Message(msgType t, const char* c, int a = -1) : type(t), author(a) { 
        strncpy(content, c, MAX_MESSAGE_LENGTH - 1);
        content[MAX_MESSAGE_LENGTH - 1] = '\0';
    }
};


auto init_wsa_and_get_socket() -> std::expected<SOCKET, std::string> {
    WSADATA wsaData;
    int wsaerr;
    WORD wVersion = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersion, &wsaData);
    if (wsaerr) {
        return std::unexpected(std::string(ANSI_COLORS_RED) + "win sock dll not found\n" + ANSI_COLORS_DEFAULT);
    } else {
        std::cout << "win sock dll found" << std::endl;
    }

    SOCKET newSocket = INVALID_SOCKET;
    newSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (newSocket == INVALID_SOCKET) {
        std::string err = std::to_string(WSAGetLastError());
        WSACleanup();
        return std::unexpected(std::string(ANSI_COLORS_RED) + "error at socket: " + err + ANSI_COLORS_DEFAULT + "\n" );
    } else {
        std::cout << "socket is ok!" << std::endl;
    }

    return newSocket;
}

auto send_message(SOCKET socket, Message msg) -> std::expected<Unit, std::string> {
    int byteCount = send(socket, (char*)&msg, sizeof(msg), 0);
    if (byteCount == SOCKET_ERROR) {
        std::string err = std::to_string(WSAGetLastError());
        return std::unexpected(std::string(ANSI_COLORS_RED) + "error occued when sending: " + err + "\n" + ANSI_COLORS_DEFAULT);
    }
    return Unit();
}