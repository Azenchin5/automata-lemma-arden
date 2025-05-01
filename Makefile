# Nom du programme final
TARGET = automate

# Fichiers sources
LEX = automate.l
YACC = automate.y
MAIN = main.c

# Fichiers générés
LEX_C = lex.yy.c
YACC_C = automate.tab.c
YACC_H = automate.tab.h

# Commandes
CC = gcc
FLEX = flex
BISON = bison -d --debug

# Compilation finale
$(TARGET): $(LEX_C) $(YACC_C) $(MAIN)
	$(CC) $(MAIN) $(LEX_C) $(YACC_C) -o $(TARGET) -lfl

# Générer les fichiers C à partir de Flex/Bison
$(LEX_C): $(LEX)
	$(FLEX) $(LEX)

$(YACC_C): $(YACC)
	$(BISON) $(YACC)

# Nettoyer les fichiers générés
clean:
	rm -f $(TARGET) $(LEX_C) $(YACC_C) $(YACC_H) automate.dot automate.png
