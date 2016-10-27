# c1218_stack

Description:
ANSI C12.18(2006) stack implement by C. It have been implement the needed services, and it can
work in Linux.

First step:
If you want it work in virtual PC, you must create two virtual serial ports at first. 

Complie step:
$ cmake .
$ make

Usage:
$ ./c1218_stack [-t <device|client>] -d /dev/ttyS0
