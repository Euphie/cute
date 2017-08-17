project=cute
CC=gcc
target:
	$(CC) server.c cJSON.c -lpthread -lm -o bin/$(project)
clean:
	rm -f bin/$(project)
