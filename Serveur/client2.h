#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

struct groupConv;
struct Client;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   struct groupConv* group_conv[10];
   
}Client;

typedef struct
{
   char conv_name[50];
   Client* clients[10];
}groupConv;


#endif /* guard */
