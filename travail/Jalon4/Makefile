# Liste des fichiers source
SOURCES = test.c client.c server.c receiver.c sender.c

# Cibles à construire (exécutables)
TARGETS = test client server receiver sender

# Options de compilation
CFLAGS = -Wall

# Variables pour les paramètres
PORT = 8080
HOST = 127.0.0.1
PORT2 = 8888
HOST2 = 127.0.0.1
NAME = papa
FILE = "fichier.txt"

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

run-sender: sender
	./sender $(HOST2) $(PORT2) $(NAME) $(FILE)

run-receiver : receiver
	./receiver $(PORT2) 
# Nettoyage
clean:
	rm -f $(TARGETS)

.PHONY: all run-client run-server clean
