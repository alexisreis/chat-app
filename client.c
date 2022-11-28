/* Client pour les sockets
 *    socket_client ip_server port
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  int sockfd, newsockfd, clilen, chilpid, ok, nleft, nbwriten;
  char c;
  struct sockaddr_in cli_addr, serv_addr;

  if (argc != 3)
  {
    printf("usage  socket_client server port\n");
    exit(0);
  }

  /*
   *  partie client
   */
  printf("client starting\n");

  /* initialise la structure de donnee */
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  /* ouvre le socket */
  /* @param AF_INET : protocole de communication IP */
  /* @param SOCK_STREAM : TCP */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("socket error\n");
    exit(0);
  }

  /* effectue la connection */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("socket error\n");
    exit(0);
  }

  /* repete dans le socket tout ce qu'il entend */
  while (c != EOF)
  {
    c = getchar();
    write(sockfd, &c, 1);
  }

  /*  attention il s'agit d'une boucle infinie
   *  le socket n'est jamais ferme !
   */
  close(sockfd);

  return 1;
}
