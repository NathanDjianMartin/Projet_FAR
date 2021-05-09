#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <semaphore.h>
#include "utils/headers/utils.h"
#include "utils/headers/server_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

pthread_t msg_thread;
pthread_t file_thread;
pthread_t file_recv_thread;
//sem_t file_semaphore;
int fake_semaphore = 0;
int fake_recv_semaphore =0;
char file_content[MAX_FILE_SIZE];
char fileName[MAX_MSG_SIZE];
int file_size;
/**
 * Configures the server's socket and updates the socket_return and addr_return values with the
 * created socket and the created address.
 * @param address the IPV4 address we want to use
 * @param port the port we want to use
 * @param socket_return the pointer where the created socket will be stored at
 * @param addr_return the address where the created sockaddr_in will be stored at
 * @return 0 if everything was successful; -1 if there was an error during socket creation
 */
int configure_connecting_socket(char* address, int port, int* socket_return, struct sockaddr_in *addr_return) {

    // creates a socket in the IPV4 domain using TCP protocol
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Erreur lors de la création de la socket serveur pour les messages.\n");
        return -1;
    }
    printf("Socket serveur créée avec succès.\n");

    // server address configuration
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // address type
    inet_pton(AF_INET, address, &(server_address.sin_addr)); //converts the address from the CLI to the correct format
    server_address.sin_port = htons(port); // address port (converted from the CLI)
    printf("Adresse du serveur configurée avec succès ! (%s:%d)\n", address, port);

    *socket_return = server_socket;
    *addr_return = server_address;
    return 0;
}

/**
 * Connects the given address to the given socket.
 * @param socket the server's socket
 * @param address the server's address (ip + port)
 * @return
 */
int connect_on(int socket, struct sockaddr_in address) {
    // connection to the server (message socket)
    socklen_t server_address_len = sizeof(struct sockaddr_in);
    int connect_res = connect(socket, (struct sockaddr*) &address, server_address_len); // opens the socket with the configured address
    if (connect_res == -1) {
        perror("Erreur lors de la connexion au serveur.\n");
        return -1;
    }
    printf("En attente de l'acceptation du serveur...\n");
    return 0;
}

void get_file_to_send(int* size_file) {
    // Demander à l'utilisateur quel fichier afficher
    DIR *dir_stream;
    struct dirent *dir_entry;
    dir_stream = opendir("./");
    if (dir_stream != NULL) {
        printf("Voilà la liste de fichiers :\n");
        while (dir_entry = readdir (dir_stream)) {
            if(strcmp(dir_entry->d_name, ".") != 0 && strcmp(dir_entry->d_name, "..") != 0) {
                // does not display the . and .. files
                printf("%s\n", dir_entry->d_name);
            }
        }
        (void) closedir(dir_stream);
    } else {
        perror ("Ne peux pas ouvrir le répertoire");
    }

    printf("Indiquer le nom du fichier : \n");
    fgets(fileName, sizeof(fileName), stdin);
    fileName[strlen(fileName) - 1] = '\0';
    int fp = open(fileName,O_RDONLY);
    int size=0;
    if (fp == -1){
        printf("Ne peux pas ouvrir le fichier suivant : %s\n", fileName);
    } else {
        char str[MAX_FILE_SIZE];
        // Stores the file content
        size = read(fp,file_content,MAX_FILE_SIZE);
        file_content[size]=0;
        printf("size %d\n",size);
        //write(1,file_content,size);

    }
    *size_file=size;
    close(fp);
}


/**
 * The client's messaging thread.
 * @param socket the server's socket
 */
void* messaging_thread(void *socket) {
    char send_buffer[MAX_MSG_SIZE];
    int server_socket = (int) (long) socket;

    // gets and sends the client's name to the server
    printf("Entrez votre pseudo (max 10 lettres) : \n");
    fgets(send_buffer, MAX_MSG_SIZE, stdin);
    send_buffer[strcspn(send_buffer, "\n")] = 0; // removes the \n at the end
    send(server_socket, send_buffer, MAX_MSG_SIZE, 0);
    printf("Bienvenue %s !\n", send_buffer);

    while (1) {
        fgets(send_buffer, MAX_MSG_SIZE, stdin);

        if (strcmp(send_buffer, "file\n") == 0) {
            // the client wants to send a file
            
            get_file_to_send(&file_size);
            //sem_post(&file_semaphore);
            fake_semaphore += 1;
            printf("Fichier envoyé.\n");
        }

        // sends the file name to receive to the server
        if (strcmp(send_buffer, "filesrv\n") == 0) {
            send(server_socket, send_buffer, MAX_MSG_SIZE, 0);
            // recevoir la liste
            printf("Entrez un fichier à recevoir: \n");
            fgets(fileName, MAX_MSG_SIZE, stdin);
        	fake_recv_semaphore+=1;

        }

        else {
            // the client wants to send a message
            send(server_socket, send_buffer, MAX_MSG_SIZE, 0);
            printf("[Vous] : %s", send_buffer);
            }
        }
    }


/**
 * The client's file sending thread.
 * @param socket the server's socket
 */
void* file_sending_thread(void *socket) {
    int server_socket = (int) (long) socket;
    while (1) {
        //sem_wait(&file_semaphore);
        while (fake_semaphore == 0) {}
        fake_semaphore -= 1;
        int len = (int) strlen(file_content);
        send(server_socket, fileName, MAX_MSG_SIZE, 0); // envoi du nom du fichier
        send(server_socket, &file_size, sizeof(int), 0); // envoi taille fichier
        send(server_socket, file_content, len, 0); // envoi du contenu du fichier
        printf("file_content: %s\n",file_content);
        printf("Fichier envoyé !\n");
        bzero(file_content, len);
        
    }
}

/**
*/
void* file_receiving_thread(void *socket) {
	printf("Coucou reception file\n");
    int server_socket = (int) (long) socket;
    while (1) {
        //sem_wait(&file_semaphore);
        while (fake_recv_semaphore == 0) {}
      
        fake_recv_semaphore -= 1;
        //int len = (int) strlen(file_content);
        send(server_socket, fileName, MAX_MSG_SIZE, 0); // envoi du nom du fichier
        printf("fileNametoSend: %s\n",fileName );
        recv(server_socket, &file_size, sizeof(int), 0); // envoi taille fichier
        printf("Received file_size : %d\n",file_size );
        char* file_recv_cont = NULL;
        file_recv_cont = malloc(file_size);
        recv(server_socket, file_recv_cont, file_size, 0); // envoi du contenu du fichier
        printf("Received file_recv_content: %s\n",file_recv_cont);
        
        free(file_recv_cont);
       
    }
}


int  create_server_socket(char *argv[],int num_port) {
    // creates a socket in the IPV4 domain using TCP protocol
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    check_error(server_socket, "Erreur lors de la création de la socket serveur.\n");
    printf("Socket serveur créée avec succès sur le port %d.\n",num_port);

    // server address configuration
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // address type
    inet_pton(AF_INET, argv[1], &(server_address.sin_addr)); //converts the address from the CLI to the correct format
    server_address.sin_port = htons(num_port); // address port (converted from the CLI)
    printf("Adresse du serveur configurée avec succès !\n");

    // connection to the server
    socklen_t server_address_len = sizeof(struct sockaddr_in);
    int connect_res = connect(server_socket, (struct sockaddr*) &server_address, server_address_len); // opens the socket with the configured address
    printf("En attente de l'acceptation du serveur...\n");
    check_error(connect_res, "Erreur lors de la connexion au serveur.\n");
    
    char msgServeur[MAX_MSG_SIZE];
    recv(server_socket, msgServeur, MAX_MSG_SIZE, 0);
    printf("%s\n", msgServeur );

    return server_socket;
}

int main(int argc, char *argv[]) {

    // checks for the correct args number
    if (argc != 3) {
        printf("Nombre d'arguments incorrect. Utilisation :\n");
        printf("%s <adresse_ip_serveur> <port>\n", argv[0]);
        exit(0);
    } else {
        printf("Lancement du client...\n");
    }


     // configures the server's file sending socket and connects to it
     /*
    int server_file_socket;
    struct sockaddr_in server_file_address;
    configure_server_socket(argv[1], atoi(argv[2]) + 1, &server_file_socket, &server_file_address);
    int file_sending_socket = connect_on(server_file_socket, server_file_address);

    int server_file_socket = create_server_socket(argv,atoi(argv[2])+1);
    // send thread start
    */

    int server_file_sending_socket;
    struct sockaddr_in server_file_sending_address;
    configure_connecting_socket(argv[1], atoi(argv[2]) + 1, &server_file_sending_socket, &server_file_sending_address);
    connect_on(server_file_sending_socket, server_file_sending_address);
    pthread_create(&file_thread, NULL, file_sending_thread, NULL);


    int server_file_receiving_socket;
    struct sockaddr_in server_file_receiving_address;
    configure_connecting_socket(argv[1], atoi(argv[2]) + 1, &server_file_receiving_socket, &server_file_receiving_address);
    connect_on(server_file_receiving_socket, server_file_receiving_address);
    pthread_create(&file_recv_thread, NULL, file_receiving_thread, NULL);


    int server_msg_socket = create_server_socket(argv,atoi(argv[2]));
    // send thread start
    pthread_create(&msg_thread, NULL, messaging_thread, (void *) (long) server_msg_socket);


    // receives the connection confirmation message from the server
    char recv_buffer[MAX_MSG_SIZE];
    // starts the messaging thread

    // starts the file sending thread
    //sem_init(&file_semaphore, PTHREAD_PROCESS_SHARED, 0);


   while (1) {
        recv(server_msg_socket, recv_buffer, MAX_MSG_SIZE, 0);
        char affichage[MAX_MSG_SIZE+14];
        strcat(affichage,"[");
        strcat(affichage,recv_buffer);
        strcat(affichage,"] : ");
        int recv_res = recv(server_msg_socket, recv_buffer, MAX_MSG_SIZE, 0);
        strcat(affichage,recv_buffer);
        printf("%s\n", affichage);
        if (recv_res == 0) {
            // the server closed the connection
            terminate_program(0);
        }
    }
}