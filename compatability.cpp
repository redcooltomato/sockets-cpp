#pragma once

#include <unordered_map> // for socket block switching on win

enum KernelType {
    Windows,
    Unix
};

#ifdef WIN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

const KernelType kernel = ::Windows;

#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

const KernelType kernel = ::Unix;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

typedef unsigned long long SOCKET;

void WSACleanup() {}
int closesocket(SOCKET socket) { return close(socket); }
#endif

auto getlasterror() -> int {
    #ifdef WIN // so linter wont yap
    return WSAGetLastError();
    #else
    return errno;
    #endif
}

auto set_socket_blocking(SOCKET socket, bool blocking) -> int {
    #ifdef WIN
    static std::unordered_map<SOCKET, unsigned long> sock_to_par;

    sock_to_par[socket] = !blocking;

    return ioctlsocket(socket, FIONBIO, &sock_to_par[socket]);
    #else
    int flags = fcntl(socket, F_GETFL);

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    
    return fcntl(socket, F_SETFL, flags);
    #endif
}