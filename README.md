Portable-VI
===========
It is a simplified & portable vi source code.
about 30KB@linux, 12KB@stm32, ram should better >= 8KB

Usage
-----
1. edit vi_platform.c, config buffer size and implement some platform related functions
2. `gcc -o vi vi.c -D PLATFORM=linux` to build linux vi 
