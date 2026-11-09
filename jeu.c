#include <stdio.h>
#include <string.h>

#include "jeu.h"

// Règles du jeu, sans affichage ni saisie : utilisées par le jeu local (awale.c)
// et par le serveur (server2.c).

int est_tout_a_zero(const int tab[], int taille) {
    for (int i =0; i < taille; i++) {
        if (tab[i] != 0) {
            return 0; // Dès qu'un élément est différent de 0, on renvoie faux
        }
    }
    return 1; // Tous les éléments sont égaux à 0
}

// Retourne le joueur (0 ou 1) à qui appartient la case d'indice pos
int camp_de(int pos){
    return pos / NB_CASES;
}

// Retourne 1 si toutes les cases du joueur sont vides
int camp_vide(const Partie* p, int joueur){
    return est_tout_a_zero(&p->cases[joueur * NB_CASES], NB_CASES);
}

// Nombre de graines dans le camp d'un joueur
int graines_du_camp(const Partie* p, int joueur){
    int total = 0;
    for (int i = 0; i < NB_CASES; i++)
        total += p->cases[joueur * NB_CASES + i];
    return total;
}


void partie_init(Partie* p, int premier){
    for (int i = 0; i < NB_TOTAL; i++)
        p->cases[i] = 4;

    p->points[0] = 0;
    p->points[1] = 0;
    p->courant = premier;
}

// Le coup est jouable si la case n'est pas vide et, quand l'adversaire n'a plus de graines,
// s'il lui en donne : depuis la case tas, avec graines graines, on atteint le camp
// adverse seulement si tas + graines >= NB_CASES.
int coup_valide(const Partie* p, int joueur, int tas){
    int graines = p->cases[joueur * NB_CASES + tas];

    if (graines == 0)
        return 0;
    if (camp_vide(p, 1 - joueur) && tas + graines < NB_CASES)
        return 0;
    return 1;
}

int peut_jouer(const Partie* p, int joueur){
    for (int tas = 0; tas < NB_CASES; tas++){
        if (coup_valide(p, joueur, tas))
            return 1;
    }
    return 0;
}

// La partie finit à 25 points, ou quand le joueur dont c'est le tour ne peut plus jouer
// (plus de graines, ou impossible de nourrir l'adversaire)
int fin_jeu(const Partie* p){
    return p->points[0] >= 25 || p->points[1] >= 25 || !peut_jouer(p, p->courant);
}

// Chaque joueur garde les graines de son camp
void ramasser_restantes(Partie* p){
    for (int j = 0; j < 2; j++){
        p->points[j] += graines_du_camp(p, j);
        for (int i = 0; i < NB_CASES; i++)
            p->cases[j * NB_CASES + i] = 0;
    }
}

// Retourne le joueur gagnant (0 ou 1), ou -1 en cas d'égalité
int gagnant(const Partie* p){
    if (p->points[0] == p->points[1])
        return -1;
    if (p->points[0] > p->points[1])
        return 0;
    return 1;
}

// Joue la case tas (entre 0 et 5) du joueur courant. Le coup doit être valide
// (vérifié avant avec coup_valide).
void jouer_coup(Partie* p, int tas){
    int joueur = p->courant;
    int autre = 1 - joueur;

    // semis dans le sens inverse des aiguilles d'une montre
    int depart = joueur * NB_CASES + tas; // indice absolu sur le plateau
    int nbre_saut = p->cases[depart];
    p->cases[depart] = 0;

    int pos = depart;
    while (nbre_saut > 0){
        pos = (pos + 1) % NB_TOTAL;
        // on saute la case de départ s'il y a plus de 11 graines
        if (pos == depart)
            continue;

        ++p->cases[pos];
        --nbre_saut;
    }

    // capture : on remonte depuis la dernière case tant que c'est chez l'adversaire
    // et qu'il y a 2 ou 3 graines
    int gain = 0;
    int i = pos;
    while (i >= 0 && camp_de(i) == autre && (p->cases[i] == 2 || p->cases[i] == 3)){
        gain += p->cases[i];
        --i;
    }

    // si on prenait toutes les graines de l'adversaire (rafle totale), on ne prend rien
    if (gain < graines_du_camp(p, autre)){
        for (int j = pos; j > i; j--)
            p->cases[j] = 0;
        p->points[joueur] += gain;
    }

    p->courant = autre;
}

void afficher_separateur(int largeurs[], char* texte) {
    for (int i = 0; i < NB_CASES; i++) {
        strcat(texte, "+");
        for (int j = 0; j < largeurs[i]; j++) {
            strcat(texte, "-");
        }
    }
    strcat(texte, "+\n");
}

// rangee : pointeur sur les NB_CASES cases d'un joueur
void afficher_rangee(const int* rangee, int inverser, int largeurs[], char* texte) {
    for (int i = 0; i < NB_CASES; i++) {
        int index = inverser ? NB_CASES - 1 - i : i;
        int graines = rangee[index];
        int espaces = largeurs[i] - graines;

        strcat(texte, "|");
        for (int j = 0; j < espaces / 2; j++) {
            strcat(texte, " ");
        }
        for (int j = 0; j < graines; j++) {
            strcat(texte, ".");
        }
        for (int j = 0; j < espaces - espaces / 2; j++) {
            strcat(texte, " ");
        }
    }
    strcat(texte, "|\n");
}

// Écrit le plateau dans texte (qui doit faire au moins TAILLE_PLATEAU caractères).
// nom_haut joue les cases 6..11 (en haut), nom_bas les cases 0..5 (en bas).
void plateau_texte(const Partie* p, const char* nom_haut, const char* nom_bas, char* texte) {
    int largeurs[NB_CASES];

    for (int i = 0; i < NB_CASES; i++) {
        int graines_haut = p->cases[NB_TOTAL - 1 - i];
        int graines_bas = p->cases[i];
        largeurs[i] = graines_haut > graines_bas ? graines_haut : graines_bas;
        if (largeurs[i] < 3) {
            largeurs[i] = 3;
        }
    }

    texte[0] = 0;
    sprintf(texte, "%s (%d points)\n", nom_haut, p->points[1]);
    afficher_separateur(largeurs, texte);
    afficher_rangee(&p->cases[NB_CASES], 1, largeurs, texte);
    afficher_separateur(largeurs, texte);
    afficher_rangee(&p->cases[0], 0, largeurs, texte);
    afficher_separateur(largeurs, texte);
    sprintf(texte + strlen(texte), "%s (%d points)\n", nom_bas, p->points[0]);
}
