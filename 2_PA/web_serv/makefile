CC=gcc
CFLAGS=-I. -Wall -Wextra
DEPS = tcp_serv.h status_codes.h wrappers.h utilities.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tcp_serv_st: tcp_serv_st.o wrappers.o server_funs.o utilities.o
	$(CC) -o tcp_serv_st tcp_serv_st.o wrappers.o server_funs.o utilities.o
	
tcp_serv: tcp_serv_st.o wrappers.o server_funs.o utilities.o
	$(CC) -o tcp_serv tcp_serv.o wrappers.o server_funs.o utilities.o