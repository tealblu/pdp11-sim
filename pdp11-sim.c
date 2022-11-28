// #define DEBUG

/**
 * @file pdp11-sim.c
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief Simulation of PDP-11 instruction set
 * @version 0.1
 * @date 2022-10-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

// Defines
#define MEMSIZE (32*1024)

/* struct top help organize source and destination operand handling */
typedef struct ap {
    int mode;
    int reg;
    int addr; /* used only for modes 1-7 */
    int value;
} addr_phrase_t;

// Global variables
uint16_t memory[MEMSIZE]; // 16-bit memory
uint16_t reg[8] = {0}; // R0-R7
bool n, z, v, c; // Condition codes

addr_phrase_t src, dst; // Source and destination address phrases

bool running; // Flag to indicate if the program is running
bool trace = false;
bool verbose = false;
int memory_reads = 0;
int memory_writes = 0;
int inst_fetches = 0;
int inst_execs = 0;
int branch_taken = 0;
int branch_execs = 0;

// Function prototypes
void operate(uint16_t instruction);
void get_operand(addr_phrase_t *phrase);
void update_operand(addr_phrase_t *phrase);
void put_operand(addr_phrase_t *phrase);
void add(uint16_t operand);
void asl(uint16_t operand);
void asr(uint16_t operand);
void beq(uint16_t operand);
void bne(uint16_t operand);
void br(uint16_t operand);
void cmp(uint16_t operand);
void halt(uint16_t operand);
void mov(uint16_t operand);
void sob(uint16_t operand);
void sub(uint16_t operand);
void pstats();
void pregs();

// Main function
int main(int argc, char *argv[])
{
    // Initialize everything
    running = true;
    n = z = v = c = false;
    src.mode = src.reg = src.addr = src.value = 0;
    dst.mode = dst.reg = dst.addr = dst.value = 0;

    // Initialize memory
    for (int i = 0; i < MEMSIZE; i++) memory[i] = 0;

    // Run command format: ./a.out <flags>
    // Flags: -t (instruction trace), -v (verbose trace)

    // Check for flags
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0) trace = true;
        else if (strcmp(argv[i], "-v") == 0) verbose = true;
        else
        {
            printf("Invalid flag: %s\n", argv[i]);
            exit(1);
        }
    }

    // verbose trace
    if (verbose) printf("\nreading words in octal from stdin:\n");

    // Read words in octal from stdin
    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        // Read a word into every other memory location
        memory[i] = (uint16_t)strtol(line, NULL, 8);

        // verbose trace
        if (verbose) printf("  %07o\n", memory[i]);
        i += 2;
    }

    // instruction trace
    if (trace || verbose) printf("\ninstruction trace:\n");

    // Set PC to 0
    reg[7] = 0;

    // Loop through memory
    while (reg[7] < MEMSIZE && running)
    {
        if(trace || verbose) printf("at %05o, ", reg[7]);

        // Get instruction from memory
        uint16_t instruction = memory[reg[7]];
        reg[7] += 2;

        #ifdef DEBUG
        printf("\nexecuting instruction %05o at address %05o\n", instruction, reg[7]-1);
        #endif

        operate(instruction);

        // ensure PC is not out of bounds
        if(reg[7] >= MEMSIZE)
        {
            printf("PC out of bounds: %d\n", reg[7]);
            exit(1);
        }
    }

    // Print execution statistics
    pstats();
}

// Function definitions
void operate(uint16_t instruction) {

    // Execute 4 bit opcode
    if(((instruction & 0xF000) >> 12) == 01) // mov
    {
        mov(instruction);
    }
    else if(((instruction & 0xF000) >> 12) == 02) // cmp
    {
        cmp(instruction);
    }
    else if(((instruction & 0xF000) >> 12) == 06) // add
    {
        add(instruction);
    }
    else if(((instruction & 0xF000) >> 12) == 016) // sub
    {
        sub(instruction);
    }

    // Opcode is not 4 bits, try 7
    else if(((instruction & 0xFE00) >> 9) == 077) // sob
    {
        sob(instruction);
    }

    // Opcode is not 7 bits, try 8
    else if(((instruction & 0xFF00) >> 8) == 001) // br
    {
        br(instruction);
    }
    else if(((instruction & 0xFF00) >> 8) == 002) // bne
    {
        bne(instruction);
    }
    else if(((instruction & 0xFF00) >> 8) == 003) // beq
    {
        beq(instruction);
    }

    // Opcode is not 8 bits, try 10
    else if(((instruction & 0xFFC0) >> 6) == 0062) // asr
    {
        asr(instruction);
    }
    else if(((instruction & 0xFFC0) >> 6) == 0063) // asl
    {
        asl(instruction);
    }

    // Opcode is not 10 bits, try 16
    else if(instruction == 0000) // halt
    {
        halt(instruction);
    }

    // Invalid opcode
    else
    {
        printf("Invalid opcode: %d\n", instruction);
        exit(1);
    }

    // Increment instruction execution count
    inst_execs++;

    // Increment instruction fetch count
    inst_fetches++;
}

void get_operand( addr_phrase_t *phrase) {
    #ifdef DEBUG
    printf("get_operand: mode = %d, reg = %d\n", phrase->mode, phrase->reg);
    #endif

    assert( (phrase->mode >= 0) && (phrase->mode <= 7) );
    assert( (phrase->reg >= 0) && (phrase->reg <= 7) );
    
    // Decide register mode
    switch( phrase->mode ) {

        /* register */
        case 0:
            phrase->value = reg[phrase->reg];
            break;

        /* register indirect */
        case 1:
            phrase->addr = reg[phrase->reg ];  /* address is in the register */
            assert( phrase->addr < MEMSIZE );
            phrase->value = memory[phrase->addr ];  /* value is in memory */
            memory_reads++;
            assert( phrase->value < 0200000 );

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* autoincrement (post reference) */
        case 2:
            // Update PC mode
            if( phrase->reg == 7 ) { // Immediate mode
                #ifdef DEBUG
                printf("get_operand: immediate mode\n");
                #endif

                // Operand is in the next word
                phrase->value = memory[reg[7]];
                inst_fetches++;
                reg[7] += 2;
                assert( phrase->value < 0200000 );

                // Immediate mode has no address
                phrase->addr = 0;
            }
            // Update register mode
            else {
                phrase->addr = reg[phrase->reg];  /* address is in the register */
                assert( phrase->addr < MEMSIZE );
                phrase->value = memory[phrase->addr];  /* value is in memory */
                memory_reads++;
                assert( phrase->value < 0200000 );
                reg[phrase->reg] += 2;  /* increment the register */
            }

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* autoincrement indirect */
        case 3:
            // Update PC mode
            if( phrase->reg == 7 ) { // Absolute mode
                #ifdef DEBUG
                printf("get_operand: absolute mode\n");
                #endif

                // The address of the operand is in the next word
                phrase->addr = memory[reg[7]];
                memory_reads++;
                inst_fetches++;
                reg[7] += 2;
                assert( phrase->addr < MEMSIZE );
            }

            // Update register mode
            else {
                phrase->addr = reg[phrase->reg];  /* address is in the register */
                assert( phrase->addr < MEMSIZE );
                phrase->addr = memory[phrase->addr];  /* address is in memory */
                memory_reads++;
                assert( phrase->addr < MEMSIZE );
            }

            // The value of the operand is in memory
            phrase->value = memory[phrase->addr];
            memory_reads++;
            assert( phrase->value < 0200000 );

            // Increment the register
            reg[phrase->reg] += 2;

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* autodecrement */
        case 4:
            reg[phrase->reg] -= 2;  /* decrement the register */
            phrase->addr = reg[phrase->reg];  /* address is in the register */
            assert( phrase->addr < MEMSIZE );
            phrase->value = memory[phrase->addr];  /* value is in memory */
            memory_reads++;
            assert( phrase->value < 0200000 );
            break;

        /* autodecrement indirect */
        case 5:
            reg[phrase->reg] -= 2;  /* decrement the register */
            phrase->addr = reg[phrase->reg];  /* address is in the register */
            assert( phrase->addr < MEMSIZE );
            phrase->addr = memory[phrase->addr];  /* address is in memory */
            memory_reads++;
            assert( phrase->addr < MEMSIZE );
            phrase->value = memory[phrase->addr];  /* value is in memory */
            memory_reads++;

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* index */
        case 6:
            // Update PC mode
            if( phrase->reg == 7 ) { // Relative mode
                // The address of the operand is in the next word of the instruction added to the PC
                phrase->addr = memory[reg[7]] + reg[7];
                inst_fetches++;
                reg[7] += 2;
                assert( phrase->addr < MEMSIZE );
            }

            // Update register mode
            else {
                phrase->addr = memory[reg[7]] + reg[phrase->reg];
                inst_fetches++;
                reg[7] += 2;
                assert( phrase->addr < MEMSIZE );
            }

            // Get value from memory
            phrase->value = memory[phrase->addr];
            memory_reads++;
            assert( phrase->value < 0200000 );

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* index indirect */
        case 7:
            // Update PC mode
            if( phrase->reg == 7 ) { // Relative deferred mode
                // The address of the address of the operand is the next word of the instruction added to reg[7]
                phrase->addr = memory[reg[7]] + reg[7];
                inst_fetches++;
                assert( phrase->addr < MEMSIZE );
            }

            // Update register mode
            else {
                phrase->addr = memory[reg[7]] + reg[phrase->reg];
                inst_fetches++;
                // Fix out of bounds addresses
                while( phrase->addr >= MEMSIZE ) {
                    phrase->addr -= MEMSIZE;
                }
                assert( phrase->addr < MEMSIZE );
            }
            reg[7] += 2;

            // Get value from memory
            phrase->addr = memory[phrase->addr];
            memory_reads++;
            assert( phrase->addr < MEMSIZE );
            phrase->value = memory[phrase->addr];
            memory_reads++;
            assert( phrase->value < 0200000 );

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        default:
            printf("unimplemented address mode %o\n", phrase->mode);
            exit(1);
    }
}

void update_operand(addr_phrase_t *phrase) {
    assert( (phrase->mode >= 0) && (phrase->mode <= 7) );
    assert( (phrase->reg >= 0) && (phrase->reg <= 7) );

    // Decide register mode
    switch( phrase->mode ) {

        /* register */
        case 0:
            reg[phrase->reg] = phrase->value;
            break;

        /* register indirect */
        case 1:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autoincrement (post reference) */
        case 2:
            // Update PC mode
            if( phrase->reg == 7 ) { // Immediate mode
                // Immediate mode has no address
                // Do nothing
            }
            // Update register mode
            else {
                memory[phrase->addr] = phrase->value;
                memory_writes++;
            }
            break;

        /* autoincrement indirect */
        case 3:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autodecrement */
        case 4:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autodecrement indirect */
        case 5:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* index */
        case 6:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* index indirect */
        case 7:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;
    }
}

void put_operand(addr_phrase_t *phrase) {
    assert( (phrase->mode >= 0) && (phrase->mode <= 7) );
    assert( (phrase->reg >= 0) && (phrase->reg <= 7) );

    // Decide register mode
    switch( phrase->mode ) {

        /* register */
        case 0:
            reg[phrase->reg] = phrase->value;
            break;

        /* register indirect */
        case 1:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autoincrement (post reference) */
        case 2:
            // Update PC mode
            if( phrase->reg == 7 ) { // Immediate mode
                // Immediate mode has no address
                // Do nothing
            }
            // Update register mode
            else {
                memory[phrase->addr] = phrase->value;
                memory_writes++;
            }
            break;

        /* autoincrement indirect */
        case 3:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autodecrement */
        case 4:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* autodecrement indirect */
        case 5:
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* index */
        case 6:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;

        /* index indirect */
        case 7:
            // Get value from memory
            memory[phrase->addr] = phrase->value;
            memory_writes++;
            break;
    }
}

void add(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------add: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Save old value of destination
    uint16_t old_dst = dst.value;

    // Add source and destination
    dst.value = dst.value + src.value;

    // Ensure that the result is 16 bits
    dst.value = dst.value & 0xFFFF;

    // Write back to memory
    update_operand(&dst);

    // Set condition codes
    n = dst.value >> 15;
    z = (dst.value == 0);
    v = ((old_dst >> 15) == (src.value >> 15)) && ((dst.value >> 15) != (src.value >> 15));
    c = (dst.value < old_dst);

    // instruction trace
    if (trace || verbose)
    {
        printf("add instruction sm %o, sr %o dm %o dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // verbose
    if (verbose) {
        printf("  src.value = %07o\n  dst.value = %07o\n  result    = %07o\n", src.value, old_dst, dst.value);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        pregs();
    }
}

void asl(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------asl: operand = %d----------\n", operand);
    #endif

    // Get address phrase
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);
    
    // Get value
    get_operand(&dst);
    
    // Save old value
    uint16_t old_value = dst.value;

    // Shift left
    dst.value = dst.value << 1;

    // Clamp to 16 bits
    dst.value = dst.value & 0xFFFF;

    // Write back to memory
    update_operand(&dst);

    // Set condition codes
    n = dst.value & 0x8000;
    z = dst.value == 0;
    v = (old_value & 0x8000) != (dst.value & 0x8000);
    c = (old_value & 0x8000) != 0;

    // instruction trace
    if (trace || verbose)
    {
        printf("asl instruction dm %o dr %o\n", dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("  dst.value = %07o\n  result    = %07o\n", old_value, dst.value);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        pregs();
    }
}

void asr(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------asr: operand = %d----------\n", operand);
    #endif

    // Get address phrase
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get value
    get_operand(&dst);

    // Save old value
    uint16_t old_value = dst.value;

    // Copy sign bit
    uint16_t sign_bit = dst.value & 0x8000;

    // Shift right
    dst.value = dst.value >> 1;

    // Write sign bit
    dst.value = dst.value | sign_bit;

    // Ensure that the result is 16 bits
    dst.value = dst.value & 0xFFFF;

    // Write back to memory
    update_operand(&dst);

    // Set condition codes
    n = dst.value & 0x8000;
    z = dst.value == 0;
    v = (old_value & 0x8000) != (dst.value & 0x8000);
    c = (old_value & 0x0001) != 0;

    // instruction trace
    if (trace || verbose)
    {
        printf("asr instruction dm %o dr %o\n", dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("  dst.value = %07o\n  result    = %07o\n", old_value, dst.value);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        pregs();
    }
}

void beq(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------beq: operand = %d----------\n", operand);
    #endif

    int offset = operand & 0377;          /* 8-bit signed offset */ 

    offset = offset << 24;       /* sign extend to 32 bits */ 
    offset = offset >> 24; 

    #ifdef DEBUG
    printf("operand: %o, offset: %o\n", operand, offset);
    #endif

    if(z){ 
        reg[7] = (reg[7] + (offset << 1)) & 0177777; 
        branch_taken++; 
    }

    branch_execs++;

    // instruction trace
    if (trace || verbose) {
        printf("beq instruction with offset %04o\n", offset);
    }

    // value dump
    if (verbose) {
        // register dump
        pregs();
    }
}

void bne(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------bne: operand = %d----------\n", operand);
    #endif

    int offset = operand & 0377;          /* 8-bit signed offset */ 

    offset = offset << 24;       /* sign extend to 32 bits */ 
    offset = offset >> 24; 

    #ifdef DEBUG
    printf("operand: %o, offset: %o\n", operand, offset);
    #endif

    if(!z){ 
        reg[7] = (reg[7] + (offset << 1)) & 0177777; 
        branch_taken++; 
    } 

    branch_execs++;

    // instruction trace
    if (trace || verbose) {
        printf("bne instruction with offset 0%03o\n", offset);
    }

    // value dump
    if (verbose) {
        // register dump
        pregs();
    }
}

void br(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------br: operand = %d----------\n", operand);
    #endif
    
    // Get 8 bit offset
    reg[7] += 2 * (int8_t)operand;

    // Update branches taken
    branch_taken++;
    branch_execs++;

    // instruction trace
    if (trace || verbose) {
        printf("br instruction with offset %04o\n", (int8_t) operand);
    }

    // value dump
    if (verbose) {
        // register dump
        pregs();
    }
}

void cmp(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------cmp: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Compare
    uint16_t result = src.value - dst.value;

    // Set flags
    n = result & 0x8000;
    z = result == 0;
    v = (src.value & 0x8000) != (dst.value & 0x8000);
    c = (src.value < dst.value);

    // instruction trace
    if (trace || verbose) {
        printf("cmp instruction sm %o, sr %o dm %o dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("  src.value = %07o\n  dst.value = %07o\n  result    = %07o\n", src.value, dst.value, result);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);

        // register dump
        pregs();
    }
}

void halt(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------halt: operand = %d----------\n", operand);
    #endif

    // Halt
    running = 0;

    // instruction trace
    if (trace || verbose) {
        printf("halt instruction\n");
    }

    // value dump
    if (verbose) {
        // register dump
        pregs();
    }
}

void mov(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------mov: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Move
    dst.value = src.value;

    // If moving byte to register, sign extend into bits 15-8
    if (dst.mode == 0 && dst.reg > 0) {
        dst.value = dst.value & 0x00FF;
        if (dst.value & 0x0080) {
            dst.value = dst.value | 0xFF00;
        }
    }

    // Store destination value in memory or register
    put_operand(&dst);

    // Set flags
    n = (dst.value & 0x8000) >> 15;
    z = (dst.value == 0);
    v = 0;
    c = 0;

    // instruction trace
    if (trace || verbose)
    {
        printf("mov instruction sm %o, sr %o dm %o dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    if (verbose) {
        printf("  src.value = %07o\n", src.value);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        pregs();
    }
}

void sob(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------sob: operand = %d----------\n", operand); 
    #endif

    // Get register index
    int reg_index = operand & 0x0007;
    
    // Get 6 bit offset
    int offset = operand & 0x003F;

    // Subtract one from register
    reg[reg_index]--;

    // Branch if register is not zero
    if (reg[reg_index] != 0)
    {
        reg[7] -= 2 * offset;
        branch_taken++;
    }

    branch_execs++;

    // instruction trace
    if (trace || verbose)
    {
        printf("sob instruction reg %o with offset %03o\n", reg_index, offset);
    }

    // value dump
    if (verbose) {
      
        // register dump
        pregs();
    }
}

void sub(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------sub: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Subtract
    uint16_t result = dst.value - src.value;

    // Ensure result is 16 bits
    result &= 0xFFFF;

    // Store destination value in memory or register
    if (dst.mode == 0)
    {
        reg[dst.reg] = result;
    }
    else
    {
        memory[dst.value] = result;
    }

    // Set flags
    n = (result & 0x8000) >> 15;
    z = (result == 0);
    v = ((dst.value & 0x8000) >> 15) ^ ((src.value & 0x8000) >> 15) ^ ((result & 0x8000) >> 15);
    c = ((dst.value & 0x8000) >> 15) ^ ((src.value & 0x8000) >> 15) ^ ((result & 0x8000) >> 15);

    // instruction trace
    if (trace || verbose)
    {
        printf("sub instruction sm %o, sr %o dm %o dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("  src.value = %07o\n  dst.value = %07o\n  result    = %07o\n", src.value, dst.value, result);
        printf("  nzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        pregs();
    }
}

void pstats() {
    printf("\nexecution statistics (in decimal):\n");
    printf("  instructions executed     = %d\n", inst_execs);
    printf("  instruction words fetched = %d\n", inst_fetches);
    printf("  data words read           = %d\n", memory_reads);
    printf("  data words written        = %d\n", memory_writes);
    printf("  branches executed         = %d\n", branch_execs);
    printf("  branches taken            = %d (%0.1f%%)\n", branch_taken, (float) ((branch_taken * 100) / branch_execs));

    // Print first 20 words of memory after execution halts
    printf("\nfirst 20 words of memory after execution halts:\n");
    for(int i = 0; i < 40; i += 2) printf("  %05o: %06o\n", i, memory[i]);
}

void pregs() {
    printf("  R0:%07o  R2:%07o  R4:%07o  R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
    printf("  R1:%07o  R3:%07o  R5:%07o  R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
}