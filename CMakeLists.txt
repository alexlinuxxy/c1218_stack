cmake_minimum_required (VERSION 2.8)
project (c1218_stack)
SET (SRCS
	src/c1218_stack.c
	src/c1218_al.c
	src/ber.c
	src/c1219_tbl.c
	src/c1218_dl.c
	src/c1218_pl.c
	src/c1218_linux_pl.c
	src/linux_time.c
	src/main.c
	src/c12_crc.c
	)
add_executable(c1218_stack ${SRCS})
ADD_DEFINITIONS(-std=c99 -Wall -Werror -g -D__LINUX__ -D__LOG__ -D__DEBUG__)
