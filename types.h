#include <cstring>

const int MAX_MESSAGE_LENGTH = 300;

const char CLIENT_CONNECT[] = "connected!!!";
const char CLIENT_DISCONNECT[] = "disconnected!!!";

#define ANSI_COLORS_DEFAULT "\033[39m"
#define ANSI_COLORS_RED "\033[31m"
#define ANSI_COLORS_GREEN "\033[32m"
#define ANSI_COLORS_BLUE "\033[34m"
#define ANSI_COLORS_CYAN "\033[36m"

enum msgType {
    System,
    User
};

struct Message {
    msgType type;
    char content[MAX_MESSAGE_LENGTH];
    Message() {}
    Message(msgType t, const char* c) : type(t) { strncpy(content, c, MAX_MESSAGE_LENGTH - 1); content[MAX_MESSAGE_LENGTH - 1] = '\0'; }
};