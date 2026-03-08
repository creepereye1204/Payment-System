CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pthread
LIBS    = -lmysqlclient -lpthread
INCLUDE = -I/usr/include/mysql -Iinclude

SERVER_SRC = src/main.c src/server.c src/db.c src/validator.c src/logger.c
CLIENT_SRC = client/client.c

all: server client

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(INCLUDE) -o server $(SERVER_SRC) $(LIBS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o client $(CLIENT_SRC)

clean:
	rm -f server client

.PHONY: all clean
