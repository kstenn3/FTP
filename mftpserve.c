#include "mftp.h"
void exit_command(int);
int rchange_directory(int, char*);
int rlist_directory(int, int);
int rmove_file(int, int, char*);
int rget_file(int, int, char*);
void error_statement(int);


int main(int argc, char const *argv[]) {
	int listenfd, connectfd, datafd, dataconnfd;
	struct sockaddr_in serv_addr, client_addr, dataserv_addr, new_addr;;
	int length = sizeof(struct sockaddr_in); int port = 0;
	struct hostent* host_entry;
	char* host_name; char buff[PORT_BUFF];
	char input, buffer, client_request[BUFF_SIZE];

	/* socket setup */
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {perror("socket"); exit(1);}

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT_NUMBER);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	length = sizeof(struct sockaddr_in);

	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ){
		perror("bind"); exit(1);
	}

	listen(listenfd, 4);

	/* wait for requests from client */
	while(1){

		connectfd = accept(listenfd, (struct sockaddr *)&client_addr, &length);

		/* accept client requests */
		switch(fork()) {
			case -1:
				/* fork error when establish connection */
				fprintf(stderr, "Fork Call: %s\n", strerror(errno)); exit(1);
			case 0:
				/* recieve client requests */
				host_entry = gethostbyaddr(&(client_addr.sin_addr),
					sizeof(struct in_addr), AF_INET);
				host_name = host_entry->h_name;

				printf("Connected to %d\n", getpid());
				while (1){
					int i = 0;
					while (i += read(connectfd, &input, 1) == 1){
						if (input == '\n') break;
						client_request[i-1] = input;
				 	}
					client_request[i-1] = '\0';

					char request = client_request[0];
					if(request == 'D'){
						/* dataconnection to transfer data to client */
						if((dataconnfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {perror("socket"); exit(1);}

						memset(&dataserv_addr, 0, sizeof(dataserv_addr));
						dataserv_addr.sin_family = AF_INET;
						dataserv_addr.sin_port = htons(0);
						dataserv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

						if(bind(dataconnfd, (struct sockaddr *)&dataserv_addr, sizeof(dataserv_addr)) < 0){
							perror("bind"); exit(1);
						}

						/* allow up to 4 clients to connect */
						listen(dataconnfd, 4);

						memset(&new_addr, 0, sizeof(new_addr));
						length = sizeof(struct sockaddr_in);
						getsockname(dataconnfd, (struct sockaddr *) &new_addr, &length);
						port = ntohs(new_addr.sin_port);

						sprintf(buff, "A%d\n", port);
						write(connectfd, buff, 7);
					}
					if(request == 'Q'){
						/* client disconnect from server */
						exit_command(connectfd);  break;

					}
					if(request == 'P'){
						/* recieve file from client and place on server */
						datafd = accept(dataconnfd, (struct sockaddr *) &client_addr, &length);
						rmove_file(connectfd, datafd, client_request);
					}
					if(request == 'G'){
						/* show contents of file on server or send file from server to client */
						datafd = accept(dataconnfd, (struct sockaddr *) &client_addr, &length);
						rget_file(connectfd, datafd, client_request);
					}
					if(request == 'L'){
						/* list files and directory */
						datafd = accept(dataconnfd, (struct sockaddr *) &client_addr, &length);
						rlist_directory(connectfd, datafd);
					}
					if(request == 'C'){
						/* change directory on server request */
						rchange_directory(connectfd, client_request);
					}
				}

				close(listenfd); close(connectfd);
				return 0;

			default:
				waitpid(-1, NULL, WNOHANG);
				close(connectfd);
		}

	}
	return 0;
}
