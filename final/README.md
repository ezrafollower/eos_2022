# eos-2022-final

## Topic
Whack A Mole

sudo screen /dev/ttyUSB0 9600
portmap&
mount -o tcp 192.168.0.77:/home/ezra/lab_pxa /mnt
./client 192.168.0.77 8898

sudo screen /dev/ttyUSB1 9600
mount -o tcp 192.168.1.78:/home/ezra/lab_pxa /mnt
./client 192.168.1.78 8898
