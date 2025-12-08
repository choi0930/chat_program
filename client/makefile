CC = gcc
CFLAGS = -g -MMD
LDFLAGS = -lcrypto -lssl -pthread

OBJS = client.o client_cmd.o crypto_util.o
DEPS = $(OBJS:.o=.d)
EXE = client

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

-include $(DEPS)

clean:
	rm -rf $(EXE) $(OBJS) $(DEPS)
