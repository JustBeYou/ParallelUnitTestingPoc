.PHONY: all clean

all: poc
	g++ -o poc poc.cpp -lpthread -lrt
clean:
	rm -rf poc
