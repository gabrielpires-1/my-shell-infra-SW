all: myShell.c
	gcc myShell.c -o shell -pthread
clean:
	rm shell