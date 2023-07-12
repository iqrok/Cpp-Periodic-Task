all: build

create-dir:
	rm -rf build
	mkdir -p build

build: create-dir
	cmake -S . -B build
	cmake --build build --clean-first

verbose: create-dir
	cmake -DDEBUG=1 -DCMAKE_CXX_FLAGS="-DDEBUG=1" -S . -B build/
	cmake --build build --clean-first

debug: create-dir
	cmake -DDEBUG=2 -DCMAKE_CXX_FLAGS="-DDEBUG=2" -S . -B build/
	cmake --build build --clean-first

dummy: create-dir
	cmake -DDEBUG=2 -DCMAKE_CXX_FLAGS="-DDEBUG=2 -DDUMMY=1" -S . -B build/
	cmake --build build --clean-first
