# Compile nour.c into a shared object
cso:
    mkdir -p build
    cp Sandbox/project.nour build/project.nour.c
    gcc -shared -fPIC -include stddef.h -Isrc -o build/libnour.so build/project.nour.c

# Compile and run the main program
main: cso
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
