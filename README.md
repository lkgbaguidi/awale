# Serveur de jeu d'Awalé

TP 2/3/4 de Programmation réseaux (4IF, INSA Lyon).

Un serveur qui permet à des clients de discuter et de jouer des parties d'Awalé entre eux.
Le serveur vérifie les coups, compte les points et sauvegarde les parties.

## Compilation

Sous Linux :

    make

On obtient trois programmes :

- `server` : le serveur de jeu
- `client` : le client
- `awale` : le jeu en local, à deux sur le même terminal (pour tester les règles)

`make clean` supprime les programmes.

## Lancement

Le serveur écoute sur le port 1977 :

    ./server

Taper une touche puis Entrée dans le terminal du serveur l'arrête.

Chaque joueur lance un client avec l'adresse du serveur, son pseudo et son mot de passe :

    ./client 127.0.0.1 alice secret

Le pseudo ne doit pas être vide, ne doit pas contenir d'espace, doit faire moins de 32 caractères
et ne doit pas déjà être connecté. Mêmes règles pour le mot de passe. Ctrl+D quitte le client.

## Comptes

Le premier message du client est `pseudo motdepasse`. Le serveur cherche le pseudo dans
`comptes.txt` (une ligne `pseudo motdepasse` par compte) :

- pseudo inconnu : le compte est créé ;
- pseudo connu et bon mot de passe : le joueur entre ;
- mauvais mot de passe : `ERREUR : mot de passe incorrect` et la connexion est fermée.

La bio, les amis et le mode privé d'un compte sont sauvegardés dans `profil_<pseudo>.txt`
après chaque modification, et rechargés à la connexion suivante.

## Commandes du client

Une ligne qui ne commence pas par `/` est envoyée à tous les joueurs connectés (chat général,
pendant une partie ou non).

| Commande | Effet |
|---|---|
| `/help` | liste des commandes avec leur description |
| `/list` | joueurs en ligne (avec "(en partie)" s'ils jouent) |
| `/challenge <pseudo>` | défier un joueur |
| `/accept` / `/refuse` | accepter ou refuser le défi reçu |
| `/play <1-6>` | jouer une case de son camp |
| `/quit` | abandonner la partie (l'adversaire gagne par forfait) |
| `/games` | parties en cours avec leur numéro et le score |
| `/watch <numéro>` | observer une partie |
| `/unwatch` | arrêter d'observer |
| `/msg <pseudo> <texte>` | message privé à un joueur |
| `/bio <ligne>` | ajouter une ligne à sa bio (10 lignes maximum) |
| `/clearbio` | effacer sa bio |
| `/showbio <pseudo>` | afficher la bio d'un joueur |
| `/friend <pseudo>` | ajouter un ami ; `/friend` seul affiche la liste |
| `/private` | activer / désactiver le mode privé |
| `/history` | liste des parties sauvegardées |
| `/review <numéro>` | revoir une partie sauvegardée |

## Déroulement d'une partie

1. A tape `/challenge B`, B reçoit le défi et répond `/accept` ou `/refuse`.
2. Le serveur crée la partie : celui qui a lancé le défi a les cases du bas, l'autre celles du haut.
   Le serveur tire au sort qui commence.
3. À chaque coup, le plateau et le score sont envoyés aux deux joueurs et aux observateurs.
   Les graines sont affichées par des points.
4. Les cases sont numérotées de 1 à 6 dans le sens du jeu : de gauche à droite pour le joueur
   du bas, de droite à gauche pour le joueur du haut.
5. Le serveur refuse les coups interdits : pas son tour, case vide, numéro invalide, ou coup qui
   ne nourrit pas un adversaire qui n'a plus de graines.

Règles appliquées : semis dans le sens inverse des aiguilles d'une montre en sautant la case
de départ, prise des cases à 2 ou 3 graines chez l'adversaire, pas de prise si elle enlèverait
toutes les graines de l'adversaire, obligation de nourrir. La partie finit quand un joueur a
25 points ou quand le joueur dont c'est le tour ne peut plus jouer. Dans ce cas, chacun garde
les graines de son camp.

Plusieurs parties peuvent se jouer en même temps (50 au maximum).

## Observateurs et mode privé

`/games` donne le numéro des parties en cours, `/watch <numéro>` permet de les suivre
(10 observateurs par partie au maximum, une seule partie observée à la fois).

Un joueur en mode privé (`/private`) n'accepte comme observateurs que ses amis (`/friend`).
Les parties concernées apparaissent avec "[privée]" dans `/games` pour ceux qui ne peuvent pas
les observer.

## Sauvegarde des parties

Chaque partie est enregistrée par le serveur dans un fichier `partie_<numéro>.txt` (dans le
dossier où le serveur est lancé) : coups joués, plateau après chaque coup et résultat.
`/history` affiche la liste et `/review <numéro>` renvoie tout le fichier.

## Déconnexion

Si un joueur se déconnecte pendant une partie, il perd par forfait : l'adversaire et les
observateurs sont prévenus et la partie est sauvegardée. Les défis en attente avec lui sont
annulés.

## Limites

- Le mot de passe circule en clair sur le réseau (TCP ne chiffre rien ; c'est le rôle de
  SSH/TLS) et il est stocké en clair dans `comptes.txt` (il faudrait stocker une empreinte,
  par exemple avec `crypt()`). Il est aussi visible dans les arguments de la commande.
- Un joueur déconnecté ne peut pas reprendre sa partie.

## Fichiers

- `jeu.c`, `jeu.h` : règles de l'Awalé (plateau, coups, points, affichage du plateau)
- `awale.c` : jeu local en terminal
- `server2.c`, `server2.h` : serveur (connexions, commandes, parties)
- `client2.c`, `client2.h` : client

Fichiers créés par le serveur dans le dossier où il est lancé : `comptes.txt`,
`profil_<pseudo>.txt` et `partie_<numéro>.txt`.
