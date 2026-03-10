# Compile and run the main program (preprocessing is handled by main itself)
main:
    mkdir -p build
    gcc -Isrc -o build/main src/*.c -ldl
    ./build/main

# Run the compiled sandbox app
run: main
    ./build/sandbox_app

# Run tests
test:
    gcc -Isrc -Itests -o build/test tests/*.c src/arena.c src/nstr.c src/fs.c src/log.c -Wall -Wextra
    ./build/test

# Clean build artifacts
clean:
    rm -rf build

cr: clean run