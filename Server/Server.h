#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_GROUPS    100

#define BUF_SIZE    1024



#include "Client.h"

typedef struct
{
    char *pseudo;
    // int status;
    Client *client;
} ClientItem;

typedef struct
{
    ClientItem **clients;
    int size;
    int count;
} HashTable;

unsigned long hash_function(char *pseudo, int size);
ClientItem *create_client(char *pseudo, Client *client);
HashTable *create_table(int size);
void free_item(ClientItem *item);
void free_table(HashTable *table);
int ht_insert(HashTable *clientsTable, char *pseudo, Client *client);
Client * ht_search (HashTable* clientsTable, char* pseudo);


static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client **clients, Client *client, int actual, const char *buffer, char from_server);
static void remove_client(Client **clients, int to_remove, int *actual);
static void clear_clients(Client **clients, groupConv **conv, int actual, int actualConv);

static void send_message_to_client(Client *receiver, const char *buffer, char from_server);
#endif /* guard */
