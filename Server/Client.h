#ifndef CLIENT_H
#define CLIENT_H

#define MAX_FRIENDS_COUNT 10
#define MAX_GROUP_COUNT 10

#include <stdio.h>
#include "Server.h"

struct groupConv;
struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];

   struct groupConv* group_conv[MAX_GROUP_COUNT];
   int numberOfConv;

   struct groupConv* actualConv;

   struct Client* friends[MAX_FRIENDS_COUNT];
   int numberOfFriends;
};

struct groupConv
{
   char name[50];
   struct Client* clients[10];

   int numberOfClients;

   char pathToHistory[100];
   FILE* history;
};

typedef struct Client Client;
typedef struct groupConv groupConv;

#endif /* guard */
