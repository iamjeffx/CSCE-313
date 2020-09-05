CXX = g++
CXXFLAGS = -Wall -Werror -Wpedantic

all: main.o
	$(CXX) $(CXXFLAGS) main.cpp -g -o output.out

clean:
	rm -rf *.o
	rm -f *.out
	rm -f *~ *.h.gch *#

run: all
	./output.out