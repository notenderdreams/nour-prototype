# Compile and run the main program (preprocessing is handled by main itself)
main:
    mkdir -p build
    gcc -Isrc -o build/main src/*.c -ldl
    ./build/main

# Run the compiled sandbox app
run: configure main
    ./sandbox/build/sandbox_app

# Run tests
test:
    gcc -Isrc -Itests -o build/test tests/*.c src/arena.c src/nstr.c src/fs.c src/log.c -Wall -Wextra
    ./build/test

# Clean build artifacts
clean:
    rm -rf build
    rm -rf sandbox/build

# Clean & Run
cr: clean run

# Configure: compile pre-compiled libraries from source
configure:
    #!/bin/bash
    set -e
    echo "Configuring pre-compiled libraries..."
    
    # Static library
    mkdir -p sandbox/src/compiled_static/lib
    gcc -c -Isandbox/src/compiled_static/include \
        -o /tmp/example_cstatic.o \
        sandbox/src/compiled_static/src/example_cstatic.c
    ar rcs sandbox/src/compiled_static/lib/libexample_cstatic.a /tmp/example_cstatic.o
    gcc -c -Isandbox/src/compiled_static/include \
        -o /tmp/example_cstatic_extra.o \
        sandbox/src/compiled_static/src/example_cstatic_extra.c
    ar rcs sandbox/src/compiled_static/lib/libexample_cstatic_extra.a /tmp/example_cstatic_extra.o
    echo "✓ Static library: sandbox/src/compiled_static/lib/libexample_cstatic.a"
    echo "✓ Static library: sandbox/src/compiled_static/lib/libexample_cstatic_extra.a"
    
    # Dynamic library
    mkdir -p sandbox/src/compiled_dynamic/lib
    gcc -shared -fPIC -Isandbox/src/compiled_dynamic/include \
        -Wl,-install_name,@loader_path/../src/compiled_dynamic/lib/libexample_cdynamic.dylib \
        -o sandbox/src/compiled_dynamic/lib/libexample_cdynamic.dylib \
        sandbox/src/compiled_dynamic/src/example_cdynamic.c
    gcc -shared -fPIC -Isandbox/src/compiled_dynamic/include \
        -Wl,-install_name,@loader_path/../src/compiled_dynamic/lib/libexample_cdynamic_extra.dylib \
        -o sandbox/src/compiled_dynamic/lib/libexample_cdynamic_extra.dylib \
        sandbox/src/compiled_dynamic/src/example_cdynamic_extra.c
    echo "✓ Dynamic library: sandbox/src/compiled_dynamic/lib/libexample_cdynamic.dylib"
    echo "✓ Dynamic library: sandbox/src/compiled_dynamic/lib/libexample_cdynamic_extra.dylib"
    echo "Configure complete!"
