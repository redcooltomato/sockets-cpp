#include <cstring>

const int MAX_MESSAGE_LENGTH = 300;

const char CLIENT_CONNECT[] = "connected!!!";
const char CLIENT_DISCONNECT[] = "disconnected!!!";

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