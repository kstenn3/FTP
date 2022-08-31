#include "mftp.h"

/* close client */
void exit_command(char **input, int socketfd) {
	printf("exiting program\n");
	write(socketfd, "Q\n", 2);

	close(socketfd);
	exit(0);
}

/* cd functionality */
int change_directory(char *pathname) {
	if(pathname == NULL) {
		printf("command error: expecting a pathname.\n"); return -1;
	}
	/* change directory locally */
	if(chdir(pathname) < 0)
		fprintf(stderr, "change directory: %s\n", strerror(errno));
	return 0;
}

/* ls functionality */
int list_directory(){
	int filedes[2];
	/* create pipe for ls -l and more command*/
	if(pipe(filedes) == -1) {
		fprintf(stderr, "Pipe Call: %s\n", strerror(errno)); return -1;
	}

	switch(fork()){
		case -1:
			/* fork error */
			fprintf(stderr, "Fork Call: %s\n", strerror(errno));
			exit(1);
		case 0:
			/* child: lists files and directories of cwd */

			/* close read end of pipe */
			 if(close(filedes[0]) == -1)
				 fprintf(stderr, "Pipe Close: %s\n", strerror(errno));

			/* copy write filedes to stdout */
			if(dup2(filedes[1], 1) == -1)
				fprintf(stderr, "Process Copy: %s\n", strerror(errno));

			/* execute ls -l command */
			if(execlp("ls", "ls", "-l", (char *)0) < 0)
				fprintf(stderr, "Execute File: %s\n", strerror(errno));
			break;
		default:
			/* parent: executes more command */
			if(close(filedes[1]) == -1)
				fprintf(stderr, "Pipe Close: %s\n", strerror(errno));

			/* copy read to stdin */
			if(dup2(filedes[0], 0) == -1)
				fprintf(stderr, "Process Copy: %s\n", strerror(errno));

			/* execute more command */
			if(execlp("more", "more", LINE_QTY, (char *)0) < 0)
				fprintf(stderr, "Execute: %s\n", strerror(errno));
	}

}

/* put functionality */
int move_file(char *path_name, char *host, int socketfd) {
	int fd, datafd, error;
	char serv_cmd[BUFF_SIZE]; char buffer;

	if (path_name == NULL){
		printf("Error: expecting a pathname.\n"); return -1;
	}

	memset(serv_cmd, 0, strlen(serv_cmd));
	strcpy(serv_cmd, "P"); strcat(serv_cmd, path_name); strcat(serv_cmd, "\n");


	/* open check file perms and open file */
	if(file_perms(path_name) == -1) return -1;

	/* open file in local cwd to be sent to server */
	if((fd = open(path_name, O_RDONLY)) == -1) {
		fprintf(stderr,"Open file '%s': %s\n", path_name, strerror(errno)); return -1;
	}

	/* make data connection with server */
	if((datafd = data_connection(socketfd, host, serv_cmd)) == -1) return -1;

	/* send file to server */
	if(transfer_data(datafd, fd) != -1) return -1;

	return 0;

}

/* check file permissions, returns -1 for error, fd on success */
int file_perms(char *path_name) {
	int fd; struct stat sb;

	/* check for the following: exists, read permissions, regular file */
	if(access(path_name, F_OK) != 0) {
		fprintf(stderr, "'%s' pathname does not exist\n", path_name); return -1;
	} else if(access(path_name, R_OK) != 0) {
		fprintf(stderr, "'%s' pathname is not readable\n", path_name); return -1;
	}
	 else if(lstat(path_name, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "Local file '%s' is a directory, command ignored\n", path_name);
		return -1;
	}
	return fd;
}

/* read/write systems calls used to transfer data between client and server */
int transfer_data(int datafd, int fd) {
	int error; char buffer;

	while((error = read(fd, &buffer, 1)) > 0 ){
		if(error < 0){
			fprintf(stderr, "File Write: %s\n", strerror(errno)); return -1;
		}
		if(write(datafd, &buffer, 1) < 0 ){
			fprintf(stderr, "File Write: %s\n", strerror(errno)); return -1;
		}
	}
	close(fd); close(datafd);
	return 0;
}

/* send client request to server using write system call */
int send_request(int fd, char *cmd) {
	if(write(fd, cmd, strlen(cmd)) < 0 ){
		fprintf(stderr, "Request Error: '%s'\n", strerror(errno)); exit(1);
	}

	return 0;
}
