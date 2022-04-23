#ifndef COMMON_H
#define COMMON_H

//-----------------------------------------------------------------------------

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>

//-----------------------------------------------------------------------------

#define INPUT_ARGUMENTS_MAX 3

//-----------------------------------------------------------------------------

#define printerr(fmt,...) \
    do {\
            fprintf(stderr, fmt, ## __VA_ARGS__); \
            fflush(stderr); \
    } while(0)

//-----------------------------------------------------------------------------

#endif // COMMON_H
