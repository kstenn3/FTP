#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT_NUMBER 	49999
#define BUFF_SIZE 		200
#define PORT_BUFF		8
#define LINE_QTY		"-20"
#define BACKLOG			4

/* execute cd command */
int change_directory(char *);
/* executes the ls -l command and more command */
int list_directory();
/* transfers files between client and server */
int move_file(char *, char *, int);
/* get file between server and client */
int get_file(char *, char *, int);
/* handles A and E for server response to client's commands */
int server_response(int);
/* makes a data connection for transfer data between client and server */
int data_connection(int, char *, char *);
/* display contents of a serverfile to stdout */
int show_file(char*, char*, int);
/* check file permissions */
int file_perms(char *);
/* read and write function for data transfer between client/server */
int transfer_data(int, int);
/* send client command to server */
int send_request(int, char*);
