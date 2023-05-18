#include "helpers.h"

const char *get_access(char *host, int port, char *url,char *query_params, char **cookies, int cookies_len) {
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    char *message = compute_get_request(host, url, NULL, cookies, cookies_len, NULL);

    
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    return basic_extract_json_response(response);
}

const char* get_books(char *host, int port, char *url, char *query_params, char **cookies, int cookies_len, char *token_jwt) {
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    char *message = compute_get_request(host, url, NULL, cookies, cookies_len, token_jwt);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    if(strstr(response, "[")) {
        return strstr(response, "[");
    } else {
        char *ret_json = basic_extract_json_response(response);
        JSON_Value *val_ret = json_parse_string(ret_json);
        JSON_Object *json_ret = json_value_get_object(val_ret);
        return json_object_get_string(json_ret, "error");
    }
}

const char* get_book_info(char *host, int port, char *url, char *query_params, char **cookies, int cookies_len, char *token_jwt) {
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    char id[10];
    printf("id = ");
    scanf("%s", id);
    char new_url[100];
    strcpy(new_url, url);
    strcat(new_url, id);

    char *message = compute_get_request(host, new_url, NULL, cookies, cookies_len, token_jwt);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    return basic_extract_json_response(response);
}

const char *add_book(char *host, int port, char *payload_type, char *url, char *token_jwt) {
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    char title[50];
    char author[50];
    char genre[10];
    char publisher[20];
    char page_count[10];

    char newline[2];
    fgets(newline, 2, stdin);

    printf("title = ");
    fgets(title, 50, stdin);

    printf("author = ");
    fgets(author, 50, stdin);

    printf("genre = ");
    fgets(genre, 10, stdin);

    printf("publisher = ");
    fgets(publisher, 20, stdin);

    while (1) {
        int ok = 1;
        
        printf("page_count = ");
        fgets(page_count, 10, stdin);

        for (int i = 0 ; i < strlen(page_count) - 1; i++) {
            if (page_count[i] < '0' || page_count[i] > '9') {
                printf("Page count not valid!\n");
                ok = 0;
                break;
            }
        }

        if (ok == 1) {
            break;
        }
    }
    
    JSON_Value *value = json_value_init_object();
    JSON_Object *json = json_value_get_object(value);
    json_object_set_string(json, "title", title);
    json_object_set_string(json, "author", author);
    json_object_set_string(json, "genre", genre);
    json_object_set_string(json, "publisher", publisher);
    json_object_set_string(json, "page_count", page_count);

    char *payload = json_serialize_to_string_pretty(value);

    char* message = compute_post_request(host, url, payload_type, payload, NULL, 0, token_jwt);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *error = basic_extract_json_response(response);

    if (error == NULL) {
        return "You book has been added!";
    }

    JSON_Value *val_ret = json_parse_string(error);
    JSON_Object *json_ret = json_value_get_object(val_ret);
    return json_object_get_string(json_ret, "error");

}

const char* delete_book(char *host, int port, char *payload_type, char *url, char *token_jwt) {
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "connection");

    char id[20];
    printf("id = ");
    scanf("%s", id);

    char new_url[100];
    strcpy(new_url, url);
    strcat(new_url, id);

    char* message = compute_delete_request(host, new_url, payload_type, token_jwt);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *error = basic_extract_json_response(response);

    if (error == NULL) {
        return "You book has been deleted!";
    }

    JSON_Value *val_ret = json_parse_string(error);
    JSON_Object *json_ret = json_value_get_object(val_ret);
    return json_object_get_string(json_ret, "error");
}