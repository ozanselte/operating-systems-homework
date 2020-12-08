CXX = g++
CXXFLAGS = -Wall -pedantic-errors -std=c++1y -g
CXXFLAGS += -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
CXXFLAGS += -D_FILE_OFFSET=64
LIBS = -lm -lrt -pthread
DEPS = main.h pageTable.h
OBJ = main.o pageTable.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LIBS)

all: sortArrays

sortArrays: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f *.o sortArrays