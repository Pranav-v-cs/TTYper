ttyper:
	g++ -std=c++17 main.cpp -lncurses -o ./build/ttyper

run: ttyper
	./build/ttyper

install: ttyper
	mkdir -p ~/.ttyper
	cp data/word_list.csv ~/.ttyper/word_list.csv
	sudo cp ttyper /usr/local/bin/ttyper