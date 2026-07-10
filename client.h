#pragma once

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <expected>
#include <string>

#include "meta.cpp"

void get_ip_port();

std::expected<Unit, std::string> send_message(SOCKET socket, Message msg);

std::expected<Unit, std::string> connect_to_server(SOCKET clientSocket);