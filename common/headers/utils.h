#ifndef PROJET_FAR_UTILS_H
#define PROJET_FAR_UTILS_H
#endif

#define MAX_MSG_SIZE 1000
#define MAX_FILE_SIZE 10000
#define MAX_NAME_SIZE 12
#define SERVER_DIR "./server_files/"
#define CLIENT_DIR "./"
#define NB_MAX_ROOM 3
#define MAX_CLIENTS 3

void check_error(int check, char *err_msg);

void terminate_program(int code);

void printf_red(char *message);

int value_in_array(char* val, char** arr);

void list_files(char* dir_name, char** buffer);

int get_last_tty();

FILE* new_tty();

//int getCommandId(char* command);

