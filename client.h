#pragma once

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <expected>
#include <string>
#include <thread>
#include <print>

#include "meta.cpp"


auto handle_server(SOCKET clientSocket) -> void;

void get_ip_port();

auto connect_to_server(SOCKET clientSocket) -> std::expected<Unit, std::string>;