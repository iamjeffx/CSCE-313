CXX = g++
CXXFLAGS = -Wall -Werror -Wpedantic

all: buggy.o
	$(CXX) $(CXXFLAGS) buggy.cpp -g -o buggy.out

clean:
	rm -rf *.o
	rm -f *.out
	rm -f *~ *.h.gch *#

run: all
	./buggy.out