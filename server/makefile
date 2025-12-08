CC = gcc
CFLAGS = -g -MMD
LDFLAGS = -lcrypto -lssl -pthread

OBJS = server.o server_cmd.o crypto_util.o common.o
DEPS = $(OBJS:.o=.d)
EXE = server

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

-include $(DEPS)

clean:
	rm -rf $(EXE) $(OBJS) $(DEPS)
