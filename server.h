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

auto get_ip_port() -> void;

auto bind_and_listen(SOCKET serverSocket) -> std::expected<Unit, std::string>;

auto handle_sigint_cleanup(int sig) -> void;

auto handle_client(SOCKET clientSocket, int sessionID) -> void;