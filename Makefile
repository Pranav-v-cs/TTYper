ttyper:
	g++ main.cpp -lncurses -o build/ttyper

run: ttyper
	./build/ttyper