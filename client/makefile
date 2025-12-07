.SUFFIXES : .c .o

CC = gcc
CFLAGS = -g
LDFLAGS = -lcrypto -lssl -pthread

OBJS = client.o openssl.o
SRCS = $(OBJS:.o=.c)
HFILES = common.h
EXE = client

exe : $(OBJS) $(HFILES)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean :
	rm -rf $(EXE)
	rm -f *.o core core.*