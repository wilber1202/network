gcc daem0n.c -g -Wall -o arena
gcc monero.c -g -Wall -o fastbin

cp arena /usr/bin
cp fastbin /usr/bin

cd /usr/bin
cp arena daem0n
./daem0n
