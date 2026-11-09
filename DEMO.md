# Scénario de démonstration (~10 minutes)

## Préparation (avant la démo)

    make clean && make
    rm -f partie_*.txt        # repartir sans parties sauvegardées

Ouvrir 5 terminaux dans le dossier du projet :

| Terminal | Rôle | Commande |
|---|---|---|
| T1 | serveur | `./server` (ne rien taper dedans : une ligne l'arrête) |
| T2 | alice | `./client 127.0.0.1 alice` |
| T3 | bob | `./client 127.0.0.1 bob` |
| T4 | carol | `./client 127.0.0.1 carol` |
| T5 | dave | lancé plus tard |

Lancer T1, puis T2, T3, T4 dans cet ordre.

Les commandes se tapent **sans espace au début** de la ligne.

---

## 1. Inscription et pseudos (1 min)

| Où | Taper | Résultat attendu |
|---|---|---|
| T5 | `./client 127.0.0.1 alice` | `ERREUR : pseudo déjà pris`, le client s'arrête |
| T5 | `./client 127.0.0.1 "da ve"` | `ERREUR : le pseudo ne doit pas contenir d'espace` |
| T5 | `./client 127.0.0.1 dave` | `Bienvenue ! ...`, les autres reçoivent `dave joined !` |
| T2 | `/list` | `Joueurs en ligne : alice bob carol dave` |
| T2 | `/help` | liste détaillée des commandes |

## 2. Chat, message privé, bio (1 min 30)

| Où | Taper | Résultat attendu |
|---|---|---|
| T2 | `bonjour tout le monde` | bob, carol et dave reçoivent `alice : bonjour tout le monde` |
| T2 | `/msg bob on fait une partie ?` | seul bob reçoit `[privé] alice : on fait une partie ?` |
| T2 | `/msg zorro salut` | `ERREUR : joueur inconnu : zorro` |
| T2 | `/bio Etudiante en 4IF` | `Ligne ajoutée à votre bio` |
| T2 | `/bio Je joue a l'awale depuis 2 ans` | `Ligne ajoutée à votre bio` |
| T4 | `/showbio alice` | `Bio de alice :` puis les 2 lignes |
| T4 | `/showbio bob` | `bob n'a pas encore écrit de bio` |

## 3. Défi : refus puis acceptation (1 min)

| Où | Taper | Résultat attendu |
|---|---|---|
| T2 | `/challenge alice` | `ERREUR : vous ne pouvez pas vous défier vous-même` |
| T2 | `/challenge bob` | bob reçoit `alice vous défie ! Tapez /accept ou /refuse` |
| T3 | `/refuse` | alice reçoit `bob a refusé votre défi` |
| T2 | `/challenge bob` | nouveau défi |
| T3 | `/accept` | les deux reçoivent `Partie 1 lancée : alice (en bas) contre bob (en haut). X commence !`, le plateau et `C'est au tour de X` |
| T4 | `/list` | `alice(en partie)` et `bob(en partie)` |

Expliquer le plateau : un `.` = une graine, 4 par case au départ. Les cases se numérotent
de 1 à 6 dans le sens du jeu : de gauche à droite pour alice (en bas), de droite à gauche
pour bob (en haut). Celui qui commence est tiré au sort par le serveur.

## 4. Jouer et vérification des coups (2 min)

Dans la suite, "J" est le joueur dont c'est le tour et "l'autre" son adversaire.

| Où | Taper | Résultat attendu |
|---|---|---|
| l'autre | `/play 1` | `ERREUR : ce n'est pas votre tour` |
| J | `/play 9` | `ERREUR : choisissez une case entre 1 et 6` |
| J | `/play 3` | `J joue la case 3`, nouveau plateau chez les deux, `C'est au tour de ...` |
| l'autre | `/play 2` | coup joué, c'est de nouveau à J |
| J | `/play 3` | `ERREUR : cette case est vide` (elle a été vidée au premier coup) |
| J | `/play 4` | coup joué |

Faire remarquer le semis dans le sens inverse des aiguilles d'une montre et le score
affiché à côté des noms. Pendant la partie, taper un message sans `/` montre que le chat
marche aussi en jeu.

## 5. Observateur et mode privé (2 min)

| Où | Taper | Résultat attendu |
|---|---|---|
| T4 | `/games` | `Partie 1 : alice (..) contre bob (..)` |
| T4 | `/watch 1` | `Vous observez la partie 1`, le plateau et le joueur dont c'est le tour |
| J | `/play <case>` | carol (T4) reçoit aussi le coup et le plateau |
| T2 | `/private` | `Mode privé activé : seuls vos amis peuvent observer vos parties` |
| T5 | `/games` | la partie apparaît avec `[privée]` |
| T5 | `/watch 1` | `ERREUR : partie privée, seuls les amis des joueurs peuvent l'observer` |
| T2 | `/friend dave` | `dave ajouté à vos amis` |
| T2 | `/friend` | `Vos amis : dave` |
| T5 | `/watch 1` | dave observe la partie |
| T5 | `/unwatch` | `Vous n'observez plus la partie` |

## 6. Plusieurs parties en même temps (1 min)

| Où | Taper | Résultat attendu |
|---|---|---|
| T4 | `/unwatch` | carol arrête d'observer |
| T4 | `/challenge dave` | défi envoyé |
| T5 | `/accept` | `Partie 2 lancée : carol (en bas) contre dave (en haut)...` |
| T2 | `/games` | les parties 1 et 2 sont listées |
| (J de chaque partie) | `/play <case>` | chaque partie avance sans gêner l'autre |

## 7. Fin de partie : abandon et déconnexion (1 min)

| Où | Taper | Résultat attendu |
|---|---|---|
| T5 | Ctrl+C (dave quitte) | carol reçoit `dave s'est déconnecté : carol gagne par forfait !` et `Partie sauvegardée, pour la revoir : /review 2` ; tout le monde reçoit `dave disconnected !` |
| T3 | `/quit` | `bob abandonne : alice gagne par forfait !` chez alice et bob, plus le message de sauvegarde |
| T2 | `/list` | alice et bob ne sont plus `(en partie)` |
| T2 | `/play 1` | `ERREUR : vous n'êtes pas en partie` |

Une partie peut aussi finir normalement (25 points ou joueur bloqué). C'est trop long pour
la démo : pour montrer les règles de fin (famine, rafle totale), utiliser le jeu local
`./awale` à la fin si on a le temps.

## 8. Parties sauvegardées (30 s)

| Où | Taper | Résultat attendu |
|---|---|---|
| T4 | `/history` | une ligne par partie (`Partie 1 lancée : alice ... contre bob ...`, `Partie 2 ...`) |
| T4 | `/review 1` | tous les coups de la partie 1 avec le plateau après chaque coup, puis le résultat |
| T4 | `/review 9` | `ERREUR : pas de partie sauvegardée avec ce numéro` |
| T1 (après avoir arrêté le serveur) | `cat partie_1.txt` | le même contenu, stocké dans un fichier texte |

## Points à présenter à l'oral

- **Architecture** : les règles du jeu sont dans `jeu.c` (sans affichage ni saisie). Le jeu
  local `awale.c` et le serveur `server2.c` les utilisent tous les deux.
- **Le serveur décide de tout** : tirage au sort, validité des coups, score. Le client
  envoie seulement des lignes de texte et affiche ce qu'il reçoit.
- **Protocole** : une ligne qui commence par `/` est une commande, sinon c'est un message de
  chat. Le serveur termine chaque message par `\n`.
- **Un seul processus** : `select()` surveille toutes les sockets, il n'y a pas de `fork`.
  Chaque partie est une case du tableau `parties`, et chaque client sait dans quelle partie il
  joue ou laquelle il observe.
- **Robustesse** : pseudos vérifiés, serveur plein refusé, `MSG_NOSIGNAL` pour que le serveur
  ne s'arrête pas si un client est déjà parti, forfait en cas de déconnexion.
- **Ajouter une commande** : une branche dans `traiter_commande` et une fonction `commande_xxx`.
- **Limites** : bio, amis et mode privé gardés en mémoire seulement ; pas de reprise de partie
  après une déconnexion.
