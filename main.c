#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ExpressionNode
{
    char *texte;                 // Par exemple : "a", "L_S1", "+", ".", "ε"
    struct ExpressionNode *next; // Lien vers le morceau suivant
} ExpressionNode;

typedef struct
{
    char nom[32];         // Exemple : "L_S1"
    ExpressionNode *expr; // Liste chaînée de l'expression
} Equation;

extern FILE *yyin;
extern int yyparse(void);

// Fonctions et structures déclarées dans automate.y
void export_dot(const char *filename);
int verifier_coherence(void);
int contient(char* tableau[], int taille, const char* valeur);


// Structure automate définie dans automate.y
extern struct
{
    char *etats[100];
    int nb_etats;

    char *alphabet[100];
    int nb_alpha;

    char *etat_initial;

    struct
    {
        char *from;
        char *to;
        char *symbol;
    } transitions[100];
    int nb_transitions;

    char *finaux[100];
    int nb_finaux;
} automate;

int est_final(const char* etat) {
    return contient(automate.finaux, automate.nb_finaux, etat);
}

int simulate_recursive(const char* etat_courant, const char* mot) {
    if (*mot == '\0') {
        return est_final(etat_courant);  // mot terminé
    }

    char symbole[2] = { mot[0], '\0' };
    int accepte = 0;

    for (int i = 0; i < automate.nb_transitions; i++) {
        if (strcmp(automate.transitions[i].from, etat_courant) == 0 &&
            strcmp(automate.transitions[i].symbol, symbole) == 0) {

            if (simulate_recursive(automate.transitions[i].to, mot + 1)) {
                accepte = 1;
                break;
            }
        }
    }

    return accepte;
}

int simulate(const char* mot) {
    return simulate_recursive(automate.etat_initial, mot);
}


void tester_mots_fichier(const char *nom_fichier)
{
    FILE *f = fopen(nom_fichier, "r");
    if (!f)
    {
        perror("Erreur lors de l'ouverture du fichier de mots");
        return;
    }

    char ligne[256];
    int ligne_num = 1;

    printf("\n--- Test des mots depuis '%s' ---\n", nom_fichier);

    while (fgets(ligne, sizeof(ligne), f))
    {
        ligne[strcspn(ligne, "\n")] = '\0'; // enlever \n

        if (strlen(ligne) == 0)
            continue;

        printf("Mot %d : \"%s\" --> ", ligne_num, ligne);
        if (simulate(ligne))
        {
            printf("ACCEPTÉ\n");
        }
        else
        {
            printf("REFUSÉ\n");
        }

        ligne_num++;
    }

    fclose(f);
}

void ajouter_morceau(ExpressionNode **head, const char *texte) {
    ExpressionNode *nouveau = malloc(sizeof(ExpressionNode));
    nouveau->texte = strdup(texte);
    nouveau->next = NULL;

    if (*head == NULL) {
        *head = nouveau;
    } else {
        ExpressionNode *courant = *head;
        while (courant->next != NULL) {
            courant = courant->next;
        }
        courant->next = nouveau;
    }
}

void afficher_expression(ExpressionNode *e) {
    while (e) {
        printf("%s", e->texte);
        e = e->next;
    }
    printf("\n");
}

void generer_equations_arden(Equation system[], int *n_equations)
{
    int n = automate.nb_etats;
    *n_equations = n;

    // Initialiser les équations
    for (int i = 0; i < n; i++)
    {
        snprintf(system[i].nom, sizeof(system[i].nom), "L_%s", automate.etats[i]);
        system[i].expr = NULL;
    }

    // Ajouter les transitions
    for (int i = 0; i < automate.nb_transitions; i++)
    {
        char *from = automate.transitions[i].from;
        char *to = automate.transitions[i].to;
        char *symbol = automate.transitions[i].symbol;

        int index_from = -1;
        for (int j = 0; j < n; j++)
        {
            if (strcmp(from, automate.etats[j]) == 0)
            {
                index_from = j;
                break;
            }
        }

        if (index_from == -1)
        {
            fprintf(stderr, "Erreur : état de départ %s introuvable.\n", from);
            continue;
        }

        // Si déjà quelque chose -> ajouter un "+"
        if (system[index_from].expr != NULL)
        {
            ajouter_morceau(&(system[index_from].expr), "+");
        }

        // Ajouter séparément :
        // 1. symbole
        ajouter_morceau(&(system[index_from].expr), symbol);
        // 2. point
        ajouter_morceau(&(system[index_from].expr), ".");
        // 3. L_ + to
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "L_%s", to);
        ajouter_morceau(&(system[index_from].expr), buffer);
    }

    // Ajouter epsilon pour les états finaux
    for (int i = 0; i < n; i++)
    {
        int est_final = 0;
        for (int j = 0; j < automate.nb_finaux; j++)
        {
            if (strcmp(automate.etats[i], automate.finaux[j]) == 0)
            {
                est_final = 1;
                break;
            }
        }

        if (est_final)
        {
            if (system[i].expr != NULL)
            {
                ajouter_morceau(&(system[i].expr), "+");
            }
            ajouter_morceau(&(system[i].expr), "ε");
        }
    }
}

void appliquer_lemme_arden(Equation *eq) {
    if (!eq || !eq->expr) return;

    ExpressionNode *courant = eq->expr;
    ExpressionNode *dot_node = NULL;
    ExpressionNode *Lq_node = NULL;
    ExpressionNode *after_Lq = NULL;

    // Trouver L_qi
    while (courant != NULL) {
        if (courant->texte &&
            strncmp(courant->texte, "L_", 2) == 0 &&
            strcmp(courant->texte, eq->nom) == 0) {
            Lq_node = courant;
            break;
        }
        courant = courant->next;
    }

    if (!Lq_node) return;

    // Vérifier que juste avant L_qi il y a un "."
    courant = eq->expr;
    while (courant && courant->next != Lq_node) {
        courant = courant->next;
    }
    if (!courant || strcmp(courant->texte, ".") != 0) return;

    dot_node = courant;

    // Isoler A
    ExpressionNode *A = eq->expr;
    ExpressionNode *tmp = A;
    while (tmp && tmp->next != dot_node) {
        tmp = tmp->next;
    }
    if (tmp) tmp->next = NULL;

    // Isoler B
    after_Lq = Lq_node->next;

    // Copier A
    ExpressionNode *copieA = NULL;
    tmp = A;
    while (tmp) {
        ajouter_morceau(&copieA, tmp->texte);
        tmp = tmp->next;
    }

    // Copier B
    ExpressionNode *copieB = NULL;
    tmp = after_Lq;
    while (tmp) {
        ajouter_morceau(&copieB, tmp->texte);
        tmp = tmp->next;
    }

    // Libérer ancienne expression
    courant = eq->expr;
    while (courant) {
        ExpressionNode *next = courant->next;
        free(courant->texte);
        free(courant);
        courant = next;
    }

    // Reconstruire : ( A ) * . ( B )
    eq->expr = NULL;
    ajouter_morceau(&(eq->expr), "(");
    courant = copieA;
    while (courant) {
        ajouter_morceau(&(eq->expr), courant->texte);
        courant = courant->next;
    }
    ajouter_morceau(&(eq->expr), ")");
    ajouter_morceau(&(eq->expr), "*");
    ajouter_morceau(&(eq->expr), ".");

    // Vérifier si B contient un + hors parenthèses
    int besoin_parentheses = 0;
    int niveau = 0;
    courant = copieB;
    while (courant) {
        if (strcmp(courant->texte, "(") == 0) niveau++;
        else if (strcmp(courant->texte, ")") == 0) niveau--;
        else if (strcmp(courant->texte, "+") == 0 && niveau == 0) {
            besoin_parentheses = 1;
            break;
        }
        courant = courant->next;
    }

    if (besoin_parentheses)
        ajouter_morceau(&(eq->expr), "(");

    // Supprimer + initial si présent
    courant = copieB;
    if (courant && strcmp(courant->texte, "+") == 0)
        courant = courant->next;

    while (courant) {
        ajouter_morceau(&(eq->expr), courant->texte);
        courant = courant->next;
    }

    if (besoin_parentheses)
        ajouter_morceau(&(eq->expr), ")");

    // Libérer A et B
    courant = copieA;
    while (courant) {
        ExpressionNode *next = courant->next;
        free(courant->texte);
        free(courant);
        courant = next;
    }

    courant = copieB;
    while (courant) {
        ExpressionNode *next = courant->next;
        free(courant->texte);
        free(courant);
        courant = next;
    }
}


int remplacer_expression_propre(Equation *cible, const char *nom_a_remplacer, ExpressionNode *remplacement)
{
    ExpressionNode *courant = cible->expr;
    ExpressionNode *precedent = NULL;
    int fait_remplacement = 0;

    while (courant != NULL)
    {
        if (strcmp(courant->texte, nom_a_remplacer) == 0)
        {
            fait_remplacement = 1;

            // Créer la copie avec parenthèses
            ExpressionNode *copie = NULL;
            ajouter_morceau(&copie, "(");

            ExpressionNode *tmp = remplacement;
            while (tmp != NULL)
            {
                ajouter_morceau(&copie, tmp->texte);
                tmp = tmp->next;
            }

            ajouter_morceau(&copie, ")");

            // Remplacer
            ExpressionNode *next = courant->next;
            free(courant->texte);
            free(courant);

            if (precedent == NULL)
            {
                cible->expr = copie;
            }
            else
            {
                precedent->next = copie;
            }

            // Aller à la fin
            ExpressionNode *fin = copie;
            while (fin->next != NULL)
                fin = fin->next;
            fin->next = next;

            courant = next;
        }
        else
        {
            precedent = courant;
            courant = courant->next;
        }
    }

    return fait_remplacement;
}

int reste_L_Si(ExpressionNode *expr)
{
    while (expr)
    {
        if (strncmp(expr->texte, "L_S", 3) == 0)
        {
            return 1; // Il reste un L_SX
        }
        expr = expr->next;
    }
    return 0; // Plus aucun L_SX
}

void factoriser_L_Si(ExpressionNode **expr, const char *nom) {
    if (!expr || !*expr) return;

    ExpressionNode *facteurs = NULL;
    ExpressionNode *autres = NULL;
    ExpressionNode *courant = *expr;

    while (courant) {
        ExpressionNode *terme_fin = courant;
        int parenthese = 0;

        // Trouver la fin du terme (jusqu'à + hors parenthèses)
        while (terme_fin) {
            if (strcmp(terme_fin->texte, "(") == 0) parenthese++;
            else if (strcmp(terme_fin->texte, ")") == 0) parenthese--;
            else if (strcmp(terme_fin->texte, "+") == 0 && parenthese == 0) break;
            terme_fin = terme_fin->next;
        }

        // Nouvelle détection du dernier ". L_nom" juste avant terme_fin
        int est_facteur = 0;
        ExpressionNode *ptr = courant;
        ExpressionNode *dernier_point = NULL;

        while (ptr && ptr->next && ptr->next != terme_fin) {
            if (strcmp(ptr->texte, ".") == 0 &&
                strcmp(ptr->next->texte, nom) == 0 &&
                ptr->next->next == terme_fin) {
                dernier_point = ptr;
                est_facteur = 1;
            }
            ptr = ptr->next;
        }

        // Repartir du début pour copier le terme
        ptr = courant;
        while (ptr != terme_fin) {
            if (est_facteur && ptr == dernier_point)
                break;
            ajouter_morceau(est_facteur ? &facteurs : &autres, ptr->texte);
            ptr = ptr->next;
        }

        if (!est_facteur && ptr != terme_fin) {
            while (ptr != terme_fin) {
                ajouter_morceau(&autres, ptr->texte);
                ptr = ptr->next;
            }
        }

        courant = (terme_fin ? terme_fin->next : NULL);
    }

    ExpressionNode *nouvelle = NULL;

    if (facteurs) {
        if (facteurs->next) ajouter_morceau(&nouvelle, "(");
        ExpressionNode *f = facteurs;
        while (f) {
            ajouter_morceau(&nouvelle, f->texte);
            if (f->next &&
                strcmp(f->texte, "+") != 0 &&
                strcmp(f->texte, ".") != 0 &&
                strcmp(f->texte, "(") != 0 &&
                strcmp(f->texte, ")") != 0 &&
                strcmp(f->next->texte, "+") != 0 &&
                strcmp(f->next->texte, ".") != 0 &&
                strcmp(f->next->texte, ")") != 0) {
                ajouter_morceau(&nouvelle, "+");
            }
            f = f->next;
        }
        if (facteurs->next) ajouter_morceau(&nouvelle, ")");
        ajouter_morceau(&nouvelle, ".");
        ajouter_morceau(&nouvelle, nom);
    }

    if (autres) {
        if (nouvelle) ajouter_morceau(&nouvelle, "+");
        ExpressionNode *a = autres;
        while (a) {
            ajouter_morceau(&nouvelle, a->texte);
            if (a->next &&
                strcmp(a->texte, "+") != 0 &&
                strcmp(a->texte, ".") != 0 &&
                strcmp(a->texte, "(") != 0 &&
                strcmp(a->texte, ")") != 0 &&
                strcmp(a->next->texte, "+") != 0 &&
                strcmp(a->next->texte, ".") != 0 &&
                strcmp(a->next->texte, ")") != 0) {
                ajouter_morceau(&nouvelle, "+");
            }
            a = a->next;
        }
    }

    ExpressionNode *tmp = *expr;
    while (tmp) {
        ExpressionNode *suiv = tmp->next;
        free(tmp->texte);
        free(tmp);
        tmp = suiv;
    }

    *expr = nouvelle;
}


void developper_expression_propre(Equation *eq)
{
    if (!eq || !eq->expr) return;

    ExpressionNode *courant = eq->expr;
    ExpressionNode *precedent = NULL;

    while (courant && courant->next && courant->next->next)
    {
        if (strcmp(courant->next->texte, ".") == 0 &&
            strcmp(courant->next->next->texte, "(") == 0)
        {
            //  Étape 1 : Sauver le début de l'ancien bloc AVANT toute modif
            ExpressionNode *debut_bloc = courant;

            //  Étape 2 : Copier le facteur
            ExpressionNode *facteur = NULL;
            ExpressionNode *ptr_facteur = eq->expr;
            while (ptr_facteur != courant->next) {
                ajouter_morceau(&facteur, ptr_facteur->texte);
                ptr_facteur = ptr_facteur->next;
            }

            //  Étape 3 : Trouver fermeture
            ExpressionNode *contenu = courant->next->next->next;
            ExpressionNode *fermeture = contenu;
            int niveau = 1;
            while (fermeture) {
                if (strcmp(fermeture->texte, "(") == 0) niveau++;
                else if (strcmp(fermeture->texte, ")") == 0) niveau--;
                if (niveau == 0) break;
                fermeture = fermeture->next;
            }
            if (!fermeture) return;

            ExpressionNode *suite = fermeture->next;

            //  Étape 4 : Construire les nouveaux morceaux
            ExpressionNode *nouveau = NULL;
            ExpressionNode *tmp = contenu;
            int debut_terme = 1;
            while (tmp && tmp != fermeture) {
                if (strcmp(tmp->texte, "+") == 0) {
                    ajouter_morceau(&nouveau, "+");
                    debut_terme = 1;
                } else if (strcmp(tmp->texte, ".") == 0) {
                    ajouter_morceau(&nouveau, ".");
                    debut_terme = 0;
                } else {
                    if (debut_terme) {
                        ExpressionNode *f = facteur;
                        while (f) {
                            ajouter_morceau(&nouveau, f->texte);
                            f = f->next;
                        }
                        ajouter_morceau(&nouveau, ".");
                    }
                    ajouter_morceau(&nouveau, tmp->texte);
                    debut_terme = 0;
                }
                tmp = tmp->next;
            }

            //  Étape 5 : Libérer l’ancien bloc
            tmp = eq->expr;
            while (tmp != suite) {
                ExpressionNode *next = tmp->next;
                free(tmp->texte);
                free(tmp);
                tmp = next;
            }

            //  Libérer le facteur
            tmp = facteur;
            while (tmp) {
                ExpressionNode *next = tmp->next;
                free(tmp->texte);
                free(tmp);
                tmp = next;
            }

            //  Étape 6 : Remplacer eq->expr seulement maintenant
            eq->expr = nouveau;
            ExpressionNode *fin = nouveau;
            while (fin && fin->next)
                fin = fin->next;
            if (fin) fin->next = suite;

            courant = suite;
            precedent = NULL;
        }
        else {
            precedent = courant;
            courant = courant->next;
        }
    }
}


void supprimer_epsilon_et_points(ExpressionNode **expr) {
    if (!expr || !*expr) return;

    ExpressionNode *courant = *expr;
    ExpressionNode *precedent = NULL;

    while (courant && courant->next) {
        ExpressionNode *n1 = courant;
        ExpressionNode *n2 = courant->next;
        ExpressionNode *n3 = n2 ? n2->next : NULL;

        // Cas 1 : . ε .
        if (n1 && n2 && n3 &&
            n1->texte && n2->texte && n3->texte &&
            strcmp(n1->texte, ".") == 0 &&
            strcmp(n2->texte, "ε") == 0 &&
            strcmp(n3->texte, ".") == 0) {

            if (precedent)
                precedent->next = n3->next;
            else
                *expr = n3->next;

            free(n1->texte); free(n1);
            free(n2->texte); free(n2);
            free(n3->texte); free(n3);

            courant = (precedent ? precedent->next : *expr);
            continue;
        }

        // Cas 2 : . ε
        if (n1 && n2 &&
            n1->texte && n2->texte &&
            strcmp(n1->texte, ".") == 0 &&
            strcmp(n2->texte, "ε") == 0) {

            if (precedent)
                precedent->next = n2->next;
            else
                *expr = n2->next;

            free(n1->texte); free(n1);
            free(n2->texte); free(n2);

            courant = (precedent ? precedent->next : *expr);
            continue;
        }

        // Cas 3 : ε .
        if (n1 && n2 &&
            n1->texte && n2->texte &&
            strcmp(n1->texte, "ε") == 0 &&
            strcmp(n2->texte, ".") == 0) {

            if (precedent)
                precedent->next = n2->next;
            else
                *expr = n2->next;

            free(n1->texte); free(n1);
            free(n2->texte); free(n2);

            courant = (precedent ? precedent->next : *expr);
            continue;
        }

        // Sinon, avance
        precedent = courant;
        courant = courant->next;
    }
}



int main()
{
    extern int yydebug;
    yydebug = 0;

    // Initialisation
    automate.nb_etats = 0;
    automate.nb_alpha = 0;
    automate.nb_transitions = 0;
    automate.nb_finaux = 0;
    automate.etat_initial = NULL;

    // Rediriger yyin vers exemple.txt
    yyin = fopen("exemple.txt", "r");
    if (!yyin)
    {
        perror("Erreur lors de l'ouverture de 'exemple.txt'");
        return 1;
    }

    printf("Début de l'analyse...\n");

    if (yyparse() == 0)
    {
        fclose(yyin);

        printf("\nSTRUCTURE DE L'AUTOMATE :\n");

        printf("- États : ");
        for (int i = 0; i < automate.nb_etats; i++)
            printf("%s ", automate.etats[i]);
        printf("\n");

        printf("- Alphabet : ");
        for (int i = 0; i < automate.nb_alpha; i++)
            printf("%s ", automate.alphabet[i]);
        printf("\n");

        printf("- État initial : %s\n", automate.etat_initial);

        printf("- Transitions :\n");
        for (int i = 0; i < automate.nb_transitions; i++)
        {
            printf("  %s -> %s : %s\n",
                   automate.transitions[i].from,
                   automate.transitions[i].to,
                   automate.transitions[i].symbol);
        }

        printf("- États finaux : ");
        for (int i = 0; i < automate.nb_finaux; i++)
            printf("%s ", automate.finaux[i]);
        printf("\n");

        printf("Vérification de la cohérence de l'automate...\n");

        if (verifier_coherence())
        {
            printf("Vérification de la cohérence terminée.\n");
            printf("L'automate est cohérent.\n");

            // Export .dot
            printf("Exportation de l'automate au format DOT...\n");
            export_dot("automate.dot");
            printf("Fichier DOT généré : automate.dot\n");

            // Génération automatique de l'image
            if (system("dot -Tpng automate.dot -o automate.png") == 0)
            {
                printf("Image PNG générée avec succès : automate.png\n");
            }
            else
            {
                printf("Erreur : impossible de générer le fichier automate.png\n");
            }

            tester_mots_fichier("mots.txt");

            // Simulation de mots
            char mot[256];
            while (1)
            {
                printf("\nEntrez un mot à tester (ou 'exit' pour quitter) : ");
                if (!fgets(mot, sizeof(mot), stdin))
                    break;
                mot[strcspn(mot, "\n")] = '\0';
                if (strcmp(mot, "exit") == 0)
                    break;

                if (simulate(mot))
                {
                    printf("Résultat : mot ACCEPTÉ\n");
                }
                else
                {
                    printf("Résultat : mot REFUSÉ\n");
                }
            }

            Equation system[automate.nb_etats];
            int n_equations = 0;
            generer_equations_arden(system, &n_equations);

            printf("\n--- Équations Arden ---\n");
            for (int i = 0; i < n_equations; i++)
            {
                printf("%s = ", system[i].nom);
                afficher_expression(system[i].expr);
                printf("\n");
            }

            for (int i = n_equations - 1; i >= 0; i--)
            {
                printf("\n--- Traitement de %s ---\n", system[i].nom);

                // 1. Développement
                printf("Développement de %s\n", system[i].nom);
                developper_expression_propre(&system[i]);
                printf("%s = ", system[i].nom);
                afficher_expression(system[i].expr);
                printf("\n");

                // 2. Factorisation
                printf("Factorisation de %s\n", system[i].nom);
                factoriser_L_Si(&system[i].expr, system[i].nom);
                printf("%s = ", system[i].nom);
                afficher_expression(system[i].expr);
                printf("\n");

                // 3. Appliquer le lemme d'Arden
                printf("Application du lemme d'Arden sur %s\n", system[i].nom);
                appliquer_lemme_arden(&system[i]);
                printf("%s = ", system[i].nom);
                afficher_expression(system[i].expr);
                printf("\n");

                // 4. Développement
                printf("Développement de %s après application du lemme\n", system[i].nom);
                developper_expression_propre(&system[i]);
                printf("%s = ", system[i].nom);
                afficher_expression(system[i].expr);
                printf("\n");

                // 5. Remplacer cette solution dans toutes les équations au-dessus (indices < i)
                for (int j = i - 1; j >= 0; j--)
                {
                    printf("Remplacement de %s dans %s\n", system[i].nom, system[j].nom);
                    if (remplacer_expression_propre(&system[j], system[i].nom, system[i].expr))
                    {
                        printf("%s = ", system[j].nom);
                        afficher_expression(system[j].expr);
                        printf("\n");
                    }
                }
            }

            printf("\n--- Langage finale après résolution --- \n");
            printf("%s = ", system[0].nom);
            supprimer_epsilon_et_points(&(system[0].expr));
            afficher_expression(system[0].expr);
            printf("\n");
           
        }
        else
        {
            printf("Vérification de la cohérence terminée.\n");
            printf("L'automate n'est pas cohérent.\n");
        }
    }

    return 0;
}
