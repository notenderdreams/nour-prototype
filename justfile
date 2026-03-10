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

# Clean & Run
cr: clean run

# Configure: compile pre-compiled libraries from source
configure:
    #!/bin/bash
    set -e
    echo "Configuring pre-compiled libraries..."
    
    # Static library
    mkdir -p sandbox/compiled_static/lib
    gcc -c -Isandbox/compiled_static/include \
        -o /tmp/example_cstatic.o \
        sandbox/compiled_static/src/example_cstatic.c
    ar rcs sandbox/compiled_static/lib/libexample_cstatic.a /tmp/example_cstatic.o
    echo "✓ Static library: sandbox/compiled_static/lib/libexample_cstatic.a"
    
    # Dynamic library
    mkdir -p sandbox/compiled_dynamic/lib
    gcc -shared -fPIC -Isandbox/compiled_dynamic/include \
        -o sandbox/compiled_dynamic/lib/libexample_cdynamic.dylib \
        sandbox/compiled_dynamic/src/example_cdynamic.c
    echo "✓ Dynamic library: sandbox/compiled_dynamic/lib/libexample_cdynamic.dylib"
    echo "Configure complete!"
