CC=gcc
OBJS = df_server.o df_server_funs.o ../wrappers.o ../utilities.o ../file_cabinet.o
CFLAGS=-I. -Wall -Wextra -g 
DEPS = df_server.h client_serv_common.h wrappers.h utilities.h file_cabinet.h
LDFLAGS = -lpthread -lcrypto

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

df_server: $(OBJS)
	$(CC) -o dfs $(OBJS) $(LDFLAGS)