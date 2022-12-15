CC=gcc
OBJS = df_client.o ../wrappers.o ../utilities.o ../file_cabinet.o
CFLAGS=-I. -Wall -Wextra -g 
DEPS = df_client.h client_serv_common.h wrappers.h utilities.h file_cabinet.h
LDFLAGS = -lpthread -lcrypto

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

df_client: $(OBJS)
	$(CC) -o dfc $(OBJS) $(LDFLAGS)
	
	

	
	