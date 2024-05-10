CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

all: folders server concordia-mensagem concordia-grupo concordia-user

server: bin/daemon

concordia-mensagem: bin/concordia-mensagem

concordia-grupo: bin/concordia-grupo

concordia-user: bin/concordia-user

folders:
	@mkdir -p src include obj bin tmp

bin/daemon: obj/daemon.o obj/command_handler.o obj/user_commands.o obj/message_commands.o obj/group_commands.o
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