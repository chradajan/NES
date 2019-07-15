CXXFLAGS = -ggdb -std=c++14 -Wall -Wextra
OUT_DIR = ./bin
SRC_DIR = ./src

main: ./src/*.cpp
	g++ $(CXXFLAGS) $(SRC_DIR)/*.cpp -o $(OUT_DIR)/main
clean:
	cd bin && rm -f main
run:
	g++ $(CXXFLAGS) $(SRC_DIR)/*.cpp -o $(OUT_DIR)/main
	cd bin && ./main