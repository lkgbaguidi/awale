#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#include <stdio.h>
#include "jeu.h"

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100

#define BUF_SIZE    1024

#define TAILLE_PSEUDO      32    /* longueur max d'un pseudo */
#define MAX_PARTIES        50    /* parties jouées en même temps */
#define MAX_OBSERVATEURS   10    /* observateurs par partie */
#define MAX_AMIS           20
#define LIGNES_BIO         10
#define TAILLE_LIGNE_BIO  100

/* MSG_NOSIGNAL : empêche send() de tuer le serveur (SIGPIPE) quand un client est déjà parti.
   Il n'existe pas sous Windows, où on le remplace par 0 (sans effet). */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* texte d'aide : envoyé à l'arrivée et pour une commande inconnue */
#define COMMANDES "/help /list /challenge /accept /refuse /play /quit /games /watch /unwatch /msg /bio /clearbio /showbio /friend /private /history /review"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   /* socket du joueur qui nous a défiés (INVALID_SOCKET s'il n'y en a pas) */
   SOCKET defi_de;
   /* socket de notre adversaire si on est en partie (INVALID_SOCKET sinon) */
   SOCKET adversaire;
   /* indice (dans le tableau des parties) de la partie qu'on joue ou qu'on observe, -1 sinon */
   int partie;
   int observe;
   /* bio : jusqu'à LIGNES_BIO lignes */
   char bio[LIGNES_BIO][TAILLE_LIGNE_BIO];
   int nb_lignes_bio;
   /* amis : les seuls qui peuvent observer nos parties en mode privé */
   char amis[MAX_AMIS][TAILLE_PSEUDO];
   int nb_amis;
   int prive;
} Client;

/* une partie jouée sur le serveur entre deux clients */
typedef struct
{
   int en_cours;                  /* 0 : la case du tableau est libre */
   int numero;                    /* numéro de la partie, aussi celui de son fichier de sauvegarde */
   Partie jeu;
   SOCKET joueurs[2];             /* joueurs[0] a les cases du bas, joueurs[1] celles du haut */
   char noms[2][TAILLE_PSEUDO];
   SOCKET observateurs[MAX_OBSERVATEURS];
   int nb_observateurs;
   FILE *sauvegarde;              /* fichier partie_<numero>.txt */
} PartieReseau;

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static const char *erreur_pseudo(const Client *clients, int actual, const char *pseudo);
static void traiter_commande(Client *clients, int actual, int index, const char *commande);
static int trouver_client(const Client *clients, int actual, const char *nom);
static int trouver_client_par_sock(const Client *clients, int actual, SOCKET sock);
static void commande_challenge(Client *clients, int actual, int index, const char *nom);
static void commande_accept(Client *clients, int actual, int index);
static void commande_refuse(Client *clients, int actual, int index);
static void oublier_client(Client *clients, int actual, Client parti);
static void envoyer_texte(SOCKET sock, const char *texte);
static int trouver_partie(int numero);
static void envoyer_a_la_partie(int n, const char *texte);
static void envoyer_plateau(int n);
static void terminer_partie(Client *clients, int actual, int n, const char *resultat);
static void abandonner_partie(Client *clients, int actual, int index, int deconnecte);
static void retirer_observateur(int n, SOCKET sock);
static void commande_play(Client *clients, int actual, int index, const char *arg);
static int est_ami(const Client *client, const char *nom);
static int peut_observer(const Client *clients, int actual, int n, const char *nom);
static void commande_games(Client *clients, int actual, int index);
static void commande_watch(Client *clients, int actual, int index, const char *arg);
static void commande_unwatch(Client *clients, int index);
static void commande_msg(Client *clients, int actual, int index, const char *arg);
static void commande_bio(Client *clients, int index, const char *ligne);
static void commande_showbio(Client *clients, int actual, int index, const char *nom);
static void commande_friend(Client *clients, int index, const char *nom);
static void commande_private(Client *clients, int index);
static void commande_history(Client *clients, int index);
static void commande_review(Client *clients, int index, const char *arg);

#endif /* guard */
