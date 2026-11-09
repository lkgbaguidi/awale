#ifndef JEU_H
#define JEU_H

#define NB_CASES 6                  // cases par joueur
#define NB_TOTAL (2 * NB_CASES)     // cases sur tout le plateau
#define TAILLE_PLATEAU 4096         // taille du texte qui contient le plateau

// Tout l'état d'une partie. Les cases 0..5 sont celles du joueur 0 (joueur 1 à l'écran),
// les cases 6..11 celles du joueur 1 (joueur 2 à l'écran).
typedef struct {
    int cases[NB_TOTAL];
    int points[2];
    int courant;    // joueur dont c'est le tour : 0 ou 1
} Partie;

void partie_init(Partie* p, int premier);
int coup_valide(const Partie* p, int joueur, int tas);
int fin_jeu(const Partie* p);
void ramasser_restantes(Partie* p);
void jouer_coup(Partie* p, int tas);
int gagnant(const Partie* p);
void plateau_texte(const Partie* p, const char* nom_haut, const char* nom_bas, char* texte);

#endif
