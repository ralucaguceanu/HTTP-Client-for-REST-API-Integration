_Tema 4_ - Protocoale de comunicatii - _Guceanu Raluca-Zinca-Ioana 322CD_

#
Programul citeste comenzi de la tastatura si trimite cereri catre un server pe
baza acestora.

Implementarea porneste de la scheletul laboratorului 9 la care am adaugat
fisierele sursa "parson" pentru a folosi functiile deja implementate de
constructie a body-ului unui request in forma de JSON (json_object_set_string,
json_serialize_to_string) si de formatare a acestuia (json_serialize_to_string_pretty).

#
Am ales sa imi definesc o structura de tip Session cu ajutorul careia sa memorez
cookie-ul si tokenul jwt de la sesiunea curenta declarand global o variabila de
acest tip, astfel avand acces mai usor la informatiile sesiunii curente.

#
Modificari aduse laboratorului:
- request.c:
    - compute_get_request si compute_post_request: am eliminat parametrii care
    aveau legatura cu cookie-ul intrucat acesta este stocat global
    - compute_delete_request: am adaugat aceasta functie pentru cererea de
    stergere a unei carti
- helpers.c:
    - basic_extract_json_list_response: functie care extrage din raspunsul
    serverului o lista de JSON

#
Logica programului se afla in client.c, unde, pentru fiecare comanda se deschide
conexiunea cu hostul pe portul specificat si se apeleaza functia corespunzatoare
fiecarei comenzi, ulterior urmand sa se inchida conexiunea cu serverul dupa
realizarea fiecare comenzi:

- register_user: pentru a putea inregistra un utilizator pe server initializam o
valoare JSON cu ajutorul careia formam obiectul. Afisam apoi prompturi pentru
introducerea numelui de utilizator si a parolei, setam campurile din JSON folosind
json_object_set_string, iar dupa validarea datelor introduse de utilizator (sa nu
contina spatii) le serializam intr-un string, cu functia json_serialize_to_string,
pe care il dam ca parametru functiei compute_post_request care compune mesajul ce
va fi trimis catre server (send_to_server) drept cerere POST. Raspunsul primit
este salvat (receive_from_server), iar daca avem o eroare aceasta este extrasa
(basic_extract_json_response) si afisata; in sens contrar, inregistrarea a avut
loc cu succes.
- login: verificam intai sa nu fim deja logati, iar apoi se procedeaza similar ca
la register cu diferenta ca URL-ul este unul nou, specific pentru login. Daca apar
erori, inseamna ca numele de utilizator si parola nu corespund. Altfel, extragem si
memoram cookie-ul primit ca raspuns in variabila globala pentru sesiunea curenta.
- enter_library: verificam intai daca suntem logati (daca avem cookie), apoi formam
mesajul ce va fi trimis catre server drept cerere GET. Salvam raspunsul primit,
extragem tokenul jwt din acesta si il memoram in sesiunea curenta.
- get_books: verificam intai daca suntem logati, iar apoi daca avem acces la 
biblioteca (daca avem token jwt). In caz afirmativ, asemanator cu functia enter_library,
compunem mesajul, il trimitem la server si salvam raspunsul. Din raspuns ne extragem
lista de carti. In cazul in care aceasta este goala afisam un mesaj corespunzator,
altfel formatam raspunsul adus de server sub forma specificata in cerinta folosind
json_parse_string si json_serialize_to_string_pretty si afisam.
- get_book: pentru a vizualiza o carte trebuie sa fim logati si sa avem acces la
biblioteca. Dupa aceste verificari, oferim utilizatorului un prompt unde va
introduce id-ul cartii pe care doreste sa o vada, lasand validarea acestuia pe
seama serverului, care aduce eroare daca id-ul nu a fost gasit sau daca nu este un
numar. Adaugam id-ul in URL ca sa formam mesajul ce va fi trimis la server drept
cerere GET. Salvam raspunsul primit, iar daca acesta contine o eroare o extragem
si o afisam, altfel afisam cartea asemanator ca la get_books.
- add_book: verificam daca suntem logati si daca avem acces la biblioteca. Apoi,
aplicam aceeasi logica de la login, intrucat si aceasta este o cerere POST.
Folosim insa URL-ul specific pentru book si campurile necesare pentru o carte.
Erorile tratate sunt pentru campuri goale si pentru id non-numeric.
- delete_book: procedam asemanator ca la functia get_book, folosind insa
compute_delete_request cu URL-ul specific
- logout: nu putem da logout decat daca am fost autentificati inainte, prin urmare
verificam si aici cookie-ul. Compunem mesajul, il trimitem la server drept cerere
GET si salvam raspunsul. Daca primim raspuns afirmativ de la server, eliberam
memoria alocata sesiunii curente, altfel afisam eroarea intoarsa de server.
- exit: la aceasta comanda nu se intra in while, iar programul se opreste.
