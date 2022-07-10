BUILD=./build
SRC=./src
WARN=-Wall -Wextra
OPTIMIZATION=-O3
SECURITY=-fstack-protector-all -fstack-clash-protection -fasynchronous-unwind-tables -fexceptions -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS
HEADER=-Isrc
CFLAGS=$(WARN) $(OPTIMIZATION) $(SECURITY) $(HEADER)
CXX = clang++ $(CFLAGS)
compile:
	$(CXX) -c $(SRC)/silang.cpp -o $(BUILD)/silang.o
	$(CXX) -o $(BUILD)/silang.exe $(BUILD)/silang.o
	$(CXX) -o $(BUILD)/silang $(BUILD)/silang.o