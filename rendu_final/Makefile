# Liste des fichiers source
SOURCES = client.c server.c receiver.c sender.c

# Cibles à construire (exécutables)
TARGETS = client server receiver sender 

# Options de compilation
CFLAGS = -Wall

# Variables pour les paramètres
PORT = 8080
HOST = 127.0.0.1
PORT2 = 8888
HOST2 = 127.0.0.1

# Dossier Inbox
INBOX_FOLDER = inbox

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
	rm -rf $(INBOX_FOLDER)/*

.PHONY: all run-client run-server clean
