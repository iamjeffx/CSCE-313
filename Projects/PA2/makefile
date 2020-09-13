CXX = g++
CXXFLAGS = -Wall -Werror -Wpedantic

all: main.o
	$(CXX) $(CXXFLAGS) main.cpp -g -o output.out

clean:
	rm -rf *.o
	rm -f *.out
	rm -f *~ *.h.gch *#

valgrind: all
	valgrind ./output.out

run: all
	./output.out