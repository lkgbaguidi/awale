#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "server2.h"

static void init(void)
{
   /* for the random choice of who starts a game */
   srand(time(NULL));

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
   Client clients[MAX_CLIENTS];

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
         FD_SET(clients[i].sock, &rdfs);
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
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* no room left : we tell the client and hang up */
         if(actual == MAX_CLIENTS)
         {
            write_client(csock, "ERREUR : serveur plein");
            closesocket(csock);
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == 0)
         {
            /* disconnected */
            closesocket(csock);
            continue;
         }

         /* the name must be valid and not already used */
         const char *erreur = erreur_pseudo(clients, actual, buffer);
         if(erreur != NULL)
         {
            write_client(csock, erreur);
            closesocket(csock);
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.defi_de = INVALID_SOCKET;
         c.adversaire = INVALID_SOCKET;
         clients[actual] = c;
         actual++;

         write_client(csock, "Bienvenue ! Commandes disponibles : " COMMANDES);
         strncpy(buffer, c.name, BUF_SIZE - 1);
         strncat(buffer, " joined !", BUF_SIZE - strlen(buffer) - 1);
         send_message_to_all_clients(clients, c, actual, buffer, 1);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  oublier_client(clients, actual, client);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else if(buffer[0] == '/')
               {
                  /* a command, the answer goes to this client only */
                  traiter_commande(clients, actual, i, buffer);
               }
               else
               {
                  send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         /* the message is rebuilt for each receiver */
         message[0] = 0;
         if(from_server == 0)
         {
            strncat(message, sender.name, sizeof message - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

/* returns NULL if the name is acceptable, otherwise the message to send to the client */
static const char *erreur_pseudo(const Client *clients, int actual, const char *pseudo)
{
   int i = 0;
   /* no space : later commands will be split on spaces */
   if(strchr(pseudo, ' ') != NULL)
   {
      return "ERREUR : le pseudo ne doit pas contenir d'espace";
   }
   for(i = 0; i < actual; i++)
   {
      if(strcmp(clients[i].name, pseudo) == 0)
      {
         return "ERREUR : pseudo déjà pris";
      }
   }
   return NULL;
}

static void traiter_commande(Client *clients, int actual, int index, const char *commande)
{
   char message[BUF_SIZE];
   int i = 0;

   if(strcmp(commande, "/list") == 0)
   {
      message[0] = 0;
      strncat(message, "Joueurs en ligne :", sizeof message - 1);
      for(i = 0; i < actual; i++)
      {
         strncat(message, " ", sizeof message - strlen(message) - 1);
         strncat(message, clients[i].name, sizeof message - strlen(message) - 1);
         if(clients[i].adversaire != INVALID_SOCKET)
         {
            strncat(message, "(en partie)", sizeof message - strlen(message) - 1);
         }
      }
      write_client(clients[index].sock, message);
   }
   else if(strcmp(commande, "/challenge") == 0)
   {
      write_client(clients[index].sock, "Usage : /challenge <pseudo>");
   }
   else if(strncmp(commande, "/challenge ", 11) == 0)
   {
      commande_challenge(clients, actual, index, commande + 11);
   }
   else if(strcmp(commande, "/accept") == 0)
   {
      commande_accept(clients, actual, index);
   }
   else if(strcmp(commande, "/refuse") == 0)
   {
      commande_refuse(clients, actual, index);
   }
   else
   {
      write_client(clients[index].sock, "Commande inconnue. Commandes disponibles : " COMMANDES);
   }
}

/* index of the client called nom, -1 if nobody has this name */
static int trouver_client(const Client *clients, int actual, const char *nom)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      if(strcmp(clients[i].name, nom) == 0)
      {
         return i;
      }
   }
   return -1;
}

/* index of the client using this socket, -1 if there is none */
static int trouver_client_par_sock(const Client *clients, int actual, SOCKET sock)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      if(clients[i].sock == sock)
      {
         return i;
      }
   }
   return -1;
}

/* /challenge nom : the client at index challenges nom */
static void commande_challenge(Client *clients, int actual, int index, const char *nom)
{
   char message[BUF_SIZE];
   int cible = trouver_client(clients, actual, nom);

   if(clients[index].adversaire != INVALID_SOCKET)
   {
      write_client(clients[index].sock, "ERREUR : vous êtes déjà en partie");
      return;
   }
   if(cible == -1)
   {
      snprintf(message, sizeof message, "ERREUR : joueur inconnu : %s", nom);
      write_client(clients[index].sock, message);
      return;
   }
   if(cible == index)
   {
      write_client(clients[index].sock, "ERREUR : vous ne pouvez pas vous défier vous-même");
      return;
   }
   if(clients[cible].adversaire != INVALID_SOCKET)
   {
      snprintf(message, sizeof message, "ERREUR : %s est déjà en partie", nom);
      write_client(clients[index].sock, message);
      return;
   }
   if(clients[cible].defi_de != INVALID_SOCKET)
   {
      snprintf(message, sizeof message, "ERREUR : %s a déjà un défi en attente", nom);
      write_client(clients[index].sock, message);
      return;
   }

   /* the challenged client remembers who challenged him */
   clients[cible].defi_de = clients[index].sock;

   snprintf(message, sizeof message, "Défi envoyé à %s, en attente de sa réponse", nom);
   write_client(clients[index].sock, message);
   snprintf(message, sizeof message, "%s vous défie ! Tapez /accept ou /refuse", clients[index].name);
   write_client(clients[cible].sock, message);
}

/* /accept : the client at index accepts the challenge he received */
static void commande_accept(Client *clients, int actual, int index)
{
   char message[BUF_SIZE];
   int defieur = trouver_client_par_sock(clients, actual, clients[index].defi_de);

   if(defieur == -1)
   {
      write_client(clients[index].sock, "ERREUR : aucun défi en attente");
      return;
   }
   if(clients[defieur].adversaire != INVALID_SOCKET)
   {
      clients[index].defi_de = INVALID_SOCKET;
      snprintf(message, sizeof message, "ERREUR : %s est déjà en partie", clients[defieur].name);
      write_client(clients[index].sock, message);
      return;
   }

   /* both players are now opponents, and a pending challenge is no longer possible */
   clients[index].adversaire = clients[defieur].sock;
   clients[defieur].adversaire = clients[index].sock;
   clients[index].defi_de = INVALID_SOCKET;
   clients[defieur].defi_de = INVALID_SOCKET;

   /* random choice of who starts */
   const char *premier = (rand() % 2 == 0) ? clients[defieur].name : clients[index].name;
   snprintf(message, sizeof message, "Partie lancée : %s contre %s. %s commence !",
            clients[defieur].name, clients[index].name, premier);
   write_client(clients[defieur].sock, message);
   write_client(clients[index].sock, message);
}

/* /refuse : the client at index refuses the challenge he received */
static void commande_refuse(Client *clients, int actual, int index)
{
   char message[BUF_SIZE];
   int defieur = trouver_client_par_sock(clients, actual, clients[index].defi_de);

   if(defieur == -1)
   {
      write_client(clients[index].sock, "ERREUR : aucun défi en attente");
      return;
   }
   clients[index].defi_de = INVALID_SOCKET;

   snprintf(message, sizeof message, "%s a refusé votre défi", clients[index].name);
   write_client(clients[defieur].sock, message);
   write_client(clients[index].sock, "Défi refusé");
}

/* parti has just left : nobody must keep a link to its (now closed) socket */
static void oublier_client(Client *clients, int actual, Client parti)
{
   char message[BUF_SIZE];
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      if(clients[i].defi_de == parti.sock)
      {
         clients[i].defi_de = INVALID_SOCKET;
      }
      if(clients[i].adversaire == parti.sock)
      {
         clients[i].adversaire = INVALID_SOCKET;
         snprintf(message, sizeof message, "%s s'est déconnecté : la partie est annulée", parti.name);
         write_client(clients[i].sock, message);
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

   /* lets us restart the server right away : without it bind() fails with "Address already
      in use" while connections of the previous run are still closing (TIME_WAIT) */
   int reuse = 1;
   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof reuse);

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
   /* a failing send must not stop the server : the client will be removed when its
      socket is seen closed (recv returns 0) */
   /* TCP is a stream : two messages sent quickly arrive glued together. Each message ends
      with '\n' so that the client can tell them apart */
   char message[BUF_SIZE + 1];
   snprintf(message, sizeof message, "%s\n", buffer);
   if(send(sock, message, strlen(message), MSG_NOSIGNAL) < 0)
   {
      perror("send()");
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
