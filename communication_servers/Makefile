all: client multiple

multiple:
	gcc -o server multiple_server_socket.c -lssl -lcrypto

client:
	gcc client_socket.c -o client

server:
	gcc server_socket.c -o server

clean:
	rm client server

format: 
	clang-format -i -style=file *.{c,h}
