export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
rm -rf build && mkdir -p build && cd build && ../llvm/configure --enable-optimized=yes --enable-assertions=no && make "$@"
