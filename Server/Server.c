#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "Server.h"
#include "Client.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

unsigned long hash_function(char *pseudo, int size)
{
    unsigned long i = 0;
    for (int j = 0; pseudo[j]; j++)
        i += pseudo[j];

    return i % size;
}

ClientItem *create_client(char *pseudo, Client *client)
{
    ClientItem *item = (ClientItem *)malloc(sizeof(ClientItem));
    item->pseudo = (char *)malloc(strlen(pseudo) + 1);
    strcpy(item->pseudo, pseudo);

    item->client = client;

    return item;
}

HashTable *create_table(int size)
{
    HashTable *clientsTable = (HashTable *)malloc(sizeof(HashTable));
    clientsTable->size = size;
    clientsTable->count = 0;
    clientsTable->clients = (ClientItem **)calloc(clientsTable->size, sizeof(ClientItem *));

    for (int i = 0; i < clientsTable->size; i++)
    {
        clientsTable->clients[i] = NULL;
    }

    return clientsTable;
}

void free_item(ClientItem *item)
{
    free(item->pseudo);
    free(item->client);
    free(item);
}

void free_table(HashTable *table)
{
    for (int i = 0; i < table->size; i++)
    {
        ClientItem *item = table->clients[i];
        if (item != NULL)
            free_item(item);
    }
    free(table->clients);
    free(table);
}

int ht_insert(HashTable *clientsTable, char *pseudo, Client *client)
{
    ClientItem *newItem = create_client(pseudo, client);

    int index = hash_function(pseudo, clientsTable->size);
    ClientItem *currentItem = clientsTable->clients[index];

    if (currentItem == NULL)
    {
        if (clientsTable->count == clientsTable->size)
        {
            printf("Maximum amount of clients");
            free(newItem);
            return -1;
        }
        clientsTable -> clients[index] = newItem;
        clientsTable -> count++;
        return 0;
    } else {
        printf("Client already exists");
        free(newItem);
        return 1;
    }
}

Client * ht_search (HashTable* clientsTable, char* pseudo)
{
    int index = hash_function(pseudo, clientsTable -> size);
    ClientItem* item = clientsTable -> clients[index];

    if(item != NULL)
    {
        if(strcmp(item->pseudo, pseudo) == 0)
            return item -> client;
    }

    return NULL;
}

/**
 * @brief Creates a new empty client
 *
 * @param csock client associated socket
 * @param name client name
 * @return Client*
 */
Client *newClient(int csock, char *name)
{
   Client *c;
   c = (Client *)malloc(sizeof(Client));
   c->sock = csock;

   c->numberOfConv = 0;
   c->numberOfFriends = 0;

   strncpy(c->name, name, BUF_SIZE - 1);

   return c;
}

/**
 * @brief Mutually add client in respective friend list
 *
 * @param client first client
 * @param friend second client
 */
void add_friend(Client *client, Client *friend)
{
   client->friends[client->numberOfFriends] = friend;
   (client->numberOfFriends)++;

   friend->friends[friend->numberOfFriends] = client;
   (friend->numberOfFriends)++;
}

/**
 * @brief add a client to a group conversation
 * 
 * @param client the client (pointer)
 * @param conv the convesation (pointer)
 */
void add_client_to_conv(Client *client, groupConv *conv)
{
   client->group_conv[client->numberOfConv] = conv;
   (client->numberOfConv)++;

   conv->clients[conv->numberOfClients] = client;
   (conv->numberOfClients)++;
}


static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client *clients[MAX_CLIENTS];

   /* an array for all group conversations*/
   groupConv *all_group_conv[MAX_GROUPS];
   int actualConv = 0;

   fd_set rdfs;

   HashTable *clientsTable = create_table(MAX_CLIENTS);

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i]->sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         /*          Client* c = malloc(sizeof(Client));
                  c -> sock = csock;
                  c -> numberOfConv = 0;
                  c -> numberOfFriends = 0; */
         // Client c = { csock };
         Client *c = newClient(csock, buffer);
         // strncpy(c -> name, buffer, BUF_SIZE - 1);

         clients[actual] = c;
         actual++;

         if(ht_insert(clientsTable, buffer, c))
         {
            strncpy(buffer, "Pseudo déjà utilisé", BUF_SIZE - 1);
            send_message_to_client(c, buffer, 1);
            // free(c);
         }

         printf("* [CON] %s just connected to server\n", c->name);
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i]->sock, &rdfs))
            {
               Client *client = clients[i];
               int c = read_client(clients[i]->sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i]->sock);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
                  printf("* [DISCO] %s disconnected from server\n", client->name);
                  remove_client(clients, i, &actual);
               }
               else if (!strcmp(buffer, "$"))
               {
                  strncpy(buffer, "Choisissez un pseudo : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);

                  read_client(clients[i]->sock, buffer);
                  for (i = 0; i < actual; i++)
                  {
                     // Le pseudo est le même que celui entré par le client
                     if (!strcmp(clients[i]->name, buffer))
                     {
                        // send_message_to_client(clients[i],);
                        strncpy(buffer, "L'utilisateur existe", BUF_SIZE - 1);
                        send_message_to_client(client, buffer, 1);
                        break;
                     }
                  }
               }
               else if (!strcmp(buffer, "$create"))
               {
                  strncpy(buffer, "Choisissez un un nom de conversation : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);

                  read_client(client->sock, buffer);

                  groupConv *newGroupConv = malloc(sizeof(groupConv));
                  strcpy(newGroupConv->name, buffer);
                  newGroupConv->clients[newGroupConv->numberOfClients] = client;
                  (newGroupConv->numberOfClients) = 1;

                  client->group_conv[client->numberOfConv] = newGroupConv;
                  (client->numberOfConv)++;

                  all_group_conv[actualConv] = newGroupConv;
                  actualConv++;
               }
               else if (!strcmp(buffer, "$debug"))
               {
                  strncpy(buffer, "******************", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);

                  printf("******************\n");
                  for (int k = 0; k < actual; ++k)
                  {
                     printf("Client n°%d :\n", k);
                     printf("\t * nom : %s\n", clients[k]->name);
                     printf("\t * socket : %d\n", clients[k]->sock);
                     if (clients[k]->numberOfConv != 0)
                     {
                        printf("\t * Conversations : \n");

                        int temp = clients[k]->numberOfConv;
                        for (int j = 0; j < temp; ++j)
                        {
                           printf("\t\t * Nom : %s\n", clients[k]->group_conv[j]->name);
                        }
                     }
                     if (clients[k]->numberOfFriends != 0)
                     {
                        printf("\t * Amis : \n");

                        int temp = clients[k]->numberOfFriends;
                        for (int j = 0; j < temp; ++j)
                        {
                           printf("\t\t * Nom : %s\n", clients[k]->friends[j]->name);
                        }
                     }
                  }
               }
               else if (!strcmp(buffer, "$add"))
               {
                  strncpy(buffer, "Choisissez une conversation de groupe : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);

                  read_client(client->sock, buffer);
                  int maxConv = client->numberOfConv;

                  groupConv *conv = NULL;
                  for (int k = 0; k < maxConv; k++)
                  {
                     // Le nom de la conv est le même que celui entré par le client
                     if (!strcmp(client->group_conv[k]->name, buffer))
                     {
                        // send_message_to_client(clients[i],);
                        strncpy(buffer, "La conversation existe", BUF_SIZE - 1);
                        send_message_to_client(client, buffer, 1);

                        conv = client->group_conv[k];
                        break;
                     }
                  }

                  if (conv != NULL)
                  {

                     strncpy(buffer, "Choisissez un pseudo à ajouter dans la conv : ", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);

                     read_client(client->sock, buffer);
                     for (int k = 0; k < actual; k++)
                     {
                        // Le pseudo est le même que celui entré par le client
                        if (!strcmp(clients[k]->name, buffer))
                        {
                           // send_message_to_client(clients[i],);
                           strncpy(buffer, "L'utilisateur existe", BUF_SIZE - 1);
                           send_message_to_client(client, buffer, 1);

                           add_client_to_conv(clients[k], conv);

                           strncpy(buffer, "Utilisateur ajouté dans la conv.", BUF_SIZE - 1);
                           send_message_to_client(client, buffer, 1);
                           break;
                        }
                     }
                  }
                  else
                  {
                     strncpy(buffer, "La conversation n'existe pas.", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                  }
               }
               else if (!strcmp(buffer, "$friend"))
               {
                  strncpy(buffer, "Entrez le pseudo de la personne à ajouter en ami : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);

                  read_client(client->sock, buffer);

                  /**
                   * Looks if client exists to add it in friend list
                   */
                  Client *newFriend = NULL;
                  for (int k = 0; k < actual; k++)
                  {
                     if (!strcmp(clients[k]->name, buffer))
                     {
                        newFriend = clients[k];
                        add_friend(client, newFriend);

                        strncpy(buffer, "Ami ajouté\n", BUF_SIZE - 1);
                        send_message_to_client(client, buffer, 1);
                        
                        strncpy(buffer, "***************************************\n", BUF_SIZE - 1);
                        strcat(buffer, client -> name);
                        strcat(buffer, " vous a ajouté en ami\n***************************************\n");
                        send_message_to_client(newFriend, buffer, 1);
                        break;
                     }
                  }

                  if (newFriend == NULL)
                  {
                     strncpy(buffer, "Personne introuvable\n", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                  }
               }
               else if (!strcmp(buffer, "$list"))
               {
                  strncpy(buffer, "Liste d'amis :", BUF_SIZE - 1);
                  int numberOfFriends = client->numberOfFriends;
                  if (numberOfFriends == 0)
                  {
                     strcat(buffer, "\n\t Liste vide");
                  }
                  else
                  {
                     for (int k = 0; k < numberOfFriends; k++)
                     {
                        strcat(buffer, "\n\t * ");
                        strcat(buffer, client->friends[k]->name);
                     }
                  }
                  // strncpy(buffer, "Liste\n", BUF_SIZE - 1);
                  strcat(buffer, "\n");
                  send_message_to_client(client, buffer, 1);
               }
               else
               {
                  send_message_to_all_clients(clients, client, actual, buffer, 0);
                  printf("* [MESS] Message from %s : %s\n", client->name, buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, all_group_conv, actual, actualConv);
   end_connection(sock);
}

static void clear_clients(Client **clients, groupConv **conv, int actual, int actualConv)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i]->sock);
      free(clients[i]);
   }

   for (int i = 0; i < actualConv; ++i)
   {
      free(conv[i]);
   }
}

static void remove_client(Client **clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   free(clients[to_remove]);
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client *));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client **clients, Client *sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender->sock != clients[i]->sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender->name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i]->sock, message);
      }
   }
}

static void send_message_to_client(Client *receiver, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;

   // if(from_server == 0)
   // {
   //    strncpy(message, sender.name, BUF_SIZE - 1);
   //    strncat(message, " : ", sizeof message - strlen(message) - 1);
   // }
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   write_client(receiver->sock, message);
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}



int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
