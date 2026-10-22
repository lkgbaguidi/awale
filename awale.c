#include <stdio.h>
#include <stdlib.h>

#define NB_CASES 6

typedef struct {
    int cases[NB_CASES];
    int points;
    Joueur* autre_joueur;
} Joueur;


int main()
{
    Joueur *joueur1 = creerJoueur();
    Joueur *joueur2 = creerJoueur();

    joueur1->autre_joueur = joueur2;
    joueur2->autre_joueur = joueur1;

    int premier = rand()  % 1;
    while(!fin_jeu(joueur1, joueur2)){
        afficher_jeu();

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

void afficher_jeu(){
    //affichage Jolie


}

int tour_de(Joueur * joueur_courant, Joueur  * autre_joueur){
    int tas = 0;
    printf("Tu veux jouer  quel tas de cailloux (entre 1 et 6) ???????\n");
    scanf("%d", &tas);

     if (tas < NB_CASES   && tas > 0 ){
        printf("entre 1 et 6 dummy");
        scanf("%d", &tas);
     }
     --tas;

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
            continue;
        }
        ++tas_actuel;
    }




}