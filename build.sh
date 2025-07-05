#!/bin/bash

mkdir -p lib/bin

CC=clang

if command -v clang-14 2>&1 /dev/null; then
    CC=clang-14
fi

$CC \
    --target=wasm32 \
    -nostdlib \
    -DCATTO_USE_64_BIT \
    -Wl,--no-entry \
    -Wl,--export=__heap_base \
    src/*.c -o lib/bin/atto.wasm