#include "helpers.h"
#include "user.h"

int main(int argc, char *argv[]) {
    char *host = "34.254.242.81";
    int port = 8080;
    char *payload_type = "application/json";
    const char *response;

    while (1) {
        char command[50];
        scanf("%s", command);

        if (strcmp(command, "register") == 0) {
            response = user_register_login(host, port, payload_type, "/api/v1/tema/auth/register");
            printf("%s\n", response);
            continue;
        }

        if (strcmp(command, "login") == 0) {
            response = user_register_login(host, port, payload_type, "/api/v1/tema/auth/login");
            printf("%s\n", response);
            continue;
        }

        if (strcmp(command, "exit") == 0) {
            break;
        }
    }

    return 0;
}
