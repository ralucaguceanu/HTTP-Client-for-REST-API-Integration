#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <ctype.h>
#include "buffer.h"
#include "helpers.h"
#include "requests.h"
#include "parson.h"
#include "client.h"

#define HOST "34.246.184.49"
#define PORT 8080
#define content_type "application/json"
#define register_url "/api/v1/tema/auth/register"
#define login_url "/api/v1/tema/auth/login"
#define access_url "/api/v1/tema/library/access"
#define books_url "/api/v1/tema/library/books"
#define book_url "/api/v1/tema/library/books/"
#define logout_url "/api/v1/tema/auth/logout"
#define max_len 100
#define cookie_name "connect.sid="

Session *curr_session;

void register_user(int sockfd)
{
    // avem alocata memorie pentru sesiunea curenta, deci deja suntem logati
    if (curr_session != NULL) {
        printf("You can not register if you are already logged in.\n");
        return;
    }

    // initializam jsonul ce va contine body-ul requestului
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    char *message, *response;
    char username[max_len], password[max_len];

    printf("%s", "username=");
    fgets(username, sizeof(username), stdin);

    username[strlen(username) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "username", username);  // adaugarea campului pentru username

    printf("%s", "password=");
    fgets(password, sizeof(password), stdin);

    password[strlen(password) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "password", password);  // adaugarea campului pentru password

    // validarea username-ului si a parolei pentru a nu contine spatii
    if (strchr(username, ' ') != NULL || strchr(password, ' ') != NULL) {
        printf("Error! Username and password should not contain white spaces!\n");
        json_value_free(json_value);
        return;
    }

    // serializam jsonul pentru a-l trimite catre server
    char *data_body = json_serialize_to_string(json_value);

    // compunem mesajul
    message = compute_post_request(HOST, register_url, content_type, &data_body, 1);

    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul de la server
    response = receive_from_server(sockfd);


    // in cazul in care raspunsul primit este o eroare, o afisam, 
    // altfel, inregistrarea s-a realizat cu succes
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
    } else {
        printf("Success! You have been registered.\n");
    }

    // eliberam memoria folosita
    json_value_free(json_value);
    json_free_serialized_string(data_body);
}

void login(int sockfd)
{
    // avem alocata memorie pentru sesiunea curenta si pentru cookie,
    // deci deja suntem logati
    if (curr_session != NULL && curr_session->cookie != NULL) {
        printf("Error! You are already logged in!\n");
        return;
    }

    // initializam jsonul ce va contine body-ul requestului
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    char *message, *response;
    char username[max_len], password[max_len];

    printf("%s", "username=");
    fgets(username, sizeof(username), stdin);

    username[strlen(username) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "username", username);  // adaugarea campului pentru username

    printf("%s", "password=");
    fgets(password, sizeof(password), stdin);

    password[strlen(password) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "password", password);  // adaugarea campului pentru password

    // validarea username-ului si a parolei pentru a nu contine spatii
    if (strchr(username, ' ') != NULL || strchr(password, ' ') != NULL) {
        printf("Error! Username and password should not contain white spaces!\n");
        json_value_free(json_value);
        return;
    }

    // serializam jsonul pentru a-l trimite catre server
    char *data_body = json_serialize_to_string(json_value);

    // compunem mesajul
    message = compute_post_request(HOST, login_url, content_type, &data_body, 1);
    
    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul de la server
    response = receive_from_server(sockfd);

    // in cazul in care raspunsul primit este o eroare, o afisam, altfel, 
    // inregistrarea s-a realizat cu succes, deci initializam sesiunea curenta,
    // pastrand valoarea cookie-ului primit in raspuns
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
    } else {
        char *first_occ_cookie = strstr(response, cookie_name);

        char* cookie = strtok(first_occ_cookie, ";");
        curr_session = (Session *)malloc(sizeof(Session));

        curr_session->cookie = (char *)malloc(strlen(cookie) * sizeof(char));
        memcpy(curr_session->cookie, cookie, strlen(cookie));
        printf("Success! You have been logged in.\n");
    }

    // eliberam memoria folosita
    json_value_free(json_value);
    json_free_serialized_string(data_body);
}

void enter_library(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    char *message, *response;

    // compunem mesajul
    message = compute_get_request(HOST, access_url, NULL);
    
    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul de la server
    response = receive_from_server(sockfd);

    // extragem tokenul din raspuns
    char *token = strstr(response, "token");
    char *jwt_token = strtok(token, ":\"");
    jwt_token = strtok(NULL, ":\"");

    curr_session->jwt_token = (char *)malloc(strlen(jwt_token) * sizeof(char));

    // salvam tokenul in sesiunea curenta
    memcpy(curr_session->jwt_token, jwt_token, strlen(jwt_token));
    printf("Success! You have access to the library.\n");
}

void get_books(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    // nu avem memorie alocata pentru tokenul jwt, prin urmare nu avem accces
    // la biblioteca
    if (curr_session->jwt_token == NULL) {
        printf("Error! You do not have access to the library.\n");
        return;
    }

    char *message, *response;

    // copunem mesajul
    message = compute_get_request(HOST, books_url, NULL);

    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul de la server
    response = receive_from_server(sockfd);

    // in cazul in care primim bad request de la server
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
        return;
    }

    // extragem cartile din json pentru a le putea afisa
    char *books = basic_extract_json_list_response(response);

    // tratam cazurile cand nu avem carti, in caz opus, parsam stringul
    // si il formatam corespunzator
    if (strstr(books, "[]")) {
        printf("You have no books.\n");
    } else {
        printf("%s\n", json_serialize_to_string_pretty(json_parse_string(books)));
    }
}

void get_book(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    // nu avem memorie alocata pentru tokenul jwt, prin urmare nu avem accces
    // la biblioteca
    if (curr_session->jwt_token == NULL) {
        printf("Error! You do not have access to the library.\n");
        return;
    }

    char buf[max_len];
    printf("id=");
    scanf("%s", buf);

    // adaugam id-ul cartii dorite in url
    char url_with_id[max_len] = book_url;
    strcat(url_with_id, buf);

    char *message, *response;

    // compunem mesajul
    message = compute_get_request(HOST, url_with_id, NULL);

    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul de la server
    response = receive_from_server(sockfd);

    // in cazul in care raspunsul primit este o eroare, o afisam, 
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
        return;
    }

    // extragem cartea din json pentru a o putea afisa
    char *book = basic_extract_json_response(response);

    // parsam stringul si il formatam corespunzator
    printf("%s\n", json_serialize_to_string_pretty(json_parse_string(book)));
}

void add_book(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    // nu avem memorie alocata pentru tokenul jwt, prin urmare nu avem accces
    // la biblioteca
    if (curr_session->jwt_token == NULL) {
        printf("Error! You do not have access to the library.\n");
        return;
    }

    char buf[max_len];

    // initializam jsonul ce va contine body-ul requestului
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    char *message, *response;
    char page_count[max_len];
    int missing_fields = 0;

    printf("%s", "title=");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "title", buf);  // adaugarea campului pentru titlu

    if (buf[0] == '\0') {
        missing_fields++;  // incrementam numarul de campuri goale
    }

    printf("%s", "author=");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "author", buf);  // adaugarea campului pentru autor

    if (buf[0] == '\0') {
        missing_fields++;  // incrementam numarul de campuri goale
    }

    printf("%s", "genre=");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "genre", buf);  // adaugarea campului pentru gen

    if (buf[0] == '\0') {
        missing_fields++;  // incrementam numarul de campuri goale
    }

    printf("%s", "page_count=");
    fgets(page_count, sizeof(page_count), stdin);
    page_count[strlen(page_count) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "page_count", page_count);  // adaugarea campului pentru nr de pagini

    if (page_count[0] == '\0') {
        missing_fields++;  // incrementam numarul de campuri goale
    }

    printf("%s", "publisher=");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';  // pentru a elimina '\n'-ul citit de fgets
    json_object_set_string(json_object, "publisher", buf);  // adaugarea campului pentru editura

    if (buf[0] == '\0') {
        missing_fields++;  // incrementam numarul de campuri goale
    }
    
    // tratam cazul pentru campurile goale
    if (missing_fields > 0) {
        printf("Error! There are %d missing fields.\n", missing_fields);
        json_value_free(json_value);
        return;
    }

    // tratam cazul cand valoarea introdusa pentru numarul de pagini nu este, de fapt, un numar
    if (!is_number(page_count)) {
        printf("Error! \"%s\" should be a number.\n", page_count);
        json_value_free(json_value);
        return;
    }
    
    // serializam jsonul pentru a-l trimite catre server
    char *data_body = json_serialize_to_string(json_value);

    // compunem mesajul
    message = compute_post_request(HOST, books_url, content_type, &data_body, 1);
    
    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul primit de la server
    response = receive_from_server(sockfd);

    // in cazul in care raspunsul primit este o eroare, o afisam, 
    // altfel, inregistrarea s-a realizat cu succes
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
    } else {
        printf("Success! Your book has been added.\n");
    }

    // eliberam memoria folosita
    json_value_free(json_value);
    json_free_serialized_string(data_body);
}

void delete_book(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    // nu avem memorie alocata pentru tokenul jwt, prin urmare nu avem accces
    // la biblioteca
    if (curr_session->jwt_token == NULL) {
        printf("Error! You do not have access to the library.\n");
        return;
    }

    char buf[max_len];
    printf("id=");
    scanf("%s", buf);

    // adaugam id-ul cartii dorite in url
    char url_with_id[max_len] = book_url;
    strcat(url_with_id, buf);

    char *message, *response;

    // compunem mesajul
    message = compute_delete_request(HOST, url_with_id);

    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul primit de la server
    response = receive_from_server(sockfd);

    // in cazul in care raspunsul primit este o eroare, o afisam
    if (strstr(response, "error") != NULL) {
        printf("%s\n", basic_extract_json_response(response));
        return;
    }

    // inregistrarea s-a realizat cu succes
    if (strstr(response, "ok") != NULL) {
        printf("Success! The book with id %s has been deleted.\n", buf);
    }
}

void logout(int sockfd)
{
    // nu avem memorie alocata pentru sesiunea curenta, prin urmare nu am fost
    // logati anterior
    if (curr_session == NULL || curr_session->cookie == NULL) {
        printf("Error! You are not logged in.\n");
        return;
    }

    char *message, *response;

    // compunem mesajul
    message = compute_get_request(HOST, logout_url, NULL);

    // trimitem requestul
    send_to_server(sockfd, message);

    // salvam raspunsul primit de la server
    response = receive_from_server(sockfd);

    // in cazul in care raspunsul primit nu este o eroare, eliberam memoria
    // pentru sesiunea curenta
    if (strstr(response, "ok") != NULL) {
        memset(curr_session->cookie, 0, strlen(curr_session->cookie));
        free(curr_session->cookie);
        if (curr_session->jwt_token != NULL) {
            memset(curr_session->jwt_token, 0, strlen(curr_session->jwt_token));
            free(curr_session->jwt_token);
        }
        free(curr_session);
        curr_session = NULL;
        printf("Success! You have been logged out.\n");
    } else {  // altfel, afisam eroarea
        printf("%s\n", basic_extract_json_response(response));
    }
}

int is_number(char *buf)
{
    int i = 0;
    if (buf == NULL || buf[0] == '\0') {
        return 0;
    }
    while (buf[i] != '\0') {
        if (!isdigit(buf[i])) {
            printf("%c ", buf[i]);
            return 0;
        }
        i++;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    char command[max_len];
    int sockfd;

    while (strcmp(fgets(command, sizeof(command), stdin), "exit\n") != 0) {

        // deschidem conexiunea cu serverul
        sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
        
        // apelam functia corespunzatoare comenzii curente
        if (strcmp(command, "register\n") == 0) {
            register_user(sockfd);
		} else if (strcmp(command, "login\n") == 0) {
			login(sockfd);
		} else if (strcmp(command, "enter_library\n") == 0) {
			enter_library(sockfd);
        } else if (strcmp(command, "get_books\n") == 0) {
            get_books(sockfd);
        } else if (strcmp(command, "get_book\n") == 0) {
            get_book(sockfd);
        } else if (strcmp(command, "add_book\n") == 0) {
            add_book(sockfd);
        } else if (strcmp(command, "delete_book\n") == 0) {
            delete_book(sockfd);
        } else if (strcmp(command, "logout\n") == 0) {
            logout(sockfd);
        }
        
        // inchidem conexiunea cu serverul
        close_connection(sockfd);
    }

    // eliberam memoria alocata pentru sesinea curenta
    if (curr_session != NULL && curr_session->cookie != NULL) {
        free(curr_session->cookie);
    }
    if (curr_session != NULL && curr_session->jwt_token != NULL) {
        free(curr_session->jwt_token);
    }
    free(curr_session);
    return 0;
}
