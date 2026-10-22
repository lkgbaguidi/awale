#include <stdio.h>
#include <stdlib.h>

#define NB_CASES 6

typedef struct {
    int cases[NB_CASES];
    int points;
} Joueur;

Joueur *creerJoueur(void);
int fin_jeu(Joueur* j1, Joueur* j2);
void afficher_jeu(Joueur* j1, Joueur* j2);

int main()
{
    Joueur *joueur1 = creerJoueur();
    Joueur *joueur2 = creerJoueur();

    int premier = rand()  % 1;
    while(!fin_jeu(joueur1, joueur2)){
        afficher_jeu(joueur1, joueur2);

    }

   return 0 ; 
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
    if((j1->points >= 25) || (est_tout_a_zero(j2->cases, NB_CASES))){
    //affichage vainqueur j1
    return 1;
    }

    else if ((j2->points >= 25) || (est_tout_a_zero(j1->cases, NB_CASES)))
    {
        //affichage vainqueur j2
        return 1;
    }

    return 0;
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

int tour_de(Joueur * joueur_courant, Joueur  * autre_joueur){
    int tas = 0;
    printf("Tu veux jouer  quel tas de cailloux (entre 0 et 5) ???????\n");
    scanf("%d", &tas);

     if (tas < NB_CASES   && tas > 0 ){
        printf("entre 1 et 6 dummy");
        scanf("%d", &tas);
     }

    int nbre_saut = joueur_courant->cases[tas];
    joueur_courant->cases[tas] = 0;
    int cote = 0;
    int tas_actuel = tas;
    
    while(nbre_saut){
        if (!cote){
            ++joueur_courant->cases[tas_actuel];
            --nbre_saut;
        }
        else{
            ++autre_joueur->cases[tas_actuel];
            --nbre_saut;
        }
        if (tas_actuel == 5){
            tas_actuel = 0;
            cote = !cote;
            continue;
        }
        ++tas_actuel;
    }




}