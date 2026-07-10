#pragma once

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

#include "meta.cpp"

void get_ip_port();

std::expected<Unit, std::string> bind_and_listen(SOCKET serverSocket);

void handle_sigint_cleanup(int sig);

void handle_client(SOCKET clientSocket, int sessionID);