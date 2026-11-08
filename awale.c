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
void ramasser_restantes(Partie* p);
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

void afficher_vainqueur(const Partie* p){
    printf("\nFin de la partie : joueur 1 = %d points, joueur 2 = %d points\n", p->points[0], p->points[1]);

    if (p->points[0] == p->points[1])
        printf("Égalité !\n");
    else if (p->points[0] > p->points[1])
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
    int autre = 1 - joueur;
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
