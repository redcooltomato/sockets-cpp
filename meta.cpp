#include "compatability.cpp"

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
const char SERVER_CONNECT[] = "server connected!!!!";
const char SERVER_DISCONNECT[] = "server disconnected!!!!";
const char NAME_ACCEPTED[] = "name accepted!!! yupee!!!";
const char NAME_REJECTED[] = "name rejected! :( damn!!!!";

const char AUTHOR_SERVER[] = "-666";

const int MAX_MESSAGE_LENGTH = 300;
const int MAX_AUTHOR_LENGTH = 12;

char IP[20] = "127.0.0.1"; // defaults
int port = 30000;


struct Unit { // like the one in rust
    Unit() {}
};

enum MessageType {
    System,
    User
};

struct FancyError {
    int code;
    std::string text;
    FancyError(std::string t, int c = -1) : code(c), text(t) {}
};

struct Message {
    MessageType type;
    char content[MAX_MESSAGE_LENGTH];
    char author[MAX_AUTHOR_LENGTH];
    Message(MessageType t = MessageType::System, const char *c = "", const char *a = "") : type(t) { 
        strncpy(content, c, MAX_MESSAGE_LENGTH - 1);
        content[MAX_MESSAGE_LENGTH - 1] = '\0';
        strncpy(author, a, MAX_AUTHOR_LENGTH);
        author[MAX_AUTHOR_LENGTH - 1] = '\0'; // nullterminate my ass
    }
};


auto init_wsa_and_get_socket() -> std::expected<SOCKET, FancyError> {
    #ifdef WIN
    WSADATA wsaData;
    int wsaerr;
    WORD wVersion = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersion, &wsaData);
    if (wsaerr) {
        return std::unexpected(std::string(ANSI_COLORS_RED) + "win sock dll not found\n" + ANSI_COLORS_DEFAULT);
    } else {
        #ifdef DEV
        std::print("win sock dll found\n");
        #endif
    }
    #endif

    SOCKET newSocket = INVALID_SOCKET;
    newSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (newSocket == (long long unsigned int)INVALID_SOCKET) {
        int err = getlasterror();
        WSACleanup();
        return std::unexpected(FancyError(std::string(ANSI_COLORS_RED) + "error at socket: " + std::to_string(err) + ANSI_COLORS_DEFAULT, err));
    } else {
        #ifdef DEV
        std::print("socket is ok!\n");
        #endif
    }

    return newSocket;
}

auto send_message(SOCKET socket, Message msg) -> std::expected<Unit, FancyError> {
    int byteCount = send(socket, (char*)&msg, sizeof(msg), 0);
    if (byteCount == SOCKET_ERROR) {
        int err = getlasterror();
        return std::unexpected(FancyError(std::string(ANSI_COLORS_RED) + "error occued when sending: " + std::to_string(err) + "\n" + ANSI_COLORS_DEFAULT, err));
    }
    return Unit();
}

auto get_ip_port() -> void {
    std::print("enter ip:\n");
    std::cin >> IP;

    std::print("enter port:\n");
    do {
        std::cin >> port;
        if (port <= 1024) {
            std::print("port must be >1024 to avoid conflicts\n");
        }
    } while (port <= 1024);

    std::cin.ignore(256, '\n');
}