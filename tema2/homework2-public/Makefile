all: server subscriber

server: server.c common.c
	gcc -g -Wall -Wextra -o server server.c common.c -lm

subscriber: subscriber.c common.c
	gcc -g -Wall -Wextra -o subscriber subscriber.c common.c -lm

clean:
	rm -rf subscriber server