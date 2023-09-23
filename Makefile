all: myShell.c
	gcc myShell.c -o shell -pthread
	./shell
clean:
	rm shell