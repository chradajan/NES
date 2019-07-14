CXXFLAGS= -ggdb -std=c++14 -Wall -Wextra
main: main.cpp CPU.cpp
	g++ $(CXXFLAGS) main.cpp NES.cpp CPU.cpp APU.cpp PPU.cpp -o main
clean:
	rm -f main
run:
	g++ $(CXXFLAGS) main.cpp NES.cpp CPU.cpp APU.cpp PPU.cpp -o main
	./main
