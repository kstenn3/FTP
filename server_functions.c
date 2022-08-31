/* file contains functions for server to handle client requests */
#include "mftp.h"


/* send 'E' with error statement to server */
void error_statement(int connectfd) {
	write(connectfd, "E", 1);
	write(connectfd, strerror(errno), strlen(strerror(errno)));
	write(connectfd, "\n", 1);
}

/* client exit function */
void exit_command(int connectfd) {
	write(connectfd, "A\n", 2);
	printf("%d: exiting.\n", getpid());
}

/* rls request - list server directories */
int rlist_directory(int connectfd, int datafd) {
	switch(fork()) {
		case -1:
			/* fork error */
			fprintf(stderr, "Server Fork Call: %s\n", strerror(errno));
			exit(1);
		case 0:
			/* execute execlp on sever side for rls */
			if(dup2(datafd, 1) < 0)
				fprintf(stderr, "Process Copy: %s\n", strerror(errno));

			if(close(datafd) < 0)
				fprintf(stderr, "Process Close: %s\n", strerror(errno));

			if(execlp("ls", "ls", "-l", (char *)0) < 0)
				fprintf(stderr, "Execute: %s\n", strerror(errno));
		default:
			wait(NULL);

	}
	/* send 'A' if process was a success */
	write(connectfd, "A\n", 2);
	close(datafd);
	return 0;
}

/* rcd request - change server directories */
int rchange_directory(int connectfd, char *dir) {
	if (chdir(dir + 1) < 0){
		/* send 'E' if error has occured */
		error_statement(connectfd);
		fprintf(stderr, "%d: %s \n", getpid(), strerror(errno));
		return -1;
	}

	/* send 'A' on success */
	write(connectfd, "A\n", 2);
	printf("%d: directory changed\n", getpid());
	return 0;
}

/* put request - move a file from client to server */
int rmove_file(int connectfd, int datafd, char *dir) {
	int filedes;
	char *path, *file, buffer;

	file = strtok(dir + 1, "/");
	do {
		path = file;
	} while ((file = strtok(NULL, "/")) != NULL);

	printf("%d: write file %s\n", getpid(), path);

	if((filedes = open(path, O_CREAT | O_EXCL | O_WRONLY , 0777)) < 0){
		error_statement(connectfd); return -1;
	}

	write(connectfd, "A\n", 2);
	while(read(datafd, &buffer, 1) > 0){
		write(filedes, &buffer, 1);
	}
	printf("%d: received file %s\n", getpid(), file);

	close(datafd); close(filedes);

	return 0;
}

/* get request - move file from server to client */
int rget_file(int connectfd, int datafd, char *dir){
	int error, filedes;
	struct stat sb;
	char *buffer;

	printf("%d: Reading file %s\n", getpid(), (dir+1));

	if((filedes = open(dir + 1, O_RDONLY )) < 0){
		error_statement(connectfd);
		return -1;
	}

	/* check file permissions */
	if(lstat((dir + 1), &sb) == 0){
		if (S_ISDIR (sb.st_mode)){
			write(connectfd, "E", 1);
			write(connectfd, "directory", 9);
			write(connectfd, "\n", 1);
			return -1;
		}

		write(connectfd, "A\n", 2);

		while((error = read(filedes, &buffer, 1)) > 0){
			if(error < 0){
				fprintf(stderr, "File Read: %s\n", strerror(errno)); return -1;
			}
			if(write(datafd, &buffer, 1) < 0){
				fprintf(stderr, "File Write: %s\n", strerror(errno)); return -1;
			}
		}

	}

	printf("%d: show/get\n", getpid());
	close(datafd); close(filedes);

	return 0;
}
