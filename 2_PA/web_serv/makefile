CC=gcc
OBJS = wrappers.o server_funs.o utilities.o
CFLAGS=-I. -Wall -Wextra
DEPS = tcp_serv.h status_codes.h wrappers.h utilities.h
LDFLAGS = -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

thread: tcp_serv.o wrappers.o server_funs.o utilities.o 
	$(CC) -o tcp_serv tcp_serv.o $(OBJS) $(LDFLAGS)
	
tcp_serv_st: tcp_serv_st.o $(OBJS)
	$(CC) -o tcp_serv_st tcp_serv_st.o $(OBJS) $(LDFLAGS)
	

	
	