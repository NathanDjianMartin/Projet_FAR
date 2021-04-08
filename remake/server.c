#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../comm_utils.h"
#include "utils.h"

//TODO: faire une fonction send et recv qui envoie et recoivent le nb de octets à transférer

int main(int argc, char *argv[]) {

    // checks for the correct args number
    if (argc != 2) {
        printf("Nombre d'arguments incorrect. Utilisation :\n");
        printf("%s <num_port>\n", argv[0]);
        exit(0);
    } else {
        printf("Lancement du serveur...\n");
    }


    /*
     * Server socket setup
     */

    // creates a socket in the IPV4 domain using TCP protocol
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    check_error(server_socket, "Erreur lors de la création de la socket serveur.\n");
    printf("Socket serveur créée avec succès.\n");

    // server address configuration
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // address type
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1])); // address port (converted from the CLI)

    // bind
    int bind_res = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)); // binds address to server socket
    check_error(bind_res, "Erreur lors du bind\n");
    printf("Bind réussi !\n");

    // listen
    int listen_res = listen(server_socket, 2); // listens for incoming connections (maximum 2 waiting connections)
    check_error(listen_res, "Erreur lors du listen\n");
    printf("Le serveur écoute sur le port %s.\n", argv[1]);


    /*
     * Clients sockets setup
     */

    // clients addresses and sockets initialization
    struct sockaddr_in client1_address; // used by "accept" to store the client's address
    int client1_socket = 0;
    struct sockaddr_in client2_address; // used by "accept" to store the client's address
    int client2_socket = 0;
    socklen_t client_address_len = sizeof(struct sockaddr_in); // len of a client address

    char buffer[MAX_SIZE];

    while (1) {

        /*
         * Clients id sending
         */

        if (client1_socket == 0) {
            // client 1 is not connected
            printf("En attente de connexion du client 1...\n");
            client1_socket = accept(server_socket, (struct sockaddr*) &client1_address, &client_address_len);
            printf("Le client 1 est connecté.\n");

            // sends the client 1 id
            int client1_id = 1;
            send(client1_socket, &client1_id, sizeof(int), 0);
            printf("ID: %d envoyé au client 1.\n", client1_id);
        }
        if (client2_socket == 0) {
            // client 2 is not connected
            printf("En attente de connexion du client 2...\n");
            client2_socket = accept(server_socket, (struct sockaddr*) &client2_address, &client_address_len);
            printf("Le client 2 est connecté.\n");

            // sends the client 2 id
            int client2_id = 2;
            send(client2_socket, &client2_id, sizeof(int), 0);
            printf("ID: %d envoyé au client 2.\n", client2_id);
        }


        /*
         * Communication between clients
         */

        int recv1_res = recv(client1_socket, buffer, MAX_SIZE, 0);
        check_error(recv1_res, "Erreur lors de la réception du message du client 1.\n");
        if (recv1_res == 0 || strcmp(buffer, END_WORD) == 0) {
            // the socket was closed or the client 1 ended the discussion with the "fin" word
            client1_socket = 0; //TODO mettre un shutdown ou un close (se documenter)
            client2_socket = 0;
            printf("Le client 1 a fermé sa socket, déconnexion des 2 clients.\n");
            continue; // goes at the top of the loop
        }
        printf("Reçu du client 1 : %s", buffer);

        send(client2_socket, buffer, MAX_SIZE, 0);
        printf("Envoyé au client 2 : %s", buffer);

        int recv2_res = recv(client2_socket, buffer, MAX_SIZE, 0);
        check_error(recv2_res, "Erreur lors de la réception du message du client 2.\n");
        if (recv2_res == 0 || strcmp(buffer, END_WORD) == 0) {
            // the socket was closed or the client 2 ended the discussion with the "fin" word
            client1_socket = 0; //TODO mettre un shutdown ou un close (se documenter)
            client2_socket = 0;
            printf("Le client 2 a fermé sa socket, déconnexion des 2 clients.\n");
            continue; // goes at the top of the loop
        }
        printf("Reçu du client 2 : %s", buffer);

        send(client1_socket, buffer, MAX_SIZE, 0);
        printf("Envoyé au client 1 : %s", buffer);
    }
}
