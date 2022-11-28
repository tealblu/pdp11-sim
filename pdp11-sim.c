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
#define MEMSIZE 32*1024

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
void get_operand(addr_phrase_t *phrase);
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

// Main function
int main(int argc, char *argv[])
{
    running = true;

    // Run command format: ./a.out <flags>
    // Flags: -t (instruction trace), -v (verbose trace)

    // Check for flags
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0)
        {
            trace = true;
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose = true;
        }
        else
        {
            printf("Invalid flag: %s\n", argv[i]);
            exit(1);
        }
    }

    // verbose trace
    if (verbose)
    {
        printf("Reading words in octal from stdin:\n");
    }

    // Read words in octal from stdin
    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        // Read in octal
        memory[i] = (uint16_t)strtol(line, NULL, 8);

        // verbose trace
        if (verbose)
        {
            printf("%07o\n", memory[i]);
        }

        i++;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("\ninstruction trace:\n");
    }

    // Set PC to 0
    reg[7] = 0;

    // Loop through memory
    while (reg[7] < MEMSIZE && running)
    {
        if(trace || verbose)
        {
            printf("at %05o: ", reg[7]);
        }

        // Get instruction from memory
        uint16_t instruction = memory[reg[7]++];

        #ifdef DEBUG
        printf("\nexecuting instruction %05o at address %05o\n", instruction, reg[7]-1);
        #endif

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

        // ensure PC is not out of bounds
        if(reg[7] >= MEMSIZE)
        {
            printf("PC out of bounds: %d\n", reg[7]);
            exit(1);
        }
    }

    // Print execution statistics
    printf("\nexecution statistics (in decimal):\n");
    printf("\tinstructions executed: %d\n", inst_execs);
    printf("\tinstruction words fetched: %d\n", inst_fetches);
    printf("\tdata words read: %d\n", memory_reads);
    printf("\tdata words written: %d\n", memory_writes);
    printf("\tbranches executed: %d\n", branch_execs);
    printf("\tbranches taken: %d\n", branch_taken);

    // Print first 20 words of memory after execution halts
    printf("\nfirst 20 words of memory after execution halts:\n");
    for(int i = 0; i < 20; i++)
    {
        printf("%05o: %07o\n", i, memory[i]);
    }
}

// Function definitions
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
            phrase->value = reg[phrase->reg];  /*value is in the register */
            assert( phrase->value < 0200000 );
            phrase->addr = 0; /* unused */
            break;

        /* register indirect */
        case 1:
            phrase->addr = reg[phrase->reg ];  /* address is in the register */
            assert( phrase->addr < 0200000 );
            phrase->value = memory[phrase->addr ];  /* value is in memory */
            assert( phrase->value < 0200000 );
            memory_reads++;

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
                phrase->value = memory[reg[7]++];
                assert( phrase->value < 0200000 );
                inst_fetches++;

                // Immediate mode has no address
                phrase->addr = 0;
            }
            // Update register mode
            else {
                phrase->addr = reg[phrase->reg];  /* address is in the register */
                assert( phrase->addr < 0200000 );
                phrase->value = memory[phrase->addr];  /* value is in memory */
                assert( phrase->value < 0200000 );
                memory_reads++;
                reg[phrase->reg]++;  /* increment the register */
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
                phrase->addr = memory[reg[7]++];
                memory_reads++;
            }

            // Update register mode
            else {
                phrase->addr = reg[phrase->reg ];  /* address is in the register */
                assert( phrase->addr < 0200000 );
                reg[phrase->reg ]++;  /* increment the register */
            }

            // Get value from memory
            phrase->value = memory[phrase->addr];
            assert( phrase->value < 0200000 );
            memory_reads++;

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        /* autodecrement */
        case 4:
            reg[ phrase->reg ] = ( reg[ phrase->reg ] - 2 ) & 0177777;
            phrase->addr = reg[ phrase->reg ]; /* address is in the register */
            assert( phrase->addr < 0200000 );
            phrase->value = memory[ phrase->addr >> 1 ]; /* adjust to word addr */
            assert( phrase->value < 0200000 );
            memory_reads++;
            break;

        /* autodecrement indirect */
        case 5:
            reg[ phrase->reg ] = ( reg[ phrase->reg ] - 2 ) & 0177777;
            phrase->addr = reg[ phrase->reg ]; /* addr of addur is in reg */
            assert( phrase->addr < 0200000 );
            phrase->addr = memory[ phrase->addr >> 1 ]; /* adjust to word addr */
            assert( phrase->addr < 0200000 );
            phrase->value = memory[ phrase->addr >> 1 ]; /* adjust to word addr */
            assert( phrase->value < 0200000 );
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
                reg[7]++;
                assert( phrase->addr < 0200000 );
            }

            // Update register mode
            else {
                phrase->addr = memory[reg[7]] + reg[phrase->reg];
                reg[7]++;
                assert( phrase->addr < 0200000 );
            }

            // Get value from memory
            phrase->value = memory[phrase->addr];
            assert( phrase->value < 0200000 );
            memory_reads++;

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
                reg[7]++;
                assert( phrase->addr < 0200000 );
            }

            // Update register mode
            else {
                phrase->addr = memory[reg[7]] + reg[phrase->reg];
                reg[7]++;
                assert( phrase->addr < 0200000 );
            }

            // Get value from memory
            phrase->addr = memory[phrase->addr];
            assert( phrase->addr < 0200000 );
            phrase->value = memory[phrase->addr];
            assert( phrase->value < 0200000 );
            memory_reads++;

            #ifdef DEBUG
            printf("get_operand: addr: %07o, value: %07o\n", phrase->addr, phrase->value);
            #endif
            break;

        default:
            printf("unimplemented address mode %o\n", phrase->mode);
            exit(1);
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

    // Add source and destination
    dst.value = dst.value + src.value;

    // Write back to memory
    memory[dst.addr] = dst.value;
    memory_writes++;

    // Set condition codes
    n = dst.value & 0x8000;
    z = dst.value == 0;
    v = (src.value & 0x8000) == (dst.value & 0x8000);
    c = dst.value > 0xFFFF;

    // instruction trace
    if (trace || verbose)
    {
        printf("add instruction sm %o, sr %o, dm %o, dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // verbose
    if (verbose) {
        printf("\tsrc.value = %07o\n\tdst.value = %07o\n\tresult = %07o\n", src.value, dst.value, dst.value);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void asl(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------asl: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);
    
    // Shift left
    memory[dst.addr] = dst.value << 1;

    // Set condition codes
    n = dst.value & 0x8000;
    z = dst.value == 0;
    v = (src.value & 0x8000) == (dst.value & 0x8000);
    c = dst.value > 0xFFFF;

    // instruction trace
    if (trace || verbose)
    {
        printf("asl instruction dm %o, dr %o\n", dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("\tdst.value = %07o\n\tresult = %07o\n", dst.value, memory[dst.addr]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void asr(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------asr: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Sign extend source value
    if (src.value & 0x8000)
    {
        src.value |= 0xFFFF0000;
    }

    // Shift destination right
    dst.value = dst.value >> 1;

    // Trim to 16 bits
    dst.value &= 0xFFFF;

    // Write back to memory
    memory[dst.addr] = dst.value;
    memory_writes++;

    // Set condition codes
    n = dst.value & 0x8000;
    z = dst.value == 0;
    v = (src.value & 0x8000) == (dst.value & 0x8000);
    c = dst.value > 0xFFFF;

    // instruction trace
    if (trace || verbose)
    {
        printf("asr instruction dm %o, dr %o\n", dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("\tdst.value = %07o\n\tresult = %07o\n", dst.value, memory[dst.addr]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void beq(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------beq: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Branch if zero
    if (z)
    {
        reg[7] = dst.value;
    }

    // Update branches taken
    branch_taken++;

    // instruction trace
    if (trace || verbose) {
        printf("beq instruction with offset: %04o\n", dst.value);
    }

    // value dump
    if (verbose) {
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void bne(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------bne: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Branch if not zero
    if (!z)
    {
        reg[7] = reg[7] + 2 * dst.value;
    }

    // Validate PC
    if (reg[7] > 0xFFFF)
    {
        reg[7] = reg[7] & 0xFFFF;
    }

    // Update branches taken
    branch_taken++;

    // instruction trace
    if (trace || verbose) {
        printf("bne instruction with offset %04o\n", dst.value);
    }

    // value dump
    if (verbose) {
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void br(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------br: operand = %d----------\n", operand);
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Branch
    reg[7] = reg[7] + 2 * dst.value;

    // Update branches taken
    branch_taken++;

    // instruction trace
    if (trace || verbose) {
        printf("br instruction with offset %04o\n", dst.value);
    }

    // value dump
    if (verbose) {
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
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
    int result = dst.value - src.value;

    // Set flags
    n = (result & 0x8000) >> 15;
    z = (result == 0);
    v = ((dst.value & 0x8000) >> 15) ^ ((src.value & 0x8000) >> 15) ^ ((result & 0x8000) >> 15);
    c = ((dst.value & 0x8000) >> 15) ^ ((src.value & 0x8000) >> 15) ^ ((result & 0x8000) >> 15);

    // instruction trace
    if (trace || verbose) {
        printf("cmp instruction sm %o, sr %o, dm %o, dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("\tsrc.value = %07o\n\tdst.value = %07o\n\tresult = %07o\n", src.value, dst.value, result);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);

        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
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
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
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

    // Store destination value in memory or register
    if (dst.mode == 0)
    {
        reg[dst.reg] = dst.value;
    }
    else
    {
        memory[dst.value] = dst.value;
    }

    // Set flags
    n = (dst.value & 0x8000) >> 15;
    z = (dst.value == 0);
    v = 0;
    c = 0;

    // instruction trace
    if (trace || verbose)
    {
        printf("mov instruction sm %o, sr %o, dm %o, dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    if (verbose) {
        printf("\tsrc.value = %07o\n", src.value);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void sob(uint16_t operand)
{
    #ifdef DEBUG
    printf("----------sob: operand = %d----------\n", operand); 
    #endif

    // Get source and destination address phrases
    src.mode = (operand & 0x0E00) >> 9;
    src.reg = (operand & 0x01C0) >> 6;
    dst.mode = (operand & 0x0038) >> 3;
    dst.reg = (operand & 0x0007);

    // Get source and destination values
    get_operand(&src);
    get_operand(&dst);

    // Subtract one
    dst.value--;

    // Update register
    if (dst.mode == 0)
    {
        reg[dst.reg] = dst.value;
    }

    // Update PC
    if (dst.value != 0)
    {
        reg[7] = reg[7] - 2 * src.value;
    }

    // Set flags
    n = (dst.value & 0x8000) >> 15;
    z = (dst.value == 0);
    v = 0;
    c = 0;

    // instruction trace
    if (trace || verbose)
    {
        printf("sob instruction sm %o, sr %o, dm %o, dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("\tdst.value = %07o\n", dst.value);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
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
    int result = dst.value - src.value;

    // Store destination value in memory or register
    if (dst.mode == 0)
    {
        reg[dst.reg] = result;
    }
    else
    {
        memory[dst.value] = result;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("sub instruction sm %o, sr %o, dm %o, dr %o\n", src.mode, src.reg, dst.mode, dst.reg);
    }

    // value dump
    if (verbose) {
        printf("\tsrc.value = %07o\n\tdst.value = %07o\n\tresult = %07o\n", src.value, dst.value, result);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}