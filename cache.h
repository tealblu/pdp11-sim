#ifndef CACHE_H
#define CACHE_H

#include<stdio.h>
#include<stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define LINES_PER_BANK 32

void cache_init( void );
void cache_stats( void );
void cache_access( uint8_t address, bool type );

#endif