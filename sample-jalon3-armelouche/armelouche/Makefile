# Liste des fichiers source
SOURCES = test.c client.c server.c

# Cibles à construire (exécutables)
TARGETS = test client server

# Options de compilation
CFLAGS = -Wall

# Variables pour les paramètres
PORT = 8080
HOST = 127.0.0.1

# Règle par défaut
all: $(TARGETS)

# Compilation générique
%: %.c
	gcc $(CFLAGS) -o $@ $<

# Exécution des cibles avec des paramètres
run-client: client
	./client $(HOST) $(PORT)

run-server: server
	./server $(PORT)

# Nettoyage
clean:
	rm -f $(TARGETS)

.PHONY: all run-client run-server clean
