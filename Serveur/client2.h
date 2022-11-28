#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

struct groupConv;

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];

   struct groupConv* group_conv[10];
   int numberOfConv;
   
};

struct groupConv
{
   char name[50];
   struct Client* clients[10];

   int numberOfClients;
};

typedef struct Client Client;
typedef struct groupConv groupConv;

#endif /* guard */
