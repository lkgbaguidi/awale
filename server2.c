#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "server2.h"

/* the games played on the server (en_cours = 0 for a free place) */
static PartieReseau parties[MAX_PARTIES];

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

         /* after connecting the client sends its name and its password */
         if(read_client(csock, buffer) == 0)
         {
            /* disconnected */
            closesocket(csock);
            continue;
         }

         /* the first message is "pseudo motdepasse" : we cut it at the first space */
         char *espace = strchr(buffer, ' ');
         if(espace == NULL)
         {
            write_client(csock, "ERREUR : il faut un pseudo et un mot de passe");
            closesocket(csock);
            continue;
         }
         *espace = 0;   /* now buffer is the pseudo and espace + 1 the password */

         /* the name must be valid and not already used, then the password must match */
         const char *erreur = erreur_pseudo(clients, actual, buffer);
         if(erreur == NULL)
         {
            erreur = verifier_compte(buffer, espace + 1);
         }
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
         c.partie = -1;
         c.observe = -1;
         charger_profil(&c);   /* bio, friends and private mode saved by a previous connection */
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
                  /* a player who leaves loses his game, an observer stops watching */
                  if(clients[i].partie != -1)
                  {
                     abandonner_partie(clients, actual, i, 1);
                  }
                  if(clients[i].observe != -1)
                  {
                     retirer_observateur(clients[i].observe, clients[i].sock);
                  }
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
   if(pseudo[0] == 0)
   {
      return "ERREUR : le pseudo ne doit pas être vide";
   }
   if(strlen(pseudo) >= TAILLE_PSEUDO)
   {
      return "ERREUR : pseudo trop long";
   }
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

/* Accounts are kept in comptes.txt, one line per account : "pseudo motdepasse".
   Unknown pseudo : the account is created. Known pseudo : the password must match.
   Returns NULL if the client can come in, otherwise the message to send to him. */
static const char *verifier_compte(const char *pseudo, const char *mdp)
{
   char p[TAILLE_PSEUDO], m[TAILLE_PSEUDO];
   FILE *f = NULL;

   if(mdp[0] == 0 || strlen(mdp) >= TAILLE_PSEUDO || strchr(mdp, ' ') != NULL)
   {
      return "ERREUR : mot de passe invalide (pas vide, pas d'espace, 31 caractères max)";
   }

   f = fopen("comptes.txt", "r");
   if(f != NULL)   /* NULL the first time : the file does not exist yet */
   {
      while(fscanf(f, "%31s %31s", p, m) == 2)
      {
         if(strcmp(p, pseudo) == 0)
         {
            fclose(f);
            if(strcmp(m, mdp) == 0)
            {
               return NULL;
            }
            return "ERREUR : mot de passe incorrect";
         }
      }
      fclose(f);
   }

   /* unknown pseudo : we create the account */
   f = fopen("comptes.txt", "a");
   if(f == NULL)
   {
      return "ERREUR : impossible de créer le compte";
   }
   fprintf(f, "%s %s\n", pseudo, mdp);
   fclose(f);
   return NULL;
}

/* The profile of an account is kept in profil_<pseudo>.txt, one piece of information per line :
      prive 1
      ami bob
      bio J'aime l'awalé
   It is written again after each change (/bio, /clearbio, /friend, /private). */
static void sauver_profil(const Client *c)
{
   char nom_fichier[64];
   int i = 0;
   FILE *f = NULL;

   snprintf(nom_fichier, sizeof nom_fichier, "profil_%s.txt", c->name);
   f = fopen(nom_fichier, "w");
   if(f == NULL)
   {
      return;
   }
   fprintf(f, "prive %d\n", c->prive);
   for(i = 0; i < c->nb_amis; i++)
   {
      fprintf(f, "ami %s\n", c->amis[i]);
   }
   for(i = 0; i < c->nb_lignes_bio; i++)
   {
      fprintf(f, "bio %s\n", c->bio[i]);
   }
   fclose(f);
}

/* reads back the profile written by sauver_profil (nothing happens for a new account) */
static void charger_profil(Client *c)
{
   char nom_fichier[64];
   char ligne[BUF_SIZE];
   FILE *f = NULL;

   snprintf(nom_fichier, sizeof nom_fichier, "profil_%s.txt", c->name);
   f = fopen(nom_fichier, "r");
   if(f == NULL)
   {
      return;
   }
   while(fgets(ligne, sizeof ligne, f) != NULL)
   {
      ligne[strcspn(ligne, "\n")] = 0;   /* remove the '\n' kept by fgets */
      if(strncmp(ligne, "prive ", 6) == 0)
      {
         c->prive = atoi(ligne + 6);
      }
      else if(strncmp(ligne, "ami ", 4) == 0 && c->nb_amis < MAX_AMIS)
      {
         strncpy(c->amis[c->nb_amis], ligne + 4, TAILLE_PSEUDO - 1);
         c->nb_amis++;
      }
      else if(strncmp(ligne, "bio ", 4) == 0 && c->nb_lignes_bio < LIGNES_BIO)
      {
         strncpy(c->bio[c->nb_lignes_bio], ligne + 4, TAILLE_LIGNE_BIO - 1);
         c->nb_lignes_bio++;
      }
   }
   fclose(f);
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
         if(clients[i].partie != -1)
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
   else if(strcmp(commande, "/help") == 0)
   {
      envoyer_texte(clients[index].sock,
         "/list : joueurs en ligne\n"
         "/challenge <pseudo> : défier un joueur, /accept ou /refuse pour répondre à un défi\n"
         "/play <1-6> : jouer une case (de gauche à droite pour le joueur du bas, de droite à gauche pour celui du haut)\n"
         "/quit : abandonner la partie en cours\n"
         "/games : parties en cours, /watch <numéro> : observer une partie, /unwatch : arrêter\n"
         "/msg <pseudo> <texte> : message privé (un message sans / est envoyé à tout le monde)\n"
         "/bio <ligne> : ajouter une ligne à sa bio (10 max), /clearbio : l'effacer, /showbio <pseudo> : la lire\n"
         "/friend <pseudo> : ajouter un ami, /friend : voir ses amis\n"
         "/private : activer/désactiver le mode privé (seuls vos amis peuvent observer vos parties)\n"
         "/history : parties sauvegardées, /review <numéro> : revoir une partie");
   }
   else if(strcmp(commande, "/play") == 0)
   {
      write_client(clients[index].sock, "Usage : /play <1-6>");
   }
   else if(strncmp(commande, "/play ", 6) == 0)
   {
      commande_play(clients, actual, index, commande + 6);
   }
   else if(strcmp(commande, "/quit") == 0)
   {
      if(clients[index].partie == -1)
      {
         write_client(clients[index].sock, "ERREUR : vous n'êtes pas en partie");
      }
      else
      {
         abandonner_partie(clients, actual, index, 0);
      }
   }
   else if(strcmp(commande, "/games") == 0)
   {
      commande_games(clients, actual, index);
   }
   else if(strncmp(commande, "/watch ", 7) == 0)
   {
      commande_watch(clients, actual, index, commande + 7);
   }
   else if(strcmp(commande, "/unwatch") == 0)
   {
      commande_unwatch(clients, index);
   }
   else if(strncmp(commande, "/msg ", 5) == 0)
   {
      commande_msg(clients, actual, index, commande + 5);
   }
   else if(strncmp(commande, "/bio ", 5) == 0)
   {
      commande_bio(clients, index, commande + 5);
   }
   else if(strcmp(commande, "/clearbio") == 0)
   {
      clients[index].nb_lignes_bio = 0;
      sauver_profil(&clients[index]);
      write_client(clients[index].sock, "Bio effacée");
   }
   else if(strncmp(commande, "/showbio ", 9) == 0)
   {
      commande_showbio(clients, actual, index, commande + 9);
   }
   else if(strcmp(commande, "/friend") == 0)
   {
      commande_friend(clients, index, "");
   }
   else if(strncmp(commande, "/friend ", 8) == 0)
   {
      commande_friend(clients, index, commande + 8);
   }
   else if(strcmp(commande, "/private") == 0)
   {
      commande_private(clients, index);
   }
   else if(strcmp(commande, "/history") == 0)
   {
      commande_history(clients, index);
   }
   else if(strncmp(commande, "/review ", 8) == 0)
   {
      commande_review(clients, index, commande + 8);
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

   if(clients[index].partie != -1)
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
   if(clients[cible].partie != -1)
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
   if(clients[defieur].partie != -1)
   {
      clients[index].defi_de = INVALID_SOCKET;
      snprintf(message, sizeof message, "ERREUR : %s est déjà en partie", clients[defieur].name);
      write_client(clients[index].sock, message);
      return;
   }

   /* we look for a free place in the array of games */
   int n = 0;
   while(n < MAX_PARTIES && parties[n].en_cours)
   {
      n++;
   }
   if(n == MAX_PARTIES)
   {
      write_client(clients[index].sock, "ERREUR : trop de parties en cours, réessayez plus tard");
      return;
   }

   /* a pending challenge is no longer possible for the two players */
   clients[index].defi_de = INVALID_SOCKET;
   clients[defieur].defi_de = INVALID_SOCKET;

   /* the game : the challenger has the bottom cases, the other one the top cases */
   PartieReseau *p = &parties[n];
   p->joueurs[0] = clients[defieur].sock;
   p->joueurs[1] = clients[index].sock;
   strncpy(p->noms[0], clients[defieur].name, TAILLE_PSEUDO - 1);
   strncpy(p->noms[1], clients[index].name, TAILLE_PSEUDO - 1);
   p->nb_observateurs = 0;
   clients[defieur].partie = n;
   clients[index].partie = n;

   /* random choice of who starts */
   partie_init(&p->jeu, rand() % 2);

   /* the game is saved in the first free file partie_1.txt, partie_2.txt, ... */
   char nom_fichier[64];
   int numero = 1;
   while(1)
   {
      snprintf(nom_fichier, sizeof nom_fichier, "partie_%d.txt", numero);
      FILE *f = fopen(nom_fichier, "r");
      if(f == NULL && trouver_partie(numero) == -1)
      {
         break;
      }
      if(f != NULL)
      {
         fclose(f);
      }
      numero++;
   }
   p->numero = numero;
   p->sauvegarde = fopen(nom_fichier, "w");
   p->en_cours = 1;

   const char *premier = p->noms[p->jeu.courant];
   snprintf(message, sizeof message, "Partie %d lancée : %s (en bas) contre %s (en haut). %s commence !",
            numero, p->noms[0], p->noms[1], premier);
   envoyer_a_la_partie(n, message);
   envoyer_plateau(n);
   snprintf(message, sizeof message, "C'est au tour de %s (/play <1-6>)", premier);
   envoyer_a_la_partie(n, message);
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

/* parti has just left : nobody must keep a pending challenge from its (now closed) socket.
   (its game, if any, was already ended by abandonner_partie) */
static void oublier_client(Client *clients, int actual, Client parti)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      if(clients[i].defi_de == parti.sock)
      {
         clients[i].defi_de = INVALID_SOCKET;
      }
   }
}

/* sends a text of several lines, one line at a time (write_client adds the '\n') */
static void envoyer_texte(SOCKET sock, const char *texte)
{
   char copie[TAILLE_PLATEAU];
   char *ligne = NULL;

   if(sock == INVALID_SOCKET)
   {
      return;
   }
   strncpy(copie, texte, sizeof copie - 1);
   copie[sizeof copie - 1] = 0;

   ligne = strtok(copie, "\n");
   while(ligne != NULL)
   {
      write_client(sock, ligne);
      ligne = strtok(NULL, "\n");
   }
}

/* index in the array parties of the game with this number, -1 if there is none */
static int trouver_partie(int numero)
{
   int i = 0;
   for(i = 0; i < MAX_PARTIES; i++)
   {
      if(parties[i].en_cours && parties[i].numero == numero)
      {
         return i;
      }
   }
   return -1;
}

/* sends texte to the players and observers of game n, and writes it in its save file */
static void envoyer_a_la_partie(int n, const char *texte)
{
   PartieReseau *p = &parties[n];
   int i = 0;

   envoyer_texte(p->joueurs[0], texte);
   envoyer_texte(p->joueurs[1], texte);
   for(i = 0; i < p->nb_observateurs; i++)
   {
      envoyer_texte(p->observateurs[i], texte);
   }

   if(p->sauvegarde != NULL)
   {
      fprintf(p->sauvegarde, "%s\n", texte);
      /* written right away, so that the game can be reviewed while it is played */
      fflush(p->sauvegarde);
   }
}

static void envoyer_plateau(int n)
{
   char texte[TAILLE_PLATEAU];
   plateau_texte(&parties[n].jeu, parties[n].noms[1], parties[n].noms[0], texte);
   envoyer_a_la_partie(n, texte);
}

/* end of game n : resultat is sent to everybody, the save file is closed and the
   players and observers are free again */
static void terminer_partie(Client *clients, int actual, int n, const char *resultat)
{
   char message[BUF_SIZE];
   PartieReseau *p = &parties[n];
   int i = 0;
   int c = 0;

   envoyer_a_la_partie(n, resultat);
   if(p->sauvegarde != NULL)
   {
      fclose(p->sauvegarde);
      p->sauvegarde = NULL;
   }

   snprintf(message, sizeof message, "Partie sauvegardée, pour la revoir : /review %d", p->numero);
   for(i = 0; i < 2; i++)
   {
      c = trouver_client_par_sock(clients, actual, p->joueurs[i]);
      if(c != -1)
      {
         clients[c].partie = -1;
         write_client(clients[c].sock, message);
      }
   }
   for(i = 0; i < p->nb_observateurs; i++)
   {
      c = trouver_client_par_sock(clients, actual, p->observateurs[i]);
      if(c != -1)
      {
         clients[c].observe = -1;
      }
   }

   p->nb_observateurs = 0;
   p->en_cours = 0;
}

/* the client at index leaves his game (/quit or disconnection) : the other player wins */
static void abandonner_partie(Client *clients, int actual, int index, int deconnecte)
{
   char message[BUF_SIZE];
   int n = clients[index].partie;
   PartieReseau *p = &parties[n];
   int moi = (p->joueurs[0] == clients[index].sock) ? 0 : 1;

   if(deconnecte)
   {
      /* his socket is closed : we don't write to it anymore */
      p->joueurs[moi] = INVALID_SOCKET;
      clients[index].partie = -1;
      snprintf(message, sizeof message, "%s s'est déconnecté : %s gagne par forfait !", p->noms[moi], p->noms[1 - moi]);
   }
   else
   {
      snprintf(message, sizeof message, "%s abandonne : %s gagne par forfait !", p->noms[moi], p->noms[1 - moi]);
   }
   terminer_partie(clients, actual, n, message);
}

static void retirer_observateur(int n, SOCKET sock)
{
   PartieReseau *p = &parties[n];
   int i = 0;
   for(i = 0; i < p->nb_observateurs; i++)
   {
      if(p->observateurs[i] == sock)
      {
         /* the last observer takes its place */
         p->observateurs[i] = p->observateurs[p->nb_observateurs - 1];
         p->nb_observateurs--;
         return;
      }
   }
}

/* /play n : the client at index plays the case n of his side */
static void commande_play(Client *clients, int actual, int index, const char *arg)
{
   char message[BUF_SIZE];
   int n = clients[index].partie;

   if(n == -1)
   {
      write_client(clients[index].sock, "ERREUR : vous n'êtes pas en partie");
      return;
   }

   PartieReseau *p = &parties[n];
   int joueur = (p->joueurs[0] == clients[index].sock) ? 0 : 1;
   int tas = atoi(arg) - 1;

   if(p->jeu.courant != joueur)
   {
      write_client(clients[index].sock, "ERREUR : ce n'est pas votre tour");
      return;
   }
   if(tas < 0 || tas >= NB_CASES)
   {
      write_client(clients[index].sock, "ERREUR : choisissez une case entre 1 et 6");
      return;
   }
   if(p->jeu.cases[joueur * NB_CASES + tas] == 0)
   {
      write_client(clients[index].sock, "ERREUR : cette case est vide");
      return;
   }
   if(!coup_valide(&p->jeu, joueur, tas))
   {
      write_client(clients[index].sock, "ERREUR : votre adversaire n'a plus de graines, vous devez lui en donner");
      return;
   }

   jouer_coup(&p->jeu, tas);
   snprintf(message, sizeof message, "%s joue la case %d", p->noms[joueur], tas + 1);
   envoyer_a_la_partie(n, message);

   if(fin_jeu(&p->jeu))
   {
      /* blocked game (and not 25 points) : each player keeps the seeds of his side */
      if(p->jeu.points[0] < 25 && p->jeu.points[1] < 25)
      {
         ramasser_restantes(&p->jeu);
      }
      envoyer_plateau(n);

      int g = gagnant(&p->jeu);
      if(g == -1)
      {
         snprintf(message, sizeof message, "Fin de la partie : %s %d - %d %s. Égalité !",
                  p->noms[0], p->jeu.points[0], p->jeu.points[1], p->noms[1]);
      }
      else
      {
         snprintf(message, sizeof message, "Fin de la partie : %s %d - %d %s. %s gagne !",
                  p->noms[0], p->jeu.points[0], p->jeu.points[1], p->noms[1], p->noms[g]);
      }
      terminer_partie(clients, actual, n, message);
   }
   else
   {
      envoyer_plateau(n);
      snprintf(message, sizeof message, "C'est au tour de %s (/play <1-6>)", p->noms[p->jeu.courant]);
      envoyer_a_la_partie(n, message);
   }
}

/* 1 if nom is in the friends of client */
static int est_ami(const Client *client, const char *nom)
{
   int i = 0;
   for(i = 0; i < client->nb_amis; i++)
   {
      if(strcmp(client->amis[i], nom) == 0)
      {
         return 1;
      }
   }
   return 0;
}

/* 1 if nom can watch game n : a player in private mode only accepts his friends */
static int peut_observer(const Client *clients, int actual, int n, const char *nom)
{
   int i = 0;
   for(i = 0; i < 2; i++)
   {
      int c = trouver_client_par_sock(clients, actual, parties[n].joueurs[i]);
      if(c != -1 && clients[c].prive && strcmp(clients[c].name, nom) != 0 && !est_ami(&clients[c], nom))
      {
         return 0;
      }
   }
   return 1;
}

static void commande_games(Client *clients, int actual, int index)
{
   char message[BUF_SIZE];
   int i = 0;
   int nb = 0;

   for(i = 0; i < MAX_PARTIES; i++)
   {
      if(parties[i].en_cours)
      {
         snprintf(message, sizeof message, "Partie %d : %s (%d) contre %s (%d)%s",
                  parties[i].numero, parties[i].noms[0], parties[i].jeu.points[0],
                  parties[i].noms[1], parties[i].jeu.points[1],
                  peut_observer(clients, actual, i, clients[index].name) ? "" : " [privée]");
         write_client(clients[index].sock, message);
         nb++;
      }
   }
   if(nb == 0)
   {
      write_client(clients[index].sock, "Aucune partie en cours");
   }
}

/* /watch numero : the client at index watches a game */
static void commande_watch(Client *clients, int actual, int index, const char *arg)
{
   char message[BUF_SIZE];
   int n = trouver_partie(atoi(arg));

   if(n == -1)
   {
      write_client(clients[index].sock, "ERREUR : aucune partie en cours avec ce numéro (voir /games)");
      return;
   }
   if(clients[index].partie == n)
   {
      write_client(clients[index].sock, "ERREUR : vous jouez dans cette partie");
      return;
   }
   if(clients[index].observe == n)
   {
      write_client(clients[index].sock, "ERREUR : vous observez déjà cette partie");
      return;
   }
   if(!peut_observer(clients, actual, n, clients[index].name))
   {
      write_client(clients[index].sock, "ERREUR : partie privée, seuls les amis des joueurs peuvent l'observer");
      return;
   }
   if(parties[n].nb_observateurs == MAX_OBSERVATEURS)
   {
      write_client(clients[index].sock, "ERREUR : il y a déjà trop d'observateurs");
      return;
   }

   /* we can only watch one game at a time */
   if(clients[index].observe != -1)
   {
      retirer_observateur(clients[index].observe, clients[index].sock);
   }
   parties[n].observateurs[parties[n].nb_observateurs] = clients[index].sock;
   parties[n].nb_observateurs++;
   clients[index].observe = n;

   char texte[TAILLE_PLATEAU];
   snprintf(message, sizeof message, "Vous observez la partie %d (/unwatch pour arrêter)", parties[n].numero);
   write_client(clients[index].sock, message);
   plateau_texte(&parties[n].jeu, parties[n].noms[1], parties[n].noms[0], texte);
   envoyer_texte(clients[index].sock, texte);
   snprintf(message, sizeof message, "C'est au tour de %s", parties[n].noms[parties[n].jeu.courant]);
   write_client(clients[index].sock, message);
}

static void commande_unwatch(Client *clients, int index)
{
   if(clients[index].observe == -1)
   {
      write_client(clients[index].sock, "ERREUR : vous n'observez aucune partie");
      return;
   }
   retirer_observateur(clients[index].observe, clients[index].sock);
   clients[index].observe = -1;
   write_client(clients[index].sock, "Vous n'observez plus la partie");
}

/* /msg pseudo texte : private message */
static void commande_msg(Client *clients, int actual, int index, const char *arg)
{
   char message[BUF_SIZE];
   char nom[BUF_SIZE];
   const char *espace = strchr(arg, ' ');

   if(espace == NULL)
   {
      write_client(clients[index].sock, "Usage : /msg <pseudo> <texte>");
      return;
   }

   /* the name is what is before the first space */
   strncpy(nom, arg, espace - arg);
   nom[espace - arg] = 0;

   int cible = trouver_client(clients, actual, nom);
   if(cible == -1)
   {
      snprintf(message, sizeof message, "ERREUR : joueur inconnu : %s", nom);
      write_client(clients[index].sock, message);
      return;
   }

   snprintf(message, sizeof message, "[privé] %s : %s", clients[index].name, espace + 1);
   write_client(clients[cible].sock, message);
   snprintf(message, sizeof message, "Message envoyé à %s", nom);
   write_client(clients[index].sock, message);
}

/* /bio ligne : adds a line to the bio */
static void commande_bio(Client *clients, int index, const char *ligne)
{
   Client *c = &clients[index];
   if(c->nb_lignes_bio == LIGNES_BIO)
   {
      write_client(c->sock, "ERREUR : votre bio a déjà 10 lignes (/clearbio pour l'effacer)");
      return;
   }
   strncpy(c->bio[c->nb_lignes_bio], ligne, TAILLE_LIGNE_BIO - 1);
   c->bio[c->nb_lignes_bio][TAILLE_LIGNE_BIO - 1] = 0;
   c->nb_lignes_bio++;
   sauver_profil(c);
   write_client(c->sock, "Ligne ajoutée à votre bio");
}

static void commande_showbio(Client *clients, int actual, int index, const char *nom)
{
   char message[BUF_SIZE];
   int c = trouver_client(clients, actual, nom);
   int i = 0;

   if(c == -1)
   {
      snprintf(message, sizeof message, "ERREUR : joueur inconnu : %s", nom);
      write_client(clients[index].sock, message);
      return;
   }
   if(clients[c].nb_lignes_bio == 0)
   {
      snprintf(message, sizeof message, "%s n'a pas encore écrit de bio", nom);
      write_client(clients[index].sock, message);
      return;
   }

   snprintf(message, sizeof message, "Bio de %s :", nom);
   write_client(clients[index].sock, message);
   for(i = 0; i < clients[c].nb_lignes_bio; i++)
   {
      write_client(clients[index].sock, clients[c].bio[i]);
   }
}

/* /friend nom : adds a friend, /friend alone : shows the friends */
static void commande_friend(Client *clients, int index, const char *nom)
{
   char message[BUF_SIZE];
   Client *c = &clients[index];
   int i = 0;

   if(nom[0] == 0)
   {
      message[0] = 0;
      strncat(message, "Vos amis :", sizeof message - 1);
      for(i = 0; i < c->nb_amis; i++)
      {
         strncat(message, " ", sizeof message - strlen(message) - 1);
         strncat(message, c->amis[i], sizeof message - strlen(message) - 1);
      }
      write_client(c->sock, message);
      return;
   }
   if(strcmp(nom, c->name) == 0)
   {
      write_client(c->sock, "ERREUR : vous ne pouvez pas être votre propre ami");
      return;
   }
   if(strlen(nom) >= TAILLE_PSEUDO)
   {
      write_client(c->sock, "ERREUR : pseudo trop long");
      return;
   }
   if(est_ami(c, nom))
   {
      snprintf(message, sizeof message, "%s est déjà votre ami", nom);
      write_client(c->sock, message);
      return;
   }
   if(c->nb_amis == MAX_AMIS)
   {
      write_client(c->sock, "ERREUR : vous avez déjà trop d'amis");
      return;
   }

   strncpy(c->amis[c->nb_amis], nom, TAILLE_PSEUDO - 1);
   c->amis[c->nb_amis][TAILLE_PSEUDO - 1] = 0;
   c->nb_amis++;
   sauver_profil(c);
   snprintf(message, sizeof message, "%s ajouté à vos amis", nom);
   write_client(c->sock, message);
}

static void commande_private(Client *clients, int index)
{
   clients[index].prive = !clients[index].prive;
   sauver_profil(&clients[index]);
   if(clients[index].prive)
   {
      write_client(clients[index].sock, "Mode privé activé : seuls vos amis peuvent observer vos parties");
   }
   else
   {
      write_client(clients[index].sock, "Mode privé désactivé : tout le monde peut observer vos parties");
   }
}

/* /history : first line of every saved game */
static void commande_history(Client *clients, int index)
{
   char nom_fichier[64];
   char ligne[BUF_SIZE];
   int numero = 1;

   while(1)
   {
      snprintf(nom_fichier, sizeof nom_fichier, "partie_%d.txt", numero);
      FILE *f = fopen(nom_fichier, "r");
      if(f == NULL)
      {
         break;
      }
      if(fgets(ligne, sizeof ligne, f) != NULL)
      {
         ligne[strcspn(ligne, "\n")] = 0;
         write_client(clients[index].sock, ligne);
      }
      fclose(f);
      numero++;
   }
   if(numero == 1)
   {
      write_client(clients[index].sock, "Aucune partie sauvegardée");
   }
}

/* /review numero : sends the whole save file of a game */
static void commande_review(Client *clients, int index, const char *arg)
{
   char nom_fichier[64];
   char ligne[BUF_SIZE];

   snprintf(nom_fichier, sizeof nom_fichier, "partie_%d.txt", atoi(arg));
   FILE *f = fopen(nom_fichier, "r");
   if(f == NULL)
   {
      write_client(clients[index].sock, "ERREUR : pas de partie sauvegardée avec ce numéro (voir /history)");
      return;
   }
   while(fgets(ligne, sizeof ligne, f) != NULL)
   {
      ligne[strcspn(ligne, "\n")] = 0;
      write_client(clients[index].sock, ligne);
   }
   fclose(f);
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
