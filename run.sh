make clean
./buildconf --force
./configure --enable-mbstring
make -j$(nproc)
make test
