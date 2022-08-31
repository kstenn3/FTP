#include "mftp.h"

/* client request to change server's directory */
int remote_cd(char *host, char *path_name, int socketfd) {
	char serv_cmd[BUFF_SIZE];

	/* change directory remotely */
	memset(serv_cmd, 0, strlen(serv_cmd));
	strcpy(serv_cmd, "C");
	strcat(serv_cmd, path_name); strcat(serv_cmd, "\n");

	/* send directory change request to server */
	send_request(socketfd, serv_cmd);

	/* verify request was successful */
	if(server_response(socketfd) < 0) return -1;

	return 0;
}

/* client request to list server's cwd */
void remote_ls(char *host, int socketfd) {
	/* create data connection to server */
	int datafd;

	datafd = data_connection(socketfd, host, "L\n");
	/* Once connected, send request */
	/* send directory change request to server */
	send_request(datafd, "L\n");

	/* execute ls -l and more on server */
	switch(fork()){
		case -1:
			/* fork error */
			fprintf(stderr, "Fork Call: %s\n", strerror(errno));
			exit(1);
		case 0:
			/* child process */
			if(close(socketfd) == -1)
				fprintf(stderr, "Pipe Close: %s\n", strerror(errno));

			/* copy read to stdin */
			if(dup2(datafd, 0) == -1)
				fprintf(stderr, "Process Copy: %s\n", strerror(errno));

			/* execute more command */
			if(execlp("more", "more", LINE_QTY, (char *)0) < 0)
				fprintf(stderr, "Execute File: %s\n", strerror(errno));
			break;
		default:
			/* parent process */
			wait(NULL);
	}
	close(datafd);
}


/* execute get function. grabs a file from server and places on client side */
int get_file(char *host, char *path_name, int socketfd) {
	int fd, datafd, error;
	char serv_cmd[BUFF_SIZE]; char buffer;

	if (path_name == NULL){
		printf("Error: expecting a pathname.\n"); return -1;
	}

	memset(serv_cmd, 0, strlen(serv_cmd));
	strcpy(serv_cmd, "G"); strcat(serv_cmd, path_name); strcat(serv_cmd, "\n");

	/* create data connection */
	if((datafd = data_connection(socketfd, host, serv_cmd)) == -1) return -1;

	/* make file on client side */
	if((fd = open(path_name, O_CREAT | O_EXCL | O_WRONLY, 0777)) < 0){
		fprintf(stderr, "opening error '%s': %s\n", path_name, strerror(errno));
		return -1;
	}
	transfer_data(fd, datafd);

	close(datafd); close(fd);
	return 0;
}


/* show contents from file on server. works like 'cat' */
int show_file(char *host, char *path_name, int socketfd) {
	int datafd;
	char serv_cmd[BUFF_SIZE];

	if (path_name == NULL){
		printf("Error: expecting a pathname.\n"); return -1;
	}

	memset(serv_cmd, 0, strlen(serv_cmd));
	strcpy(serv_cmd, "G"); strcat(serv_cmd, path_name); strcat(serv_cmd, "\n");

	datafd = data_connection(socketfd, host, serv_cmd);

	write(datafd, serv_cmd, strlen(serv_cmd));

	/* execute more on file */
	switch(fork()) {
		case -1:
			/* fork error */
			fprintf(stderr, "Fork: %s\n", strerror(errno));
			exit(1);
		case 0:
			dup2(datafd, 0);
			close(datafd);
			execlp("more", "more", "-20", (char *) 0);
		default:
			/* parent process */
			wait(NULL);
	}
	close(datafd);
	return 0;
}
