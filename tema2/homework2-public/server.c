#include "common.h"

/**
 * Function to add a new client to the list of clients.
 * @param id - the id of the client
 * @param socket - the socket of the client
 * @param addr - the address of the client
 * @param clients - the list of clients
 *
 * @return 1 if the client is new, 2 if the client is reconnected,
 * 		  -1 if the client is already connected
 * 		   2 if the client is reconnected
 */
int add_client(char id[], int socket, struct sockaddr_in addr,
			   Client **clients)
{
	int flag = 1;
	for (Client *entry = *clients; entry; entry = entry->next)
	{
		/**
		 * Check if the client is already on the list and is connected
		 */
		if (!strcmp(id, entry->id) && entry->connected)
		{
			return -1;
		}
		/**
		 * Check if the client is on the list but has previously disconnected,
		 * we will reconnect the client
		 */
		else if (!strcmp(id, entry->id) && !entry->connected)
		{
			entry->socket = socket;
			entry->addr = addr;
			entry->connected = true;

			/**
			 * Change the socket option to disable Nagle's algorithm
			 */
			setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
			return 2;
		}
	}

	/**
	 * Add a new client to the list allocating memory for it
	 */
	Client *new_client = (Client *)malloc(sizeof(Client));
	DIE(!new_client, "Memory allocation failed for new client");
	/**
	 * Memory initialization
	 */
	memset(new_client, 0, sizeof(Client));
	strcpy(new_client->id, id);

	new_client->socket = socket;
	new_client->addr = addr;
	new_client->connected = true;
	new_client->next = *clients;

	/**
	 * Set the socket option to disable Nagle's algorithm
	 */
	setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
	*clients = new_client;
	return 1;
}

/**
 * Function to "remove" a client from the list of clients.
 * We won't actually remove the client, we will just mark it as disconnected.
 *
 * @param client - the client to be "removed"
 */
void remove_client(Client *client)
{
	client->connected = false;
	close(client->socket);
	printf("Client %s disconnected.\n", client->id);
}

/**
 * Function which will actually remove the clients from the list and
 * free the memory.
 *
 * @param clients - the list of clients
 */
void free_client_list(Client **clients)
{
	Client *entry = *clients;
	while (entry)
	{
		Client *next = entry->next;
		if (entry->connected)
		{
			/**
			 * Send a message to the client to notify it that the server
			 * is shutting down.
			 */
			send_all(entry->socket, "exit server", 11);
			close(entry->socket);
		}
		free(entry);
		entry = next;
	}
	*clients = NULL;
}

/**
 * Function which will send a message to all the clients in the list.
 *
 * @param message - the message to be sent
 * @param clients - the list of clients
 */
void notify_all_clients(char *message, Client *clients)
{
	for (Client *entry = clients; entry; entry = entry->next)
	{
		if (entry->connected)
		{
			send_all(entry->socket, message, strlen(message));
		}
	}
}

/**
 * For not having to write the same code copy paste, I will use this function
 * to convert the type of the message to a string for printing.
 *
 * @param type - the type of the message in integer format
 */
char *convert_type(int type)
{
	switch (type)
	{
	case 0:
		return "INT";
	case 1:
		return "SHORT_REAL";
	case 2:
		return "FLOAT";
	case 3:
		return "STRING";
	default:
		return "UNKNOWN";
	}
}

/**
 * Same principle, but in reverse.
 *
 * @param type - the type of the message in string format
 */
int reverse_convert_type(char *type)
{
	if (!strcmp(type, "INT"))
	{
		return 0;
	}
	else if (!strcmp(type, "SHORT_REAL"))
	{
		return 1;
	}
	else if (!strcmp(type, "FLOAT"))
	{
		return 2;
	}
	else if (!strcmp(type, "STRING"))
	{
		return 3;
	}

	return -1;
}

/**
 * Function to check if the topic matches the subscribed topic.
 *
 * @param subscribed_topic - the subscribed topic
 * @param incoming_topic - the incoming topic
 *
 * @return true if the topics match, false otherwise
 */
bool match_topic(char *subscribed_topic, char *incoming_topic)
{
	char *sub = subscribed_topic;
	char *inc = incoming_topic;
	char *next;

	/**
	 * While we have not reached the end of the subscribed topic.
	 */
	while (*sub != '\0')
	{
		/**
		 * Get the next symbol of wildcard.
		 */
		for (next = sub; *next && *next != '+' && *next != '*'; next++)
		{
			/**
			 * If we reached the end of the subscribed topic,
			 * we will check if the incoming topic is also at the end.
			 */
			if (!next)
			{
				return !strcmp(sub, inc);
			}
		}

		/**
		 * Get the length of the segment and check if it matches the
		 * incoming topic.
		 */
		int segment_length = next - sub;
		if (strncmp(sub, inc, segment_length))
		{
			return false;
		}

		/**
		 * Move the pointers to the next segment.
		 */
		sub = next + 1;
		inc += segment_length;

		/**
		 * If the wildcard symbol is plus, we will skip the segment until
		 * we have the next one.
		 */
		if (*next == '+')
		{
			for (; *inc && *inc != '/'; inc++)
				;
		}
		/**
		 * If the wildcard symbol is star, we will check if the rest of the
		 * subscribed topic matches the rest of the incoming topic.
		 */
		else if (*next == '*')
		{
			while (*inc)
			{
				/**
				 * Check recursively if the rest of the subscribed topic
				 * matches the rest of the incoming topic.
				 */
				if (match_topic(sub, inc))
				{
					return true;
				}
				inc++;
			}
		}
	}

	/**
	 * If we reached the end of the subscribed topic, we will check if the
	 * incoming topic is also at the end.
	 */
	if (*inc == '\0')
		return true;
	else
		return false;
}

/**
 * Function to process the UDP message.
 *
 * @param udp_socket - the UDP socket
 * @param udp_addr - the address of the UDP client
 * @param udp_len - the length of the UDP address
 * @param clients - the list of clients
 * @param port - the port of the server
 */
void process_udp_message(int udp_socket, struct sockaddr_in udp_addr,
						 socklen_t udp_len, Client *clients, int port)
{
	/**
	 * Initialize the UDP address and populate it.
	 */
	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port);

	/**
	 * Get the data from the UDP socket and store it in a buffer.
	 */
	char buffer[MSG_MAXSIZE];
	int msg_len = recvfrom(udp_socket, buffer, MSG_MAXSIZE, MSG_WAITALL,
						   (struct sockaddr *)&udp_addr, &udp_len);

	/**
	 * If the message has been received successfully, we will process it as the
	 * name of the function suggests.
	 */
	if (msg_len > 0)
	{
		/**
		 * Set the end of the message to null character.
		 */
		buffer[msg_len] = '\0';
		/**
		 * Get the topic from the message.
		 */
		char topic[TOPIC_SIZE];
		memcpy(topic, buffer, TOPIC_SIZE);
		topic[TOPIC_SIZE] = '\0';

		/**
		 * Get the type of the message and convert it properly.
		 */
		char *type_str = (char *)malloc(10);
		strcpy(type_str, convert_type((uint8_t)buffer[TOPIC_SIZE]));

		/**
		 * Declare the full message which will be sent to the clients.
		 */
		char full_message[MSG_MAXSIZE];

		/**
		 * Now check the type of the message and convert it properly.
		 */
		if (!strcmp(type_str, "INT"))
		{
			/**
			 * Get the sign of the integer.
			 */
			uint8_t sign = *(uint8_t *)(buffer + 51);
			/**
			 * Get the value of the integer.
			 */
			int32_t value = ntohl(*(int32_t *)(buffer + 52));

			if (sign == 1)
				value = -value;

			/**
			 * Paste the processed message in full_message.
			 */
			sprintf(full_message, "%s - %s - %d", topic, type_str, value);
		}
		else if (!strcmp(type_str, "SHORT_REAL"))
		{
			/**
			 * Get the value of the short real.
			 */
			uint16_t value = ntohs(*(uint16_t *)(buffer + 51));
			/**
			 * Convert the value to a real number.
			 */
			float real_value = (float)value / 100;
			/**
			 * Paste the processed message in full_message.
			 */
			sprintf(full_message, "%s - %s - %.2f", topic, type_str,
					real_value);
		}
		else if (!strcmp(type_str, "FLOAT"))
		{
			/**
			 * Get the sign of the float.
			 */
			uint8_t sign = *(uint8_t *)(buffer + 51);

			/**
			 * Get the value of the float.
			 */
			uint32_t value = ntohl(*(uint32_t *)(buffer + 52));

			/**
			 * Get the power of the float.
			 */
			uint8_t power = *(uint8_t *)(buffer + 56);

			float real_value = (float)value;

			/**
			 * Convert the value to a real number.
			 */
			real_value /= pow(10, power);

			if (sign == 1)
				real_value = -real_value;

			/**
			 * Paste the processed message in full_message.
			 */
			sprintf(full_message, "%s - %s - %.4f", topic, type_str,
					real_value);
		}
		else if (!strcmp(type_str, "STRING"))
		{
			/**
			 * Get the content string.
			 */
			char *value = buffer + 51;
			/**
			 * Paste the processed message in full_message.
			 */
			sprintf(full_message, "%s - %s - %s", topic, type_str, value);
		}
		/**
		 * Add a newline at the end of the message because I forgot to add it
		 * and I got jumpscared at the checker error.
		 */
		strcat(full_message, "\n");

		/**
		 * Send the processed message to all the clients who are subscribed to
		 * the topic.
		 */
		for (Client *client = clients; client != NULL; client = client->next)
		{
			if (client->connected)
			{
				for (Topic *t = client->subscribed; t; t = t->next)
				{
					/**
					 * If the topic in question matches with the
					 * subscribed topic.
					 */
					if (match_topic(t->name, topic))
					{
						/**
						 * Send a message to the client and stop.
						 */
						send_all(client->socket, full_message,
								 strlen(full_message));
						break;
					}
				}
			}
		}
	}
	else
	{
		/**
		 * If the message has not been received successfully, print an error.
		 */
		perror("Failed to receive message");
	}
}

/**
 * Function to subscribe to a topic.
 *
 * @param command - the command to subscribe
 * @param client - the client to subscribe
 *
 * @return 0 if the subscription was successful
 * 		   -1 if the client is already subscribed to the topic
 * 		   -2 if the topic is not found
 * 		   DIE if memory allocation failed
 */
int subscribe(char *command, Client *client)
{
	Topic *new_topic = (Topic *)malloc(sizeof(Topic));
	DIE(!new_topic, "Memory allocation failed for new topic");
	memset(new_topic, 0, sizeof(Topic));

	char *tok = strtok(command, " ");
	/**
	 * Get the topic name.
	 */
	tok = strtok(NULL, " ");
	if (tok)
	{
		/**
		 * Copy the topic name to the new_topic.
		 */
		strncpy(new_topic->name, tok, sizeof(new_topic->name) - 1);
		tok = strtok(NULL, " ");
		/**
		 * Get the type of the topic.
		 */
		if (tok)
		{
			new_topic->type = reverse_convert_type(tok);
			/**
			 * Get the content.
			 */
			tok = strtok(NULL, "");
			if (tok)
			{
				strncpy(new_topic->content, tok, sizeof(new_topic->content) - 1);
			}
		}
	}
	else
	{
		free(new_topic);
		return -2;
	}

	/**
	 * Check if the client is already subscribed to the topic.
	 */
	for (Topic *topic = client->subscribed; topic; topic = topic->next)
	{
		if (!strcmp(topic->name, new_topic->name))
		{
			return -1;
		}
	}

	/**
	 * Check if the topic has a wildcard.
	 */
	if (strchr(new_topic->name, '+') || strchr(new_topic->name, '*'))
	{
		new_topic->has_wildcard = true;
	}

	/**
	 * Add the new topic as the head of the list because it was much
	 * simpler and quicker than putting it at the end of the list without
	 * last element pointer which could have increased the storage for the
	 * "Topic" struct.
	 */
	new_topic->next = client->subscribed;
	client->subscribed = new_topic;

	/**
	 * Send an ACK message to the client.
	 */
	char ack[TOPIC_SIZE + 24];
	sprintf(ack, "Subscribed to topic. %s", new_topic->name);
	send_all(client->socket, ack, strlen(ack));
	return 0;
}

/**
 * Function to unsubscribe from a topic.
 *
 * @param topic - the topic to unsubscribe from
 * @param client - the client to unsubscribe
 *
 * @return 0 if the unsubscription was successful
 * 		  -1 if the topic was not found
 *
 */
int unsubscribe(char *topic, Client *client)
{
	Topic *prev = NULL;
	Topic *current = client->subscribed;

	bool found = false;

	char *tok = strtok(topic, " ");
	/**
	 * Get the topic name.
	 */
	tok = strtok(NULL, " ");

	while (current != NULL)
	{
		/**
		 * Check if the topic is found.
		 */
		if (!strcmp(current->name, tok))
		{
			found = true;
			break;
		}

		prev = current;
		current = current->next;
	}

	/**
	 * If we have reached the end of the list and the topic was still not
	 * found, we will send an error message to the client.
	 */
	if (current == NULL && !found)
	{
		char ack[TOPIC_SIZE + 17];
		sprintf(ack, "Topic not found.");
		send_all(client->socket, ack, strlen(ack));
		return -1;
	}

	/**
	 * If the topic was found, we will remove it from the list and free
	 * the memory.
	 */
	if (prev == NULL)
	{
		client->subscribed = current->next;
	}
	else
	{
		prev->next = current->next;
	}

	free(current);

	/**
	 * Send an ACK message to the client which says that the client has
	 * unsubscribed from the topic successfully.
	 */
	char ack[28 + TOPIC_SIZE];
	sprintf(ack, "Unsubscribed from topic. %s", tok);
	send_all(client->socket, ack, strlen(ack));
	return 0;
}

/**
 * Debugging function to show that the client has indeed
 * subscribed/unsubscribed to/from the topics.
 *
 * @param client - the client to show the topics for
 */
void show_topics(Client *client)
{
	for (Topic *topic = client->subscribed; topic; topic = topic->next)
	{
		char *type = convert_type(topic->type);
		printf("[%s - %s - %s]\n", topic->name, type, topic->content);
	}
}

/**
 * Function to accept a TCP connection on the server.
 *
 * @param server_fd - the server file descriptor
 * @param client_addr - the address of the client
 *
 * @return the file descriptor of the client
 * 		   or -1 if the connection failed
 */
int accept_connection(int server_fd, struct sockaddr_in *client_addr)
{
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int client_fd = accept(server_fd, (struct sockaddr *)client_addr,
						   &addr_size);
	if (client_fd == -1)
	{
		perror("Accept failed");
		return -1;
	}
	return client_fd;
}

/**
 * Function to get the client by the file descriptor.
 *
 * @param fd - the file descriptor of the client
 * @param clients - the list of clients
 *
 * @return the pointer to the client with that file descriptor
 * 	   	   or NULL if the client is not found
 */
Client *get_client_by_fd(int fd, Client *clients)
{
	for (Client *entry = clients; entry; entry = entry->next)
	{
		if (entry->socket == fd)
		{
			return entry;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		DIE(1, "Usage: ./server <port>");

	/**
	 * I do as the crystal guides. - Morty Smith from Rick and Morty
	 */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	/**
	 * Get the port from the command line arguments.
	 */
	int port = atoi(argv[1]);

	/**
	 * Initialize the list of clients.
	 */
	Client *clients = NULL;

	/**
	 * Create the TCP and UDP sockets.
	 */
	int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket < 0, "Failed to create TCP socket");

	int udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_socket < 0, "Failed to create UDP socket");

	/**
	 * Set the socket options for the UDP socket and server address.
	 */
	struct sockaddr_in server_addr, udp_addr, tcp_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	/**
	 * Bind the TCP and UDP sockets to the server address.
	 */
	DIE(bind(tcp_socket, (struct sockaddr *)&server_addr,
			 sizeof(server_addr)) < 0,
		"bind TCP");
	DIE(bind(udp_socket, (struct sockaddr *)&server_addr,
			 sizeof(server_addr)) < 0,
		"bind UDP");

	/**
	 * Listen for incoming connections on the TCP socket.
	 */
	DIE(listen(tcp_socket, MAX_CLIENTS) < 0, "listen");

	/**
	 * Initialize the pollfd structure for the TCP, UDP and
	 * STDIN file descriptors.
	 */
	struct pollfd *fds = malloc(POLL_SIZE * sizeof(struct pollfd));
	memset(fds, 0, sizeof(*fds));

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds[1].fd = tcp_socket;
	fds[1].events = POLLIN;

	fds[2].fd = udp_socket;
	fds[2].events = POLLIN;

	int nfds = 3;

	forever
	{
		/**
		 * Poll the file descriptors.
		 */
		int rc = poll(fds, nfds, -1);
		DIE(rc < 0, "Failed to poll");

		/**
		 * Search for the file descriptor which has events.
		 */
		for (int i = 0; i < nfds; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				/**
				 * Events on the STDIN file descriptor.
				 * The only one being if the server is shutting down by command
				 * or by pressing ENTER on the server terminal.
				 *
				 * This seems pretty intuitive to me, so I will not explain it.
				 * It has nothing to do with the homework, just an easier
				 * way to shut down the server. :)
				 */
				if (fds[i].fd == STDIN_FILENO)
				{
					char cmd[10];
					if (fgets(cmd, 10, stdin) && ((!strcmp(cmd, "exit\n")) || !strcmp(cmd, "\n")))
					{
						printf("Shutting down server\n");
						notify_all_clients("exit server", clients);
						free_client_list(&clients);

						for (int j = 3; j < nfds; j++)
						{
							if (fds[j].fd != STDIN_FILENO)
							{
								close(fds[j].fd);
							}
						}

						close(fds[0].fd);
						close(fds[1].fd);
						close(fds[2].fd);
						free(fds);
						return 0;
					}
				}
				/**
				 * Events on the TCP socket file descriptor.
				 */
				else if (fds[i].fd == tcp_socket)
				{
					/**
					 * Accept the connection and get the client address.
					 */
					int client_socket = accept_connection(tcp_socket,
														  &tcp_addr);
					DIE(client_socket < 0, "Failed to accept connection");

					/**
					 * Get the client ID.
					 */
					char id[11];
					memset(id, 0, 11);

					/**
					 * Nu intreba de ce merge cu + 2. Daca merge, merge
					 * si nu te atingi de el.
					 */
					rc = recv_all(client_socket, id, strlen(id) + 2);
					DIE(rc < 0, "Failed to receive client ID");

					/**
					 * Try to add the client and treat the cases accordingly.
					 */
					int status = add_client(id, client_socket, tcp_addr,
											&clients);
					if (status == -1)
					{
						printf("Client %s already connected.\n", id);
						send_all(client_socket, "exit duplicate\n", 14);
						close(client_socket);
					}
					else if (status == 1)
					{
						printf("New client %s connected from %s:%d\n", id,
							   inet_ntoa(tcp_addr.sin_addr),
							   ntohs(tcp_addr.sin_port));
						fds[nfds].fd = client_socket;
						fds[nfds].events = POLLIN;

						/**
						 * Increase the number of file descriptors to have
						 * unlimited clients.
						 */
						if (!(nfds % POLL_SIZE - 1))
						{
							fds = realloc(fds, 2 * nfds *
												   sizeof(struct pollfd));
						}
						nfds++;
					}
					else
					{
						/**
						 * Reconnect message.
						 */
						printf("New client %s connected from %s:%d\n", id,
							   inet_ntoa(tcp_addr.sin_addr),
							   ntohs(tcp_addr.sin_port));
					}
				}
				/**
				 * Events on the UDP socket file descriptor.
				 */
				else if (fds[i].fd == udp_socket)
				{
					process_udp_message(udp_socket, udp_addr, sizeof(udp_addr), clients, port);
				}
				/**
				 * Events made by the clients.
				 */
				else
				{
					char buffer[MSG_MAXSIZE];
					int rc = read(fds[i].fd, buffer, MSG_MAXSIZE);

					/**
					 * Get the client by the file descriptor.
					 */
					Client *client =
						get_client_by_fd(fds[i].fd, clients);

					/**
					 * If the client has disconnected,
					 * we will "remove" it from the list *wink* *wink*.
					 */
					if (rc <= 0)
					{
						remove_client(client);
						for (int j = i; j < nfds - 1; j++)
						{
							fds[j] = fds[j + 1];
						}
					}

					/**
					 * Set the end of the message to null character.
					 */
					buffer[rc] = '\0';

					/**
					 * Check the command and treat it accordingly.
					 * Pretty intuitive also here....so I will not explain it.
					 */
					if (client)
					{
						if (strstr(buffer, "unsubscribe"))
						{
							unsubscribe(buffer, client);
						}
						else if (strstr(buffer, "show"))
						{
							show_topics(client);
						}
						else if (strstr(buffer, "subscribe"))
						{
							if (subscribe(buffer, client) == -1)
							{
								/**
								 * Send a message if the client is already
								 * subscribed
								 */
								char message[] = "Already subscribed to topic.";
								send_all(client->socket, message,
										 strlen(message));
							}
						}
					}
					/**
					 * Clear the buffer.
					 */
					memset(buffer, 0, MSG_MAXSIZE);
				}
			}
		}
	}

	/**
	 * Free and close everything
	 */
	free_client_list(&clients);
	close(tcp_socket);
	close(udp_socket);
	free(fds);
	return 0;
}
