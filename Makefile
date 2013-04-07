all:
	$(CC) main.c ihex.c -o ihextest

clean:
	rm -f *.o ihextest
