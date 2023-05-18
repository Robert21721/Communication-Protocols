#include "helpers.h"
#include "user.h"
#include "library.h"
#define N 10

int is_logged_in(char **cookies, int cookies_len) {
    for (int i = 0; i < cookies_len; i++) {
        if (strstr(cookies[i], "connect.sid")) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char *host = "34.254.242.81";
    int port = 8080;
    char *payload_type = "application/json";
    char **cookies;
    int cookies_len = 0;
    const char *response;
    char *token_jwt = NULL;

    cookies = malloc(N * sizeof(char*));
    for (int i = 0; i < N; i++) {
        cookies[i] = malloc(2 * N * sizeof(char));
    }

    while (1) {
        char command[50];
        scanf("%s", command);

        if (strcmp(command, "register") == 0) {
            if (is_logged_in(cookies, cookies_len)) {
                printf("You are already logged in!\n");
                continue;
            }

            response = user_register_login(host, port, payload_type, "/api/v1/tema/auth/register");
            printf("%s\n", response);
            continue;
        }

        if (strcmp(command, "login") == 0) {
            if (is_logged_in(cookies, cookies_len)) {
                printf("You are already logged in!\n");
                continue;
            }

            response = user_register_login(host, port, payload_type, "/api/v1/tema/auth/login");
            printf("%s\n", response);

            if (strstr(response, "connect.sid")) {
                strcpy(cookies[cookies_len++], strtok(strdup(response), ";"));
            }
            continue;
        }

        if (strcmp(command, "enter_library") == 0) {
            response = get_access(host, port, "/api/v1/tema/library/access", NULL, cookies, cookies_len);

            JSON_Value *val_ret = json_parse_string(response);
            JSON_Object *json_ret = json_value_get_object(val_ret);
            if (json_object_get_string(json_ret, "token")) {
                token_jwt = malloc(500 * sizeof(char));
                strcpy(token_jwt, json_object_get_string(json_ret, "token"));
                printf("%s\n", token_jwt);
            } else {
                printf("%s\n", json_object_get_string(json_ret, "error"));
            }

            continue;
        }

        if (strcmp(command, "get_books") == 0) {
            response = get_books(host, port, "/api/v1/tema/library/books", NULL, cookies, cookies_len, token_jwt);
            printf("%s\n", response);
            continue;
        }

        if (strcmp(command, "get_book") == 0) {
            response = get_book_info(host, port, "/api/v1/tema/library/books/", NULL, cookies, cookies_len, token_jwt);

            JSON_Value *val_ret = json_parse_string(response);
            JSON_Object *json_ret = json_value_get_object(val_ret);
            if (json_object_get_string(json_ret, "error")) {
                printf("%s\n", json_object_get_string(json_ret, "error"));
            } else {
                printf("%s\n", response);
            }

            continue;
        }

        if (strcmp(command, "add_book") == 0) {
            response = add_book(host, port, payload_type, "/api/v1/tema/library/books", token_jwt);
            printf("%s\n", response);

            continue;
        }

        if (strcmp(command, "delete_book") == 0) {
            response = delete_book(host, port, payload_type, "/api/v1/tema/library/books/", token_jwt);
            printf("%s\n", response);

            continue;
        }

        if (strcmp(command, "logout") == 0) {
            if (is_logged_in(cookies, cookies_len)) {
                cookies_len = 0;
                free(token_jwt);
                token_jwt = NULL;
            } else {
                printf("You are not logged in!\n");
            }

            continue;
        }

        if (strcmp(command, "exit") == 0) {
            break;
        }

        printf("Invalid command\n");
    }

    return 0;
}
