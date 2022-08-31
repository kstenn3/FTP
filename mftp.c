#include "mftp.h"
/* function to handle commands performed locally on the client */
void local_commands(char**, char*, int);
/* function to handle commands performed remotely on the server */
void remote_commands(char**, char*, int);
/* function that handles exiting a client */
void exit_command(char**, int);
/* function connects client to server */
int client_connect(char*, char*);
/* function displays server's cwd */
void remote_ls(char*, int);
/* function changes server's directory */
int remote_cd(char*, char*, int);

int main(int argc, char *argv[]) {
	int socketfd;
	char *buffer = (char *)malloc(sizeof(char)*BUFF_SIZE);
	char *path = argv[2];

	/* check user input has port and host */
	if (argc != 3){printf("Usage: ./mftp <port> <hostname | IP address>\n"); exit(0);}

	socketfd = client_connect(argv[1], argv[2]);
	printf("Connected to server %s\n", argv[2]);

	while(1) {
		char *user_cmd[2];
		printf("MFTP>>> ");

		fgets(buffer, BUFF_SIZE, stdin);
		if(strcmp(buffer, "\n") == 0) continue;			/* handles seg fault for pressing enter*/

		/* parse user command string */
		user_cmd[0] = strtok(buffer, " \t\r\n\v\f");	/* stores command */
		user_cmd[1] = strtok(NULL, " \t\r\n\v\f");		/* stores optional pathname */

		/* handle user commands */
		if(strcmp(user_cmd[0], "exit") == 0) {
			free(buffer);
			exit_command(user_cmd, socketfd);
		}
		else if(user_cmd[0][0] == 'r') remote_commands(user_cmd, path, socketfd);
		else if(strcmp(user_cmd[0], "get") == 0) remote_commands(user_cmd, path, socketfd);
		else if(strcmp(user_cmd[0], "show") == 0) remote_commands(user_cmd, path, socketfd);
		else local_commands(user_cmd, path, socketfd);
	}

	return 0;
}

/* handles local requests on server */
void local_commands(char **input, char* host, int socketfd) {
	if(strcmp(input[0], "cd") == 0){
		/* change directories */
		printf("performing - '%s'\n", input[0]);
		change_directory(input[1]);
	} else if(strcmp(input[0], "ls") == 0) {
		/* list files and directories locally */
		printf("performing - '%s'\n", input[0]);
		switch(fork()) {
			case -1:
				/* fork error */
				fprintf(stderr, "Fork: %s\n", strerror(errno));
				exit(1);
			case 0:
				/* child process */
				list_directory();
				break;
			default:
				/* parent process */
				wait(NULL);
		}
	} else if(strcmp(input[0], "put") == 0) {
		/* transfer file to the server's cwd */
		printf("performing - '%s'\n", input[0]);
		move_file(input[1], host, socketfd);
	} else {
		fprintf(stderr, "Command '%s' is unknown\n", input[0]);
	}

}

/* handles sending client requests to server */
void remote_commands(char **input, char *host, int socketfd) {

	if(strcmp(input[0], "rcd") == 0){
		/* change server directory*/
		printf("performing - '%s'\n", input[0]);
		if(input[1] == NULL) printf("Command error: expecting a pathname.\n");
		else remote_cd(host, input[1], socketfd);
	} else if(strcmp(input[0], "rls") == 0) {
		/* list server cwd */
		printf("performing - '%s'\n", input[0]);
		remote_ls(host, socketfd);
	} else if(strcmp(input[0], "get") == 0) {
		/* get file from server */
		printf("performing - '%s'\n", input[0]);
		get_file(host, input[1], socketfd);
	} else if(strcmp(input[0], "show") == 0)  {
		/* show file from server */
		printf("performing - '%s'\n", input[0]);
		show_file(host, input[1], socketfd);
	} else {
		fprintf(stderr, "Command '%s' is unknown - ignored\n", input[0]);
	}

}

/* establish connection with server */
int client_connect(char *port, char *host) {
	int socketfd, error;
	struct addrinfo hints, *actualdata;
	memset(&hints, 0, sizeof(hints));

	/* initialize connections to server */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if((error = getaddrinfo(host, port, &hints, &actualdata)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error)); exit(1);}

	if((socketfd = socket(actualdata->ai_family, actualdata->ai_socktype, 0)) < 0) {
		fprintf(stderr, "socket: %s\n", strerror(error)); exit(1);}

	if((connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen)) < 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno)); exit(1);}

	return socketfd;
}

/* client recieves server response to commands */
int server_response(int socketfd){
	char serv_resp[BUFF_SIZE]; char buffer;
	int i = 1; int error;

	/* read in data response from server */
	while((error = read(socketfd, &buffer, 1)) > 0) {
		if(error < 0) fprintf(stderr, "Read Error: %s\n", strerror(errno));
		if(buffer == '\n') break;
		serv_resp[i-1] = buffer; ++i;
	}
	serv_resp[i-1] = '\0';

	fprintf(stderr,"server response: %s\n", serv_resp);
	/* handle server error 'E' */
	if (serv_resp[0] == 'E') return -1;

	/* returns 0 on server success 'A' */
	return 0;
}

/* establish data connection to transfer data to server */
int data_connection(int socketfd, char *host, char *serv_cmd) {
	int datafd, error;
	char port_number[PORT_BUFF]; char buffer;
	char *file_name;

	/* send data request to server */
	write(socketfd, "D\n", 2);

	memset(port_number, 0, strlen(port_number));

	/* read in port number for data connection */
	int i = 0;
	while((error = read(socketfd, &buffer, 1)) > 0) {
		if(error < 0) fprintf(stderr, "Read Error: %s\n", strerror(errno));
		if(buffer == '\n') break;
		port_number[i] = buffer; ++i;
	}

	/* remove 'A' from server response */
	if(port_number[0] == 'A')
			memmove(port_number, port_number+1, strlen(port_number));
	else {
		fprintf(stderr, "%c: failed to retrieve port number\n", port_number[0]);
		return -1;
	}

	/* establish data_connection */
	datafd = client_connect(port_number, host);
	printf("data connection to port: %s\n", port_number);


	/* grab file name from path */
	if((file_name = strrchr(serv_cmd, '/')) != NULL)
		file_name[0] = serv_cmd[0];
	else file_name = serv_cmd;

	if(send_request(socketfd, file_name) == -1) return -1;
	if(server_response(socketfd) == -1) return -1;

	return datafd;
}
