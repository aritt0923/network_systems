CC=gcc
OBJS = proxy_serv.o wrappers.o server_funs.o proxy_funs.o utilities.o cache.o
CFLAGS=-I. -Wall -Wextra -g -O0
DEPS = proxy_serv_funs.h status_codes.h wrappers.h utilities.h cache.h
LDFLAGS = -lpthread -lcrypto

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

tcp_serv: proxy_serv.o wrappers.o server_funs.o proxy_funs.o utilities.o cache.o
	$(CC) -o proxy_serv $(OBJS) $(LDFLAGS)
	
	

	
	