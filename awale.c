#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "jeu.h"

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

    // fin par blocage (et pas par 25 points) : chacun garde les graines de son camp
    if (partie.points[0] < 25 && partie.points[1] < 25)
        ramasser_restantes(&partie);

    afficher_jeu(&partie);
    afficher_vainqueur(&partie);

    return 0;
}


void afficher_vainqueur(const Partie* p){
    printf("\nFin de la partie : joueur 1 = %d points, joueur 2 = %d points\n", p->points[0], p->points[1]);

    if (p->points[0] == p->points[1])
        printf("Égalité !\n");
    else if (p->points[0] > p->points[1])
        printf("Le joueur 1 gagne !\n");
    else
        printf("Le joueur 2 gagne !\n");
}

void afficher_jeu(const Partie* p) {
    char texte[TAILLE_PLATEAU];
    plateau_texte(p, "Joueur 2", "Joueur 1", texte);
    printf("%s", texte);
}

void tour_de(Partie* p){
    int joueur = p->courant;
    int tas = -1;

    // saisie d'une case jouable entre 1 et 6 (dans le camp du joueur courant)
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
        else if (!coup_valide(p, joueur, tas)){
            printf("Ton adversaire n'a plus de graines : tu dois lui en donner !\n");
        }
        else{
            break;
        }
    }

    jouer_coup(p, tas);
}
