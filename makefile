CXXFLAGS = -ggdb -std=c++17 -Wall -Wextra
OUT_DIR = ./bin
SRC_DIR = ./src
MAPPER_DIR = ./mappers/src
LINUX = -lSDL2 -o
#LINUX = -lSDL2 -O2 -o
NOSDL = -o

main: ./src/*.cpp
	g++ $(CXXFLAGS) $(SRC_DIR)/*.cpp $(MAPPER_DIR)/*.cpp $(LINUX) $(OUT_DIR)/main
clean:
	cd bin && rm -f main
run:
	cd bin && ./main