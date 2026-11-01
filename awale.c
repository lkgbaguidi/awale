#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NB_CASES 6

typedef struct {
    int cases[NB_CASES];
    int points;
} Joueur;

Joueur *creerJoueur(void);
int fin_jeu(Joueur* j1, Joueur* j2);
void afficher_jeu(Joueur* j1, Joueur* j2);
void tour_de(Joueur* joueur_courant, Joueur* autre_joueur);
void afficher_vainqueur(Joueur* j1, Joueur* j2);

int main()
{
    srand(time(NULL));

    Joueur *joueur1 = creerJoueur();
    Joueur *joueur2 = creerJoueur();

    int courant = rand() % 2; // 0 : joueur 1 commence, 1 : joueur 2 commence
    while(!fin_jeu(joueur1, joueur2)){
        afficher_jeu(joueur1, joueur2);

        if (courant == 0){
            printf("\nTour du joueur 1\n");
            tour_de(joueur1, joueur2);
        }
        else{
            printf("\nTour du joueur 2\n");
            tour_de(joueur2, joueur1);
        }
        courant = !courant;
    }

    afficher_jeu(joueur1, joueur2);
    afficher_vainqueur(joueur1, joueur2);

    free(joueur1);
    free(joueur2);
    return 0;
}


int est_tout_a_zero(int tab[], int taille) {
    for (int i = 0; i < taille; i++) {
        if (tab[i] != 0) {
            return 0; // Dès qu'un élément est différent de 0, on renvoie faux
        }
    }
    return 1; // Tous les éléments sont égaux à 0
}


Joueur *creerJoueur(){
    Joueur *joueur = malloc(sizeof(Joueur));

    for (int i = 0; i < NB_CASES; i++)
        joueur->cases[i] = 4;

    joueur ->points = 0;

    return joueur;
}


int fin_jeu(Joueur* j1,Joueur* j2 ){
    if((j1->points >= 25) || (j2->points >= 25)
       || est_tout_a_zero(j1->cases, NB_CASES) || est_tout_a_zero(j2->cases, NB_CASES)){
        return 1;
    }

    return 0;
}

void afficher_vainqueur(Joueur* j1, Joueur* j2){
    printf("\nFin de la partie : joueur 1 = %d points, joueur 2 = %d points\n", j1->points, j2->points);

    // celui qui a 25 points ou dont l'adversaire n'a plus de graines gagne
    if ((j1->points >= 25) || est_tout_a_zero(j2->cases, NB_CASES))
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

void afficher_rangee(Joueur* joueur, int inverser, int largeurs[]) {
    for (int i = 0; i < NB_CASES; i++) {
        int index = inverser ? NB_CASES - 1 - i : i;
        int graines = joueur->cases[index];
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

void afficher_jeu(Joueur* joueur1, Joueur* joueur2) {
    int largeurs[NB_CASES];

    for (int i = 0; i < NB_CASES; i++) {
        int graines_haut = joueur2->cases[NB_CASES - 1 - i];
        int graines_bas = joueur1->cases[i];
        largeurs[i] = graines_haut > graines_bas ? graines_haut : graines_bas;
        if (largeurs[i] < 3) {
            largeurs[i] = 3;
        }
    }

    printf("Joueur 2\n");
    afficher_separateur(largeurs);
    afficher_rangee(joueur2, 1, largeurs);
    afficher_separateur(largeurs);
    afficher_rangee(joueur1, 0, largeurs);
    afficher_separateur(largeurs);
    printf("Joueur 1\n");
}

void tour_de(Joueur * joueur_courant, Joueur  * autre_joueur){
    int tas = -1;

    // saisie d'une case non vide entre 1 et 6
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
        tas--; // indice 0..5
        if (tas < 0 || tas >= NB_CASES){
            printf("Entre 1 et %d !\n", NB_CASES);
        }
        else if (joueur_courant->cases[tas] == 0){
            printf("Cette case est vide !\n");
        }
        else{
            break;
        }
    }

    // semis dans le sens inverse des aiguilles d'une montre
    int nbre_saut = joueur_courant->cases[tas];
    joueur_courant->cases[tas] = 0;

    Joueur *cote_actuel = joueur_courant;
    int tas_actuel = tas;

    while (nbre_saut > 0){
        ++tas_actuel;
        if (tas_actuel == NB_CASES){
            tas_actuel = 0;
            cote_actuel = (cote_actuel == joueur_courant) ? autre_joueur : joueur_courant;
        }
        // on saute la case de départ s'il y a plus de 11 graines
        if (cote_actuel == joueur_courant && tas_actuel == tas)
            continue;

        ++cote_actuel->cases[tas_actuel];
        --nbre_saut;
    }

    // capture : dernière graine chez l'adversaire, on prend les cases de 2 ou 3 graines en remontant
    if (cote_actuel == autre_joueur){
        while (tas_actuel >= 0 && (autre_joueur->cases[tas_actuel] == 2 || autre_joueur->cases[tas_actuel] == 3)){
            joueur_courant->points += autre_joueur->cases[tas_actuel];
            autre_joueur->cases[tas_actuel] = 0;
            --tas_actuel;
        }
    }
}
