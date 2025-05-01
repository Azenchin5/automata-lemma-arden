%{
#include "automate.tab.h"
#include <string.h>

int yywrap(void) { return 1; }
%}

%%

"etats"         { return ETATS; }
"alphabet"      { return ALPHABET; }
"initiale"       { return INITIALE; }
"transitions"   { return TRANSITIONS; }
"finaux"        { return FINAUX; }

"->"            { return FLECHE; }
":"             { return DEUX_POINTS; }
";"             { return POINT_VIRGULE; }
","             { return VIRGULE; }

[a-zA-Z][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return IDENT;
}

[ \t\r\n]+       { /* ignorer les espaces et sauts de ligne */ }

.                { printf("Caractère inconnu : %s\n", yytext); }

%%
