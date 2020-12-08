CC = gcc
CFLAGS = -Wall -std=gnu99
LIBS = -lm
DEPS = filesystem.h
OBJ = makeFileSystem.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

all: makeFileSystem

makeFileSystem: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o makeFileSystem