#include <cstring>

const int MAX_MESSAGE_LENGTH = 300;

enum msgTypes {
    System,
    User
};

struct Message {
    msgTypes type;
    char content[MAX_MESSAGE_LENGTH];
    Message() {}
    Message(msgTypes t, const char* c) : type(t) { strncpy(content, c, MAX_MESSAGE_LENGTH - 1); content[MAX_MESSAGE_LENGTH - 1] = '\0'; }
};