#include "helpers.h"

const char* user_register_login(char *host, int port, char *payload_type, char *access_route) {
    char username[50],  password[50];

    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    printf("username = ");
    scanf("%s", username);
    printf("password = ");
    scanf("%s", password);

    JSON_Value *value = json_value_init_object();
    JSON_Object *json = json_value_get_object(value);
    json_object_set_string(json, "username", username);
    json_object_set_string(json, "password", password);
    char *payload = json_serialize_to_string_pretty(value);

    char* message = compute_post_request(host, access_route, payload_type, payload, NULL, 0, NULL);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *cookie;
    char *p = strstr(response, "Set-Cookie: ");
    if (p) {
        cookie = strtok(p + strlen("Set-Cookie: "), "\n");
        return cookie;
    }

    char *json_response = basic_extract_json_response(response);
    if (json_response == NULL) {
        return "You have been successfully registered!";

    } else {
        JSON_Value *val_ret = json_parse_string(json_response);
        JSON_Object *json_ret = json_value_get_object(val_ret);
        return json_object_get_string(json_ret, "error");
    }
}
