#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
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

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client* clients[MAX_CLIENTS];

   groupConv* all_group_conv[MAX_GROUPS];
   int actualConv = 0;

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i] -> sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client* c = malloc(sizeof(Client));
         c -> sock = csock;
         c -> numberOfConv = 0;
         c -> actualConv = NULL;

         // Client c = { csock };
         strncpy(c -> name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;
         printf("* [CON] %s connected to server\n", c -> name);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i] -> sock, &rdfs))
            {
               Client* client = clients[i];
               int c = read_client(clients[i] -> sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i] -> sock);
                  strncpy(buffer, client -> name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
                  printf("* [DISCO] %s disconnected from server\n", client -> name);
                  remove_client(clients, i, &actual);
               }
               else if (!strcmp(buffer,"$")) 
               {
                  strncpy(buffer, "Choisissez un pseudo : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);
                  
                  read_client(clients[i] -> sock, buffer);
                  for(i = 0; i < actual; i++)
                  {
                     // Le pseudo est le même que celui entré par le client
                     if(!strcmp(clients[i] -> name,buffer)) 
                     {
                        // send_message_to_client(clients[i],);
                        strncpy(buffer, "L'utilisateur existe", BUF_SIZE - 1);
                        send_message_to_client(client, buffer, 1);
                        break;
                     }
                  }
               }
               else if (!strcmp(buffer,"$create")) 
               {
                  strncpy(buffer, "Choisissez un un nom de conversation : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);
                  
                  read_client(client -> sock, buffer);
                  
                  groupConv* newGroupConv = malloc(sizeof(groupConv));
                  strcpy(newGroupConv -> name,buffer);
                  newGroupConv -> clients[newGroupConv -> numberOfClients] = client;
                  (newGroupConv -> numberOfClients) = 1;

                  client -> group_conv[client -> numberOfConv] = newGroupConv;
                  (client -> numberOfConv)++;

                  all_group_conv[actualConv] = newGroupConv;
                  actualConv++;
               }
               else if (!strcmp(buffer,"$debug")) 
               {
                  strncpy(buffer, "******************", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);
            
                  printf("******************\n");
                  for(int k = 0; k < actual; ++k) {
                     printf("Client n°%d :\n", k);
                     printf("\t * nom : %s\n", clients[k] -> name);
                     printf("\t * socket : %d\n", clients[k] -> sock);
                     printf("\t * Conversations : \n");

                     int temp = clients[k] -> numberOfConv;
                     for(int j = 0; j < temp; ++j) {
                        printf("\t\t * Nom : %s\n", clients[k] -> group_conv[j] -> name);
                     }

                  }
               }
               else if (!strcmp(buffer,"$add")) 
               {
                  strncpy(buffer, "Choisissez une conversation de groupe : ", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);
                  
                  read_client(client -> sock, buffer);
                  int maxConv = client -> numberOfConv;

                  groupConv* conv = NULL;
                  for(int k = 0; k < maxConv; k++) 
                  {
                     // Le nom de la conv est le même que celui entré par le client
                     if(!strcmp(client -> group_conv[k] -> name,buffer)) 
                     {
                        // send_message_to_client(clients[i],);
                        strncpy(buffer, "La conversation existe", BUF_SIZE - 1);
                        send_message_to_client(client, buffer, 1);
                        
                        conv = client -> group_conv[k];
                        break;
                     }
                  }

                  if(conv != NULL) {

                     strncpy(buffer, "Choisissez un pseudo à ajouter dans la conv : ", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                     
                     read_client(client -> sock, buffer);
                     for(int k = 0; k < actual; k++)
                     {
                        // Le pseudo est le même que celui entré par le client
                        if(!strcmp(clients[k] -> name,buffer)) 
                        {
                           // send_message_to_client(clients[i],);
                           strncpy(buffer, "L'utilisateur existe", BUF_SIZE - 1);
                           send_message_to_client(client, buffer, 1);
                           
                           add_client_to_conv(clients[k],conv);

                           strncpy(buffer, "Utilisateur ajouté dans la conv.", BUF_SIZE - 1);
                           send_message_to_client(client, buffer, 1);
                           break;
                        }
                     }
                  }
                  else {
                     strncpy(buffer, "La conversation n'existe pas.", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                  }
               }
               else if (!strcmp(buffer,"$join")) 
               {
                  strncpy(buffer, "*********************\n Choisissez une de vos conversations de groupe à rejoindre : \n********************\n", BUF_SIZE - 1);
                  send_message_to_client(client, buffer, 1);
                  
                  read_client(client -> sock, buffer);
                  int maxConv = client -> numberOfConv;

                  groupConv* conv = NULL;
                  for(int k = 0; k < maxConv; k++) 
                  {
                     // Le nom de la conv est le même que celui entré par le client
                     if(!strcmp(client -> group_conv[k] -> name,buffer)) 
                     {
                        conv = client -> group_conv[k]; 

                        strncpy(buffer, "Conversation actuelle : ", BUF_SIZE - 1);
                        strncat(buffer, conv -> name, BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_client(client, buffer, 1);

                        break;
                     }
                  }
                  
                  if(!(conv == NULL)) {
                     client -> actualConv = conv;
                  }
                  else {
                     strncpy(buffer, "La conversation n'existe pas.\n**********************", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                  }
               }
               else
               {
                  // send_message_to_all_clients(clients, client, actual, buffer, 0);
                  if(client-> actualConv == NULL) {
                     strncpy(buffer, "Vous n'êtes pas dans une conversation !", BUF_SIZE - 1);
                     send_message_to_client(client, buffer, 1);
                  }
                  else {
                     send_message_to_clients_in_conv(client -> actualConv,client,buffer);
                     printf("* [MESS] Message from %s : %s in conv %s\n",client -> name,buffer,client -> actualConv -> name);
                  }
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
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i] -> sock);
      free(clients[i]);
   }

   for(int i = 0; i < actualConv; ++i) {
      free(conv[i]);
   }
}

static void remove_client(Client **clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   free(clients[to_remove]);
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client*));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client **clients, Client *sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender -> sock != clients[i] -> sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender -> name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i] -> sock, message);
      }
   }
}

static void send_message_to_client(Client* receiver, const char *buffer, char from_server)
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
   write_client(receiver -> sock, message);
}

static void send_message_to_clients_in_conv(groupConv* conv, Client* sender, const char *buffer)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;

   strncpy(message, sender -> name, BUF_SIZE - 1);
   strncat(message, " : ", sizeof message - strlen(message) - 1);
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   
   int num = conv -> numberOfClients;
   for(i = 0; i < num; ++i) {

      //on n'envoie pas le message à l'envoyeur
      if(sender != conv -> clients[i]) {
         //on vérifie que le client a join la conversation 
         if(conv -> clients[i] -> actualConv == conv) {
            write_client(conv -> clients[i] -> sock, message);
         }
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
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

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
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
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void add_client_to_conv(Client* client, groupConv* conv) 
{
   client -> group_conv[client -> numberOfConv] = conv;
   (client -> numberOfConv)++;

   conv -> clients[conv -> numberOfClients] = client;
   (conv -> numberOfClients)++;
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
