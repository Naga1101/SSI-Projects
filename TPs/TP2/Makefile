CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: folders server concordia-mensagem concordia-grupo concordia-user

install: concordia-mensagem concordia-grupo concordia-user
	@echo Installing executables to $(BINDIR)
	@mkdir -p $(BINDIR)
	@cp bin/concordia-mensagem $(BINDIR)/concordia-mensagem
	@cp bin/concordia-grupo $(BINDIR)/concordia-grupo
	@cp bin/concordia-user $(BINDIR)/concordia-user
	@chmod 755 $(BINDIR)/concordia-mensagem $(BINDIR)/concordia-grupo $(BINDIR)/concordia-user
	@echo Installation complete.
	@echo "Creating FIFO directory..."
	@mkdir -p /var/lib/concordia/fifos
	@chmod 1777 /var/lib/concordia/fifos
	@echo "Installation complete."

server: bin/daemon

concordia-mensagem: bin/concordia-mensagem

concordia-grupo: bin/concordia-grupo

concordia-user: bin/concordia-user

folders:
	@mkdir -p src include obj bin tmp

bin/daemon: obj/daemon.o obj/command_handler.o obj/user_commands.o obj/message_commands.o obj/group_commands.o obj/files_struct.o obj/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/concordia-mensagem: obj/concordia-mensagem.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/concordia-grupo: obj/concordia-grupo.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/concordia-user: obj/concordia-user.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/*
	rm -f bin/*