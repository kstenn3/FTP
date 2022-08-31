HEADERS=mftp.h
CFLAGS=gcc -c -g

all: mftp mftpserve

mftp: mftp.o local_commands.o remote_commands.o

mftpserve: mftpserve.o server_functions.o

mftp.o: mftp.c $(HEADERS)
	$(CFLAGS) mftp.c

mftpserve.o: mftpserve.c $(HEADERS)
	$(CFLAGS) mftpserve.c

local_commands.o: local_commands.c $(HEADERS)
	$(CFLAGS) local_commands.c

remote_commands.o: remote_commands.c $(HEADERS)
	$(CFLAGS) remote_commands.c

server_functions.o: server_functions.c $(HEADERS)
	$(CFLAGS) server_functions.c

clean:
	rm -f *.o
