CX := clang++
CFLAGSX := -std=c++11 -Wall -lm -Iinc -Lraylib/lib -lraylib -Iraylib/include -ldl -lrt -lX11 -lGL -Wl,-rpath,raylib/lib

.PHONY: clean build

build: src/main.cc
	$(CX) -o bin/mandelbrot $(CFLAGSX) src/main.cc

clean:
	rm -rf bin/*
