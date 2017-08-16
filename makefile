project=cute
CC=gcc
target:
	$(CC) server.c cJSON.c -lpthread -lm -o $(project)

clean:
	rm -f $(project)
