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

#include "meta.h"

using namespace std;

void get_ip_port();

expected<SOCKET, string> init_and_get_socket();

void handle_sigint_cleanup(int sig);

void handle_client(SOCKET clientSocket, int sessionID);