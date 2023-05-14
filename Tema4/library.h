const char *get_access(char *host, int port, char *url, char *query_params, char **cookies, int cookies_len);
const char* get_books(char *host, int port, char *url, char *query_params, char **cookies, int cookies_len, char *tocken_jwt);
const char* get_book_info(char *host, int port, char *url, char *query_params, char **cookies, int cookies_len, char *tocken_jwt);
const char *add_book(char *host, int port, char *payload_type, char *url, char *tocken_jwt);
const char* delete_book(char *host, int port, char *payload_type, char *url, char *tocken_jwt);