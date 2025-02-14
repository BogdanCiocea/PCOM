#include "common.h"

#define NUMBER_OF_FDS 2

int main(int argc, char *argv[])
{
	if (argc != 4)
		DIE(1, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>");

	/**
	 * "I will do what I must"
	 * 	- Obi Wan Kenobi from Star Wars Episode III: Revenge of the Sith
	 */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	char buffer[MSG_MAXSIZE];
	struct sockaddr_in server_addr;
	int tcp_socket_fd;

	/**
	 * Open socket.
	 */
	tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket_fd < 0, "socket");
	int flag = 1;
	setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY,
				(char *)&flag, sizeof(int));

	/**
	 * Setup the sockaddr_in structure.
	 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[3]));
	server_addr.sin_addr.s_addr = inet_addr(argv[2]);

	/**
	 * Try to connect to the server.
	 */
	int rc = connect(tcp_socket_fd, (struct sockaddr *)&server_addr,
						sizeof(server_addr));
	DIE(rc < 0, "connect");

	/**
	 * Send the ID of the client.
	 */
	char id_client[10];
	strcpy(id_client, argv[1]);

	/**
	 * Give the ID to the server
	 */
	rc = send_all(tcp_socket_fd, id_client, strlen(id_client));
	DIE(rc < 0, "send");

	struct pollfd *fds = malloc(NUMBER_OF_FDS * sizeof(struct pollfd));
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds[1].fd = tcp_socket_fd;
	fds[1].events = POLLIN;

	while (1)
	{
		poll(fds, NUMBER_OF_FDS, -1);
		/**
		 * Stdin commands
		 */
		if (fds[0].revents & POLLIN)
		{
			/**
			 * If the command is "exit" or if the stdin has
			 * only the ENTER for easy disconnect, close the connection.
			*/
			char *command = malloc(MSG_MAXSIZE);
			fgets(command, MSG_MAXSIZE, stdin);
			if (command == NULL || strlen(command) == 0
				|| !strcmp(command, "\n")
				|| !strcmp(command, "exit\n"))
			{	
				send_all(tcp_socket_fd, "exit\n", strlen("exit\n"));
				close(tcp_socket_fd);
				return 0;
			}
			/**
			 * If the command is subscribe or unsubscribe,
			 * send it to the server.
			*/
			else if (strncmp(command, "subscribe ", 9) == 0)
			{
				char *topic = command + 10;
				topic[strcspn(topic, "\n")] = 0;
				rc = send_all(tcp_socket_fd, command, strlen(command));
				DIE(rc < 0, "send");
				continue;
			}
			else if (strncmp(command, "unsubscribe ", 11) == 0)
			{
				char *topic = command + 12;
				topic[strcspn(topic, "\n")] = 0;
				rc = send_all(tcp_socket_fd, command, strlen(command));
				DIE(rc < 0, "send");
				continue;
			}
			/**
			 * Debug command.
			*/
			else if (!strcmp(command, "show\n"))
			{
				rc = send_all(tcp_socket_fd, command, strlen(command));
				DIE(rc < 0, "send");
			}
			/**
			 * Invalid command.
			*/
			else
			{
				printf("Invalid command\n");
			}
		}
		else if (fds[1].revents & POLLIN)
		{
			/**
			 * Server response
			 */
			int rc = read(tcp_socket_fd, buffer, MSG_MAXSIZE);
			/**
			 * If there is no message from the server, close the connection.
			*/
			if (rc == 0)
			{
				close(tcp_socket_fd);
				DIE(1, "recv");
			}
			/**
			 * Messages from server if the client is already connected.
			*/
			else if (strstr(buffer, "exit duplicate"))
			{
				close(tcp_socket_fd);
				DIE(1, "Client already connected");
			}
			/**
			 * Message from server if it is shutting down.
			*/
			else if (strstr(buffer, "exit server"))
			{
				printf("Server is shutting down\n");
				close(tcp_socket_fd);
				return -1;
			}
			/**
			 * Messages from subscriptions.
			*/
			else if (strstr(buffer, "Subscribed")
					 || strstr(buffer, "Unsubscribed")
					 || strstr(buffer, "Already")
					 || strstr(buffer, "Topic not found"))
			{
				buffer[rc] = '\0';
				printf("%s\n", buffer);
			}
			else
			{
				/**
				 * UDP message.
				 *
				 * Just print the message.
				 */
				buffer[rc] = '\0';
				printf("%s", buffer);
			}

			/**
			 * Clear the buffer because it is used for the next message.
			 */
			memset(buffer, 0, MSG_MAXSIZE);
		}
	}

	/**
	 * Close the socket.
	 */
	close(tcp_socket_fd);

	return 0;
}
