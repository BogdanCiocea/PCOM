#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <math.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/**
 * Dimensiunea maxima a mesajului
 */
#define MSG_MAXSIZE 1551
#define MAX_CLIENTS 100
#define POLL_SIZE (MAX_CLIENTS + 2)
#define CONTENT_SIZE 1500
#define TOPIC_SIZE 50
#define ID_SIZE 10

#define forever while (1) // I had to. IT'S A MUST!

typedef struct client
{
  char id[ID_SIZE];
  int socket;
  struct sockaddr_in addr;
  struct client *next;
  bool connected;
  struct topic *subscribed;
} Client;

typedef struct topic
{
  char name[TOPIC_SIZE + 1];
  struct topic *next;
  int type;
  bool has_wildcard;
  char content[CONTENT_SIZE];
} Topic;

#define DIE(assertion, call_description)                 \
  do                                                     \
  {                                                      \
    if (assertion)                                       \
    {                                                    \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
      perror(call_description);                          \
      exit(EXIT_FAILURE);                                \
    }                                                    \
  } while (0)

#endif