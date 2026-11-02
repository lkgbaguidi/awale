#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NB_CASES 6                  // cases par joueur
#define NB_TOTAL (2 * NB_CASES)     // cases sur tout le plateau

// Tout l'état d'une partie. Les cases 0..5 sont celles du joueur 0 (joueur 1 à l'écran),
// les cases 6..11 celles du joueur 1 (joueur 2 à l'écran).
typedef struct {
    int cases[NB_TOTAL];
    int points[2];
    int courant;    // joueur dont c'est le tour : 0 ou 1
} Partie;

void partie_init(Partie* p, int premier);
int fin_jeu(const Partie* p);
void afficher_jeu(const Partie* p);
void tour_de(Partie* p);
void afficher_vainqueur(const Partie* p);

int main()
{
    srand(time(NULL));

    Partie partie;
    partie_init(&partie, rand() % 2); // 0 : joueur 1 commence, 1 : joueur 2 commence

    while(!fin_jeu(&partie)){
        afficher_jeu(&partie);
        printf("\nTour du joueur %d\n", partie.courant + 1);
        tour_de(&partie);
    }

    afficher_jeu(&partie);
    afficher_vainqueur(&partie);

    return 0;
}


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


void partie_init(Partie* p, int premier){
    for (int i = 0; i < NB_TOTAL; i++)
        p->cases[i] = 4;

    p->points[0] = 0;
    p->points[1] = 0;
    p->courant = premier;
}


int fin_jeu(const Partie* p){
    if((p->points[0] >= 25) || (p->points[1] >= 25)
       || camp_vide(p, 0) || camp_vide(p, 1)){
        return 1;
    }

    return 0;
}

void afficher_vainqueur(const Partie* p){
    printf("\nFin de la partie : joueur 1 = %d points, joueur 2 = %d points\n", p->points[0], p->points[1]);

    // celui qui a 25 points ou dont l'adversaire n'a plus de graines gagne
    if ((p->points[0] >= 25) || camp_vide(p, 1))
        printf("Le joueur 1 gagne !\n");
    else
        printf("Le joueur 2 gagne !\n");
}

void afficher_separateur(int largeurs[]) {
    for (int i = 0; i < NB_CASES; i++) {
        printf("+");
        for (int j = 0; j < largeurs[i]; j++) {
            printf("-");
        }
    }
    printf("+\n");
}

// rangee : pointeur sur les NB_CASES cases d'un joueur
void afficher_rangee(const int* rangee, int inverser, int largeurs[]) {
    for (int i = 0; i < NB_CASES; i++) {
        int index = inverser ? NB_CASES - 1 - i : i;
        int graines = rangee[index];
        int espaces = largeurs[i] - graines;

        printf("|");
        for (int j = 0; j < espaces / 2; j++) {
            printf(" ");
        }
        for (int j = 0; j < graines; j++) {
            printf(".");
        }
        for (int j = 0; j < espaces - espaces / 2; j++) {
            printf(" ");
        }
    }
    printf("|\n");
}

void afficher_jeu(const Partie* p) {
    int largeurs[NB_CASES];

    for (int i = 0; i < NB_CASES; i++) {
        int graines_haut = p->cases[NB_TOTAL - 1 - i];
        int graines_bas = p->cases[i];
        largeurs[i] = graines_haut > graines_bas ? graines_haut : graines_bas;
        if (largeurs[i] < 3) {
            largeurs[i] = 3;
        }
    }

    printf("Joueur 2\n");
    afficher_separateur(largeurs);
    afficher_rangee(&p->cases[NB_CASES], 1, largeurs);
    afficher_separateur(largeurs);
    afficher_rangee(&p->cases[0], 0, largeurs);
    afficher_separateur(largeurs);
    printf("Joueur 1\n");
}

void tour_de(Partie* p){
    int joueur = p->courant;
    int tas = -1;

    // saisie d'une case non vide entre 1 et 6 (dans le camp du joueur courant)
    while (1){
        printf("Quelle case veux-tu jouer (entre 1 et %d) ? ", NB_CASES);
        if (scanf("%d", &tas) != 1){
            // entrée non numérique : on vide le buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            if (c == EOF) exit(0);
            printf("Entre un nombre !\n");
            continue;
        }
        tas--; // indice 0..5 dans le camp du joueur
        if (tas < 0 || tas >= NB_CASES){
            printf("Entre 1 et %d !\n", NB_CASES);
        }
        else if (p->cases[joueur * NB_CASES + tas] == 0){
            printf("Cette case est vide !\n");
        }
        else{
            break;
        }
    }

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

    // capture : dernière graine chez l'adversaire, on prend les cases de 2 ou 3 graines en remontant
    while (pos >= 0 && camp_de(pos) != joueur && (p->cases[pos] == 2 || p->cases[pos] == 3)){
        p->points[joueur] += p->cases[pos];
        p->cases[pos] = 0;
        --pos;
    }

    // au tour de l'autre joueur
    p->courant = 1 - joueur;
}
