default: server.c client.c
	gcc server.c -o server -pthread -lmcrypt
	gcc client.c -o client -pthread -lmcrypt
client: client.c
	gcc client.c -o client -pthread -lmcrypt
server: server.c
	gcc server.c -o server -pthread -lmcrypt
clean:
	rm server client
dist:
	tar -czvf lab1b-704534585.tar.gz server.c client.c my.key README makefile


