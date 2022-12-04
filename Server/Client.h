#ifndef CLIENT_H
#define CLIENT_H

#define MAX_FRIENDS_COUNT 10
#define MAX_GROUP_COUNT 10
#define MAX_DM_COUNT 50

#define CONNECTED 1
#define DISCONNECTED 0

#include <stdio.h>
#include <openssl/sha.h>
#include "Server.h"

struct groupConv;
struct twoPeopleConv;

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char password[BUF_SIZE];

   int status;

   struct groupConv* group_conv[MAX_GROUP_COUNT];
   int numberOfConv;

   struct twoPeopleConv* direct_messages[MAX_DM_COUNT];
   int numberOfDM;

   struct groupConv* actualConv;
   struct twoPeopleConv* actualDMConv;

   struct Client* friends[MAX_FRIENDS_COUNT];
   int numberOfFriends;
};

struct groupConv
{
   char name[50];
   struct Client* clients[10];

   int numberOfClients;
   char pathToHistory[100];
};

struct twoPeopleConv
{
   struct Client* person1;
   struct Client* person2;

   char pathToHistory[100];
};

typedef struct Client Client;
typedef struct groupConv groupConv;
typedef struct twoPeopleConv twoPeopleConv;

#endif /* guard */
