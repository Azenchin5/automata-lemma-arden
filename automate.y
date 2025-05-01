%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100

typedef struct {
    char* from;
    char* to;
    char* symbol;
} Transition;

typedef struct {
    char* etats[MAX];
    int nb_etats;

    char* alphabet[MAX];
    int nb_alpha;

    char* etat_initial;

    Transition transitions[MAX];
    int nb_transitions;

    char* finaux[MAX];
    int nb_finaux;
} Automate;

Automate automate;

void yyerror(const char *msg);
int yylex(void);
extern FILE* yyin;
%}

%union {
    char* str;
}

%token ETATS ALPHABET INITIALE TRANSITIONS FINAUX
%token FLECHE DEUX_POINTS POINT_VIRGULE VIRGULE
%token <str> IDENT

%start programme

%%

programme:
    lignes
    {
        printf("AUTOMATE RECONNU.\n");
    }
;

lignes:
    ligne
  | lignes ligne
;

ligne:
    section_etats
  | section_alphabet
  | section_initiale
  | section_transitions
  | section_finaux
;

section_etats:
    ETATS DEUX_POINTS liste_ident POINT_VIRGULE
    {
        printf("Section ETATS reconnue.\n");
    }
;

liste_ident:
    IDENT
        { automate.etats[automate.nb_etats++] = strdup($1); }
  | liste_ident VIRGULE IDENT
        { automate.etats[automate.nb_etats++] = strdup($3); }
;

section_alphabet:
    ALPHABET DEUX_POINTS liste_alphabet POINT_VIRGULE
    {
        printf("Section ALPHABET reconnue.\n");
    }
;

liste_alphabet:
    IDENT
        { automate.alphabet[automate.nb_alpha++] = strdup($1); }
  | liste_alphabet VIRGULE IDENT
        { automate.alphabet[automate.nb_alpha++] = strdup($3); }
;

section_initiale:
    INITIALE DEUX_POINTS IDENT POINT_VIRGULE
    {
        printf("Section INITIALE reconnue.\n");
        automate.etat_initial = strdup($3);
    }
;

section_transitions:
    TRANSITIONS DEUX_POINTS liste_transitions
    {
        printf("Section TRANSITIONS reconnue.\n");
    }
;

liste_transitions:
    transition
  | liste_transitions transition
;

transition:
    IDENT FLECHE IDENT DEUX_POINTS IDENT POINT_VIRGULE
    {
        automate.transitions[automate.nb_transitions].from = strdup($1);
        automate.transitions[automate.nb_transitions].to = strdup($3);
        automate.transitions[automate.nb_transitions].symbol = strdup($5);
        automate.nb_transitions++;
    }
;

section_finaux:
    FINAUX DEUX_POINTS liste_finaux POINT_VIRGULE
    {
        printf("Section FINAUX reconnue.\n");
    }
;

liste_finaux:
    IDENT
        { automate.finaux[automate.nb_finaux++] = strdup($1); }
  | liste_finaux VIRGULE IDENT
        { automate.finaux[automate.nb_finaux++] = strdup($3); }
;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Erreur syntaxique : %s\n", msg);
}

void export_dot(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Erreur lors de la création du fichier DOT");
        return;
    }

    fprintf(f, "digraph automate {\n");
    fprintf(f, "    rankdir=LR;\n");
    fprintf(f, "    node [shape=circle];\n");

    // État initial (flèche invisible)
    fprintf(f, "    init [shape=point];\n");
    fprintf(f, "    init -> %s;\n", automate.etat_initial);

    // États finaux en double cercle
    for (int i = 0; i < automate.nb_finaux; i++) {
        fprintf(f, "    %s [shape=doublecircle];\n", automate.finaux[i]);
    }

    // Transitions
    for (int i = 0; i < automate.nb_transitions; i++) {
        fprintf(f, "    %s -> %s [label=\"%s\"];\n",
            automate.transitions[i].from,
            automate.transitions[i].to,
            automate.transitions[i].symbol);
    }

    fprintf(f, "}\n");
    fclose(f);
}

int contient(char* tableau[], int taille, const char* valeur){
    for (int i = 0; i < taille; i++) {
        if (strcmp(tableau[i], valeur) == 0) return 1;
    }
    return 0;
}

// Vérifie la cohérence de l'automate
// Vérifie que l'état initial existe, que les transitions sont valides et que les états finaux existent
int verifier_coherence() {
    int erreur = 0;

    // Vérifier que l'état initial existe
    if (!contient(automate.etats, automate.nb_etats, automate.etat_initial)) {
        printf("Erreur : l'état initial '%s' n'est pas déclaré dans les états.\n", automate.etat_initial);
        erreur = 1;
    }

    // Vérifier chaque transition
    for (int i = 0; i < automate.nb_transitions; i++) {
        char* from = automate.transitions[i].from;
        char* to = automate.transitions[i].to;
        char* symb = automate.transitions[i].symbol;

        if (!contient(automate.etats, automate.nb_etats, from)) {
            printf("Erreur : l'état source '%s' d'une transition n'est pas déclaré.\n", from);
            erreur = 1;
        }

        if (!contient(automate.etats, automate.nb_etats, to)) {
            printf("Erreur : l'état cible '%s' d'une transition n'est pas déclaré.\n", to);
            erreur = 1;
        }

        if (!contient(automate.alphabet, automate.nb_alpha, symb)) {
            printf("Erreur : le symbole '%s' d'une transition n'appartient pas à l'alphabet.\n", symb);
            erreur = 1;
        }
    }

    // Vérifier les états finaux
    for (int i = 0; i < automate.nb_finaux; i++) {
        if (!contient(automate.etats, automate.nb_etats, automate.finaux[i])) {
            printf("Erreur : l'état final '%s' n'est pas déclaré.\n", automate.finaux[i]);
            erreur = 1;
        }
    }

    if (!erreur) {
        printf("Toutes les vérifications de cohérence sont validées.\n");
        return 1;
    }

    return 0;
}


