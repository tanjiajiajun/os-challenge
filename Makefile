all:  multiple 

multiple:
	gcc -o server final/final_memory.c -lssl -lcrypto -lpthread

recieve: server
	./server 5003

clean:
	rm server 
	clear
    
format:
	clang-format -i -style=file *.{c,h}
