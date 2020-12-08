CC = gcc
CFLAGS = -Wall -std=gnu99
LIBS = -lm
DEPS = filesystem.h fileSystemOper.h
OBJ = fileSystemOper.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: fileSystemOper

fileSystemOper: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o fileSystemOper