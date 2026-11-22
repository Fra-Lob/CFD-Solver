
.PHONY: all build test clean rebuild

BUILD_DIR := Bin/build

all: build

build:
	cmake -S Source -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf -- $(BUILD_DIR)
	rm -f -- Bin/*.exe Tests/*.exe

rebuild: clean build
