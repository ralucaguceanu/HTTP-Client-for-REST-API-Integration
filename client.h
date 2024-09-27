// structura pentru memorarea cookie-ului si a tokenului jwt
// pentru fiecare noua sesiune
typedef struct {
    char *cookie;
    char *jwt_token;
} Session;

// variabila globala pentru sesiunea curenta
extern Session *curr_session;

// functie pentru comanda register
void register_user(int sockfd);

// functie pentru comanda login
void login(int sockfd);

// functie pentru comanda enter_library
void enter_library(int sockfd);

// functie pentru comanda get_books
void get_books(int sockfd);

// functie pentru comanda get_book
void get_book(int sockfd);

// functie pentru comanda add_book
void add_book(int sockfd);

// functie pentru comanda logout
void logout(int sockfd);

// functie care verifica daca un string este un numar sau nu
int is_number(char *buf);
