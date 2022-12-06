/* CPSC 3300, Computer Systems Organization, Clemson University
 * 
 * cache statistics for a 4 KiB, 4-way set associative, write-back
 *   data cache with 32 bytes/line and pseudo-LRU replacement
 *
 * note that this simulation does not include the contents of the
 *   cache lines - instead, the cache directory bits (valid, dirty,
 *   and tag) and the per-set replacement state are used to determine
 *   the hit, miss, and write-back counts based on the addresses used
 *   to access the cache and the types of accesses (reads or writes)
 *
 * routines
 *
 *   void cache_init( void );
 *   void cache_access( unsigned int address, unsigned int type );
 *   void cache_stats( void );
 *
 * for each call to cache_access() address is the byte address, and
 *   type is either read (=0) or write (=1)
 *
 *
 * 4 KiB four-way set-associative cache, 32 bytes/line
 *   => 128 total lines, 4 banks, 32 lines/bank
 *   => 32-bit address partitioned into
 *         22-bit tag
 *          5-bit index         [ 5 = log2( 32 lines/bank ) ]
 *          5-bit byte offset   [ 5 = log2( 32 bytes/line ) ]
 *
 * index            bank 0          bank 1          bank 2          bank 3
 * (set) PLRU   v d tag cont    v d tag cont    v d tag cont    v d tag cont
 *       +--+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+
 *   0   |  |  | | |   |////|  | | |   |////|  | | |   |////|  | | |   |////|
 *       +--+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+
 *   1   |  |  | | |   |////|  | | |   |////|  | | |   |////|  | | |   |////|
 *       +--+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+
 *       ...        ...             ...             ...             ...
 *       +--+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+
 *  31   |  |  | | |   |////|  | | |   |////|  | | |   |////|  | | |   |////|
 *       +--+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+  +-+-+---+----+
 *
 *
 * pseudo-LRU replacement using three-bit state scheme for 4-way s.a.
 *
 *  each bit represents one branch point in a binary decision tree
 *
 *  let 1 represent that the left side has been referenced more
 *  recently than the right side, and 0 vice-versa
 *
 *             are all 4 lines valid?
 *                  /       \
 *                yes        no, use an invalid line
 *                 |
 *                 |
 *                 |
 *            bit_0 == 0?           state | replace    ref to | next state
 *             /       \            ------+--------    -------+-----------
 *            y         n            00x  |  line_0    line_0 |    11_
 *           /           \           01x  |  line_1    line_1 |    10_
 *    bit_1 == 0?    bit_2 == 0?     1x0  |  line_2    line_2 |    0_1
 *      /    \          /    \       1x1  |  line_3    line_3 |    0_0
 *     y      n        y      n
 *    /        \      /        \       ('x' means     ('_' means unchanged)
 *  line_0  line_1  line_2  line_3     don't care)
 *
 * see Figure 3-7, p. 3-18, in Intel Embedded Pentium Processor Family Dev.
 *   Manual, 1998, http://www.intel.com/design/intarch/manuals/273204.htm)
 *
 * note that there is separate state kept for each set (i.e., index value)
 * 
 * NOTE: Cache size changed to 512 B
 * NOTE: Line size changed to 8 bytes
 */

#include "cache.h"

unsigned int
  plru_state[LINES_PER_BANK],  /* current state for each set */
  valid[4][LINES_PER_BANK],    /* valid bit for each line    */
  dirty[4][LINES_PER_BANK],    /* dirty bit for each line    */
  tag[4][LINES_PER_BANK],      /* tag bits for each line     */

  plru_bank[8] /* table for bank replacement choice based on state */

                 = { 0, 0, 1, 1, 2, 3, 2, 3 },

  next_state[32] /* table for next state based on state and bank ref */
                 /* index by 5-bit (4*state)+bank [=(state<<2)|bank] */

                                    /*  bank ref  */
                                    /* 0  1  2  3 */

                 /*         0 */  = {  6, 4, 1, 0,
                 /*         1 */       7, 5, 1, 0,
                 /*         2 */       6, 4, 3, 2,
                 /* current 3 */       7, 5, 3, 2,
                 /*  state  4 */       6, 4, 1, 0,
                 /*         5 */       7, 5, 1, 0,
                 /*         6 */       6, 4, 3, 2,
                 /*         7 */       7, 5, 3, 2  };

unsigned int
    cache_reads,  /* counter */
    cache_writes, /* counter */
    hits,         /* counter */
    misses,       /* counter */
    write_backs;  /* counter */

void cache_init( void ){
  int i;
  for( i=0; i<LINES_PER_BANK; i++ ){
    plru_state[i] = 0;
    valid[0][i] = dirty[0][i] = tag[0][i] = 0;
    valid[1][i] = dirty[1][i] = tag[1][i] = 0;
    valid[2][i] = dirty[2][i] = tag[2][i] = 0;
    valid[3][i] = dirty[3][i] = tag[3][i] = 0;
  }
  cache_reads = cache_writes = hits = misses = write_backs = 0;
}

void cache_stats( void ){
  printf( "cache statistics (in decimal):\n" );
  printf( "  cache reads       = %d\n", cache_reads );
  printf( "  cache writes      = %d\n", cache_writes );
  printf( "  cache hits        = %d\n", hits );
  printf( "  cache misses      = %d\n", misses );
  printf( "  cache write backs = %d\n", write_backs );
}


/* address is byte address, type is read (=0) or write (=1) */

void cache_access( uint8_t address, bool type ){

  uint8_t
    addr_tag,    /* tag bits of address     */
    addr_index,  /* index bits of address   */
    bank;        /* bank that hit, or bank chosen for replacement */

  if( type == 0 ){
    cache_reads++;
  }else{
    cache_writes++;
  }

  // tag (1) | index (5) | offset (2)

  // Get index for 8 byte line size
  //addr_index = (address >> 3) & 0x1F;
  addr_index = (address >> 2) & 0x1F;

  // Get tag for 8 byte line size
  //addr_tag = (address >> 8) & 0x7F;
  addr_tag = (address >> 7) & 0x7F;

  /* check bank 0 hit */

  if( valid[0][addr_index] && (addr_tag==tag[0][addr_index]) ){
    hits++;
    bank = 0;

  /* check bank 1 hit */

  }else if( valid[1][addr_index] && (addr_tag==tag[1][addr_index]) ){
    hits++;
    bank = 1;

  /* check bank 2 hit */

  }else if( valid[2][addr_index] && (addr_tag==tag[2][addr_index]) ){
    hits++;
    bank = 2;

  /* check bank 3 hit */

  }else if( valid[3][addr_index] && (addr_tag==tag[3][addr_index]) ){
    hits++;
    bank = 3;

  /* miss - choose replacement bank */

  }else{
    misses++;

         if( !valid[0][addr_index] ) bank = 0;
    else if( !valid[1][addr_index] ) bank = 1;
    else if( !valid[2][addr_index] ) bank = 2;
    else if( !valid[3][addr_index] ) bank = 3;
    else bank = plru_bank[ plru_state[addr_index] ];

    if( valid[bank][addr_index] && dirty[bank][addr_index] ){
      write_backs++;
    }

    valid[bank][addr_index] = 1;
    dirty[bank][addr_index] = 0;
    tag[bank][addr_index] = addr_tag;
  }

  /* update replacement state for this set (i.e., index value) */

  plru_state[addr_index] = next_state[ (plru_state[addr_index]<<2) | bank ];

  /* update dirty bit on a write */

  if( type == 1 ) dirty[bank][addr_index] = 1;
}

