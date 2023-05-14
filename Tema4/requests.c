#include "helpers.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *tocken_jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies_count > 0 && cookies != NULL) {
        sprintf(line, "Cookie: ");
        strcat(line, cookies[0]);

        for (int i = 1; i < cookies_count; i++) {
            strcat(line, "; ");
            strcat(line, cookies[i]);
        }
        compute_message(message, line);
    }

    sprintf(line, "Authorization: Bearer %s", tocken_jwt);
    compute_message(message, line);

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *payload,
                            char **cookies, int cookies_count, char *tocken_jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);


    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %ld", strlen(payload));
    compute_message(message, line);

    if (cookies_count > 0 && cookies != NULL) {
        sprintf(line, "Cookie: ");
        strcat(line, cookies[0]);

        for (int i = 1; i < cookies_count; i++) {
            strcat(line, "; ");
            strcat(line, cookies[i]);
        }
        compute_message(message, line);
    }

    // Step 5: add new line at end of header
    sprintf(line, "Authorization: Bearer %s", tocken_jwt);
    compute_message(message, line);

    compute_message(message, "");

    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    strcat(message, payload);

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char* content_type, char *tocken_jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "DELETE %s HTTP/1.1", url);

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    sprintf(line, "Authorization: Bearer %s", tocken_jwt);
    compute_message(message, line);

    // Step 4: add final new line
    compute_message(message, "");

    free(line);
    return message;
}
