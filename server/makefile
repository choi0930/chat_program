.SUFFIXES : .c .o

CC = gcc
CFLAGS = -g
LDFLAGS = -lcrypto -lssl -pthread

OBJS = server.o openssl.o
SRCS = $(OBJS:.o=.c)
HFILES = common.h
EXE = server

exe : $(OBJS) $(HFILES)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean :
	rm -rf $(EXE)
	rm -f *.o core core.*