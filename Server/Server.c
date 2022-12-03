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

   c -> actualConv = NULL;

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

   twoPeopleConv *all_dm_conv[MAX_DM_COUNT];
   int actualDMConv = 0;

   fd_set rdfs;

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

         Client *c = newClient(csock, buffer);

         clients[actual] = c;
         actual++;
         printf("* [CON] %s connected to server\n", c->name);
         printHelpPageHome(c);
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
                  strncat(buffer, " s'est déconnecté *********\n", BUF_SIZE - strlen(buffer) - 1);
                  
                  if (!(client-> actualConv == NULL) && client-> actualDMConv == NULL) {
                     send_message_to_clients_in_conv(client -> actualConv,client,buffer);
                  }
                  // Conversation client
                  else if (client-> actualConv == NULL && !(client-> actualDMConv == NULL)) {
                     if(!strcmp(client -> name, client -> actualDMConv -> person1 -> name)) {
                        printToClient(client -> actualDMConv -> person2,buffer);
                     }
                     else {
                        printToClient(client -> actualDMConv -> person1,buffer);
                     }
                  }

                  printf("* [DISCO] %s disconnected from server\n", client->name);
                  remove_client(clients, i, &actual);
               }
               else if (!strcmp(buffer, "$help"))
               {
                  printHelpPageHome(client);
               }
               else if (!strcmp(buffer, "$creategroup"))
               {
                  printToClient(client,"Choisissez un nom de conversation :");

                  read_client(client->sock, buffer);

                  int maxConv = client->numberOfConv;
                  int existeDeja = 0;
                  for (int k = 0; k < maxConv; k++)
                  {
                     // Le nom de la conv est le même que celui entré par le client
                     if (!strcmp(client->group_conv[k]->name, buffer))
                     {
                        existeDeja = 1;
                        break;
                     }
                  }

                  if(existeDeja != 0) {
                     printToClient(client,"La conversation existe déjà, sortie...\n");
                  }
                  else {
                     groupConv *newGroupConv = malloc(sizeof(groupConv));
                     strcpy(newGroupConv->name, buffer);
                     newGroupConv->clients[newGroupConv->numberOfClients] = client;
                     (newGroupConv->numberOfClients) = 1;

                     client->group_conv[client->numberOfConv] = newGroupConv;
                     (client->numberOfConv)++;

                     all_group_conv[actualConv] = newGroupConv;
                     actualConv++;

                     /*Gestion de l'historique*/
                     strcpy(buffer,"../history/group/");
                     strcat(buffer,newGroupConv->name);
                     strcat(buffer,".his");
                     strcpy(newGroupConv -> pathToHistory,buffer);

                     if(fopen(newGroupConv -> pathToHistory, "r") == NULL) {
                        fopen(newGroupConv -> pathToHistory, "w");
                     }

                     printToClient(client,"Conversation créée !\n");
                  }
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
               else if (!strcmp(buffer, "$addgroup"))
               {
                  printToClient(client,"Choisissez une conversation de groupe : \n");
                  printGroupsToClient(client);

                  int maxConv = client->numberOfConv;

                  if(maxConv != 0) {
                     read_client(client->sock, buffer);
                     groupConv *conv = NULL;
                     for (int k = 0; k < maxConv; k++)
                     {
                        // Le nom de la conv est le même que celui entré par le client
                        if (!strcmp(client->group_conv[k]->name, buffer))
                        {
                           conv = client->group_conv[k];
                           break;
                        }
                     }

                     if (conv != NULL)
                     {

                        printToClient(client,"Choisissez un utilisateur à ajouter dans le groupe :");

                        read_client(client->sock, buffer);
                        int it5 = 0;
                        for (int k = 0; k < actual; k++)
                        {
                           // Le pseudo est le même que celui entré par le client
                           if (!strcmp(clients[k]->name, buffer))
                           {
                           
                              add_client_to_conv(clients[k], conv);
                              printToClient(client,"Utilisateur ajouté dans le groupe.\n");

                              it5++;
                              break;
                           }
                        }
                        if(it5 == 0) {
                           printToClient(client,"L'utilisateur n'existe pas.\n");
                        }
                     }
                     else
                     {
                        printToClient(client,"La conversation n'existe pas.\n");
                     }
                  }
               }
               else if (!strcmp(buffer, "$addfriend"))
               {
                  printToClient(client,"Entrez le pseudo de la personne à ajouter en ami : ");
                  read_client(client->sock, buffer);

                  int maxFriends = client->numberOfConv;
                  int existeDeja = 0;
                  for (int k = 0; k < maxFriends; k++)
                  {
                     // Le nom de l'ami est le même que celui entré par le client
                     if (!strcmp(client->friends[k]->name, buffer))
                     {
                        existeDeja = 1;
                        break;
                     }
                  }

                  if(existeDeja != 0) {
                     printToClient(client,"Vous êtes déja ami avec cette personne. Sortie...\n");
                  }
                  else {
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
               }
               else if (!strcmp(buffer,"$joingroup")) 
               {
                  printToClient(client,"Choisissez une de vos conversations de groupe à rejoindre :\n");
                  printGroupsToClient(client);
         
                  read_client(client -> sock, buffer);
                  int maxConv = client -> numberOfConv;
                  groupConv* conv = NULL;
                  for(int k = 0; k < maxConv; k++) 
                  {
                     // Le nom de la conv est le même que celui entré par le client
                     if(!strcmp(client -> group_conv[k] -> name,buffer)) 
                     {
                        conv = client -> group_conv[k]; 

                        clearClientScreen(client);
                        strncpy(buffer, "**************************\nConversation actuelle : ", BUF_SIZE - 1);
                        strcat(buffer,conv -> name);
                        strcat(buffer, "\nMembres dans la conversation :\n");
                        int max = conv -> numberOfClients;
                        for(int w = 0; w < max; w++) {
                           strcat(buffer,"\t* ");
                           strcat(buffer,conv -> clients[w] -> name);
                           strcat(buffer,"\n");
                        }
                        strcat(buffer, "\n*************************");
                        printToClient(client, buffer);

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
               else if (!strcmp(buffer,"$dm")) 
               {
                  printToClient(client,"Choisissez un ami à contacter directement : \n");
                  printFriendsToClient(client); //message 0 amis print dans la fonction

                  int maxfriends = client -> numberOfFriends;
                  if(maxfriends != 0) {

                     read_client(client -> sock, buffer);
                     char toPrint[BUF_SIZE];

                     int it1 = 0;
                     for (int k = 0; k < maxfriends; k++)
                     {
                        // Le pseudo est le même que celui entré par le client
                        if (!strcmp(client->friends[k]->name, buffer))
                        {
                           clearClientScreen(client);
                           strncpy(toPrint, "Votre ami : ", BUF_SIZE - 1);
                           strcat(toPrint,client->friends[k]->name);
                           strcat(toPrint," a été trouvé, démarrage de la conversation privée...\n");
                           printToClient(client,toPrint);

                           /* On regarde si la conversation existe déjà en tant qu'objet */
                           int maxJ = client -> numberOfDM;
                           int it = 0;
                           for (int j = 0; j < maxJ; ++j)
                           {
                              if (!strcmp(client->direct_messages[j]->person1->name, buffer)
                                 || !strcmp(client->direct_messages[j]->person2->name, buffer))
                              {
                                 client -> actualDMConv = client -> direct_messages[j];
                                 break;
                              }
                              it++;
                           }

                           // La conversation n'existe pas en tant qu'objet
                           if(it == maxJ) {
                              all_dm_conv[actualDMConv] = create_new_dm_conv(client,client->friends[k]);
                              client -> actualDMConv = all_dm_conv[actualDMConv];
                              actualDMConv++;    
                           }

                           break;
                        }
                        
                        it1++;
                     }

                     if(it1 == maxfriends) {
                        printToClient(client,"La personne n'existe pas ou vous n'êtes pas ami avec elle.\n");
                     }
                     else {
                        strncpy(buffer, "********* CONVERSATION PRIVEE ENTRE ", BUF_SIZE - 1);
                        strcat(buffer, client -> actualDMConv -> person1 -> name);
                        strcat(buffer, " et ");
                        strcat(buffer, client -> actualDMConv -> person2 -> name);
                        strcat(buffer, " *********");

                        printToClient(client,buffer);
                     }
                  }
               }
               else if (!strcmp(buffer,"$exit")) 
               {
                  if(client-> actualConv == NULL && client-> actualDMConv == NULL) {
                     printToClient(client,"Vous êtes déjà à l'accueil.\n");
                  }
                  else {
                     clearClientScreen(client);
                     client -> actualConv = NULL;
                     client -> actualDMConv = NULL;
                     printHelpPageHome(client);
                  }
               }
               else if (!strcmp(buffer,"$listgroups")) 
               {
                  printGroupsToClient(client);
               }
               else if (!strcmp(buffer,"$listfriends")) 
               {
                  printFriendsToClient(client);
               }
               else
               {
                  // send_message_to_all_clients(clients, client, actual, buffer, 0);
                  if(client-> actualConv == NULL && client-> actualDMConv == NULL) {
                     printToClient(client,"Commande inconnue. Tapez $help pour afficher l'aide.\n");
                  }
                  // Conversation de groupe
                  else if (!(client-> actualConv == NULL) && client-> actualDMConv == NULL) {
                     send_message_to_clients_in_conv(client -> actualConv,client,buffer);
                     printf("* [MESS] Message from %s : %s in conv %s\n",client -> name,buffer,client -> actualConv -> name);
                  }
                  // Conversation client
                  else if (client-> actualConv == NULL && !(client-> actualDMConv == NULL)) {
                     // char temp[BUF_SIZE];
                     // strcpy(temp,buffer);
                     // strcpy(buffer,"\033[0;36m");
                     // strcat(buffer,temp);
                     // strcat(buffer,"\033[0;37m");
                     //sprintf(test,"\033[0;36m%s\033[0;37m",buffer);
                     if(!strcmp(client -> name, client -> actualDMConv -> person1 -> name)) {
                        send_direct_message_to_client(client -> actualDMConv -> person1, client -> actualDMConv -> person2, client -> actualDMConv, buffer);
                        printf("* [DIRECTMESS] Message from %s to %s: %s \n",client -> name,client -> actualDMConv -> person2 -> name,buffer);
                     }
                     else {
                        send_direct_message_to_client(client -> actualDMConv -> person2, client -> actualDMConv -> person1, client -> actualDMConv, buffer);
                        printf("* [DIRECTMESS] Message from %s to %s: %s \n",client -> name,client -> actualDMConv -> person1 -> name, buffer);
                     }
                  }
                  else {

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

static void send_direct_message_to_client(Client* sender, Client *receiver, twoPeopleConv* dmConv, const char *buffer)
{
   char message[BUF_SIZE];
   message[0] = 0;

   printf("%s / %s\n",sender -> name, receiver -> name);
   strcpy(message, sender -> name);
   strncat(message, " : ", sizeof message - strlen(message) - 1);
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   
   if(receiver -> actualDMConv == dmConv) {
      write_client(receiver->sock, message);
   }

   /* Gestion dans l'historique */
   FILE* temp = fopen(dmConv -> pathToHistory,"a");
   fprintf(temp,"%s\n",message);
   fclose(temp);
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

   /* Gestion dans l'historique */
   FILE* temp = fopen(conv -> pathToHistory,"a");
   fprintf(temp,"%s\n",message);
   fclose(temp);
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

/**
 * @brief add a client to a group conversation
 * 
 * @param client the client (pointer)
 * @param conv the convesation (pointer)
 */
static void add_client_to_conv(Client* client, groupConv* conv) 
{
   client -> group_conv[client -> numberOfConv] = conv;
   (client -> numberOfConv)++;

   conv -> clients[conv -> numberOfClients] = client;
   (conv -> numberOfClients)++;
}

static twoPeopleConv* create_new_dm_conv(Client* client1, Client* client2) {

   char temp[BUF_SIZE];

   twoPeopleConv *newTwoPeopleConv = malloc(sizeof(twoPeopleConv));
   newTwoPeopleConv -> person1 = client1;
   newTwoPeopleConv -> person2 = client2;                             
   
   client1-> direct_messages[client1->numberOfDM] = newTwoPeopleConv;
   (client1->numberOfDM)++;    

   client2-> direct_messages[client2->numberOfDM] = newTwoPeopleConv;
   (client2->numberOfDM)++;                                                     
   
   /*Gestion de l'historique*/
   strcpy(temp,"../history/person/");
   if(strcmp(newTwoPeopleConv -> person1 -> name,newTwoPeopleConv -> person2 -> name) < 0) {
      strcat(temp,newTwoPeopleConv -> person1 -> name);
      strcat(temp,"_");
      strcat(temp,newTwoPeopleConv -> person2 -> name);
   }
   else {
      strcat(temp,newTwoPeopleConv -> person2 -> name);
      strcat(temp,"_");
      strcat(temp,newTwoPeopleConv -> person1 -> name);
   }
   strcat(temp,".his");
   strcpy(newTwoPeopleConv -> pathToHistory,temp);                             
   
   if(fopen(newTwoPeopleConv -> pathToHistory, "r") == NULL) {
      fopen(newTwoPeopleConv -> pathToHistory, "w");
   }

   return newTwoPeopleConv;
}

static void printToClient(Client* client, const char* toDisplay) {
   char toPrint[BUF_SIZE];
   strncpy(toPrint, toDisplay, BUF_SIZE - 1);

   send_message_to_client(client, toPrint, 1);
}

static void printHelpPageHome(Client* client) {
   char LIST_HELP[BUF_SIZE];
   strcpy(LIST_HELP,"------ ACCUEIL ------\n");
   strcat(LIST_HELP,"Commandes disponibles : \n");
   strcat(LIST_HELP,"\t $debug : print les logs côté serveur\n");
   strcat(LIST_HELP,"\t $creategroup : créer une conversation de groupe\n");
   strcat(LIST_HELP,"\t $addgroup : ajouter un ami dans une conversation de groupe\n");
   strcat(LIST_HELP,"\t $listgroups : lister les conversations de groupe\n");
   strcat(LIST_HELP,"\t $addfriend : ajouter un ami\n");
   strcat(LIST_HELP,"\t $listfriends : lister les amis\n");
   strcat(LIST_HELP,"\t $joingroup: rejoindre une conversation de groupe\n");
   strcat(LIST_HELP,"\t $dm : commencer une conversation privée avec un de vos amis\n\n");
   strcat(LIST_HELP,"\t $help : pour afficher la liste des coommandes disponibles\n");
   printToClient(client,LIST_HELP);

}

static void printGroupsToClient(Client* client) {

   int maxConv = client -> numberOfConv;
   if(maxConv == 0) {
      printToClient(client,"Vous n'êtes dans aucun groupe pour le moment.\n");
      return;
   }

   printToClient(client,"Liste de vos groupes :\n");
   char buff[BUF_SIZE];
   for (int k = 0; k < maxConv; k++)
   {
      strcpy(buff,"");
      sprintf(buff,"\t* %s (%d membre(s))\n",client -> group_conv[k] -> name,client -> group_conv[k] -> numberOfClients);
      printToClient(client,buff);
   }
}

static void printFriendsToClient(Client* client) {

   int maxFriends = client -> numberOfFriends;
   if(maxFriends == 0) {
      printToClient(client,"Vous n'avez encore aucun ami(s) sur le serveur !\n");
      return;
   }

   printToClient(client,"Liste de vos amis :\n");
   char buff[BUF_SIZE];
   for (int k = 0; k < maxFriends; k++)
   {
      strcpy(buff,"\t*");
      strcat(buff,client -> friends[k] -> name);
      strcat(buff,"\n");
      printToClient(client,buff);
   }
}

static void clearClientScreen(Client* client) {
   printToClient(client,"\033[2J");
}


int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
