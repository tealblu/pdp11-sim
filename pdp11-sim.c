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

// Defines
#define MEMSIZE 32768

// Global variables
uint16_t memory[MEMSIZE]; // 16-bit memory
uint16_t reg[8]; // R0-R7
bool trace = false;
bool verbose = false;
uint16_t PC = 0; // Program counter

// Function prototypes
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
    // Run command format: ./a.out <flags> < <filename>
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
        printf("\n\ninstruction trace:\n");
    }

    // Set PC to 0
    PC = 0;

    // Loop through memory
    while (PC < MEMSIZE)
    {
        // Get instruction
        uint16_t instruction = memory[PC++];
        
        // Get operand
        uint16_t operand = instruction & 0x0FFF;

        // Get 4 bit opcode
        uint16_t opcode = (instruction & 0xF000) >> 12;
        // opcodes in octal: 01 = mov, 02 = cmp, 06 = add, 016 = sub, 077 = sob, 001 = br, 002 = bne, 003 = beq, 0062 = asr, 0063 = asl, 0000 = halt

#ifdef DEBUG
        printf("PC: %d, instruction: %d, opcode: %d, operand: %d, address: %d\n", PC, instruction, opcode, operand, address);
#endif

        // Execute 4 bit opcode
        if(opcode == 1) // mov
        {
            mov(operand);
        }
        else if(opcode == 2) // cmp
        {
            cmp(operand);
        }
        else if(opcode == 6) // add
        {
            add(operand);
        }
        else if(opcode == 016) // sub
        {
            sub(operand);
        }

        // Opcode is not 4 bits, try 7
        else if(((instruction & 0xFE00) >> 9) == 077) // sob
        {
            sob(operand);
        }

        // Opcode is not 7 bits, try 8
        else if(((instruction & 0xFF00) >> 8) == 001) // br
        {
            br(operand);
        }
        else if(((instruction & 0xFF00) >> 8) == 002) // bne
        {
            bne(operand);
        }
        else if(((instruction & 0xFF00) >> 8) == 003) // beq
        {
            beq(operand);
        }

        // Opcode is not 8 bits, try 10
        else if(((instruction & 0xFFC0) >> 6) == 0062) // asr
        {
            asr(operand);
        }
        else if(((instruction & 0xFFC0) >> 6) == 0063) // asl
        {
            asl(operand);
        }

        // Opcode is not 10 bits, try 16
        else if(instruction == 0000) // halt
        {
            halt(operand);
        }

        // Invalid opcode
        else
        {
            printf("Invalid opcode: %d\n", opcode);
            exit(1);
        }

        // ensure PC is not out of bounds
        if(PC >= MEMSIZE)
        {
            printf("PC out of bounds: %d\n", PC);
            exit(1);
        }
    }
}

// Function definitions
void add(uint16_t operand)
{
    #ifdef DEBUG
    printf("add\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // add
        reg[regNum] += reg[regNum2];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // add
        reg[regNum] += memory[reg[regNum]];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // add
        reg[regNum] += memory[reg[regNum]];
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // add
        reg[regNum] += memory[reg[indexRegNum]];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // add
        reg[regNum] += memory[address];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // add
        reg[regNum] += memory[address];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // add
        reg[regNum] += value;
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // add
        reg[regNum] += memory[address];
    }

    // set flags
    bool n, z, v, c;
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: add instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment pc accordingly
    PC += 2;
}

void asl(uint16_t operand)
{
    #ifdef DEBUG
    printf("asl\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= reg[regNum2];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= memory[reg[regNum]];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // asl
        reg[regNum] <<= memory[reg[regNum]];
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= memory[reg[indexRegNum]];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= memory[address];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= memory[address];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // asl
        reg[regNum] <<= value;
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asl
        reg[regNum] <<= memory[address];
    }

    // clamp result to 16 bits
    reg[regNum] &= 0xFFFF;

    // set flags
    bool n, z, v, c;
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: asl instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if not immediate mode
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void asr(uint16_t operand)
{
    #ifdef DEBUG
    printf("asr\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // sign extend
    reg[regNum] |= 0xFFFF0000;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= reg[regNum2];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= memory[reg[regNum]];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // asr
        reg[regNum] >>= memory[reg[regNum]];
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= memory[reg[indexRegNum]];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= memory[address];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= memory[address];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // asr
        reg[regNum] >>= value;
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // asr
        reg[regNum] >>= memory[address];
    }

    // clamp to 16 bits
    reg[regNum] &= 0xFFFF;

    // set flags
    bool n, z, v, c;
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: asr instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if not immediate mode
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void beq(uint16_t operand)
{
    #ifdef DEBUG
    printf("beq\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get z
    bool z = (operand & 0x0100) >> 8;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = reg[regNum2];
        }
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = memory[reg[regNum]];
        }

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // beq
        if(z)
        {
            reg[regNum] = memory[reg[regNum]];
        }
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = memory[reg[indexRegNum]];
        }
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = memory[address];
        }
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = memory[address];
        }
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = value;
        }
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // beq
        if(z)
        {
            reg[regNum] = memory[address];
        }
    }

    // get flags
    bool n, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: beq instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if not immediate mode
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void bne(uint16_t operand)
{
    #ifdef DEBUG
    printf("bne\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get z
    bool z = (operand & 0x0100) >> 8;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = reg[regNum2];
        }
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = memory[reg[regNum]];
        }

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // bne
        if(!z)
        {
            reg[regNum] = memory[reg[regNum]];
        }
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = memory[reg[indexRegNum]];
        }
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = memory[address];
        }
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = memory[address];
        }
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = value;
        }
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // bne
        if(!z)
        {
            reg[regNum] = memory[address];
        }
    }

    // get flags
    bool n, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }


    // instruction trace
    if (trace || verbose) {
        printf("at %07o: bne instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if needed
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void br(uint16_t operand)
{
    #ifdef DEBUG
    printf("br\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];

        // br
        reg[regNum] = reg[regNum2];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // br
        reg[regNum] = memory[reg[regNum]];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;

        // br
        reg[regNum] = memory[reg[regNum]];
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];

        // br
        reg[regNum] = memory[reg[indexRegNum]];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // br
        reg[regNum] = memory[address];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // br
        reg[regNum] = memory[address];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];

        // br
        reg[regNum] = value;
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];

        // br
        reg[regNum] = memory[address];
    }

    // get flags
    bool n, z, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: br instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if needed
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void cmp(uint16_t operand)
{
    #ifdef DEBUG
    printf("cmp\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }

    // get flags
    bool n, z, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: cmp instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);

        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if needed
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void halt(uint16_t operand)
{
    #ifdef DEBUG
    printf("halt\n");
    #endif

    // set n, z, v, c
    bool n, z, v, c;
    n = false;
    z = false;
    v = false;
    c = false;

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: halt instruction sm %o, sr %o, dm %o, dr %o\n", PC, 0, 0, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", 0, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // set PC to 0
    PC = 0;

    // halt
    exit(1);
}

void mov(uint16_t operand)
{
    #ifdef DEBUG
    printf("mov\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }

    // get flags
    bool n, z, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: mov instruction sm %o, sr %o, dm %o, dr %o\n", PC, src, dst, reg[regNum], reg[regNum]);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC if needed
    if((operand & 0x0004) == 0)
    {
        PC++;
    }
}

void sob(uint16_t operand)
{
    #ifdef DEBUG
    printf("sob\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 6;

    // get offset
    uint16_t offset = operand & 0x003F;

    // decrement
    reg[regNum]--;

    // check if zero
    if(reg[regNum] != 0)
    {
        // subtract offset
        PC -= offset;
    }

    // set n, z, v, c
    bool n, z, v, c;
    n = false;
    z = false;
    v = false;
    c = false;

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: sob instruction sm %o, sr %o, dm %o, dr %o\n", PC, 0, 0, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", 0, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC
    PC++;
}

void sub(uint16_t operand)
{
    #ifdef DEBUG
    printf("sub\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check all 8 addressing modes
    uint16_t src, dst;
    if((operand & 0x0100) == 0) // register mode
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get src and dst
        src = reg[regNum2];
        dst = reg[regNum];
    }
    else if((operand & 0x0080) == 0) // autoincrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // increment
        reg[regNum]++;
    }
    else if((operand & 0x0040) == 0) // autodecrement mode
    {
        // get src and dst
        src = memory[reg[regNum]];
        dst = reg[regNum];

        // decrement
        reg[regNum]--;
    }
    else if((operand & 0x0020) == 0) // index mode
    {
        // get index register number
        uint16_t indexRegNum = (operand & 0x0007);

        // get src and dst
        src = memory[reg[indexRegNum]];
        dst = reg[regNum];
    }
    else if((operand & 0x0010) == 0) // indirect mode
    {
        // get address
        uint16_t address = memory[reg[regNum]];

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0008) == 0) // deferred mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }
    else if((operand & 0x0004) == 0) // immediate mode
    {
        // get value
        uint16_t value = operand & 0x00FF;

        // get src and dst
        src = value;
        dst = reg[regNum];
    }
    else // absolute mode
    {
        // get address
        uint16_t address = operand & 0x00FF;

        // get src and dst
        src = memory[address];
        dst = reg[regNum];
    }

    // subtract
    reg[regNum] = dst - src;

    // set n, z, v, c
    bool n, z, v, c;
    if(reg[regNum] < 0)
    {
        n = true;
    }
    else
    {
        n = false;
    }
    if(reg[regNum] == 0)
    {
        z = true;
    }
    else
    {
        z = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0) || (src > 0 && dst < 0 && reg[regNum] > 0) || (src < 0 && dst > 0 && reg[regNum] < 0))
    {
        v = true;
    }
    else
    {
        v = false;
    }
    if((src > 0 && dst > 0 && reg[regNum] < 0) || (src < 0 && dst < 0 && reg[regNum] > 0))
    {
        c = true;
    }
    else
    {
        c = false;
    }

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: sub instruction sm %o, sr %o, dm %o, dr %o\n", PC, 0, 0, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", src, dst, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", n, z, v, c);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // increment PC
    PC++;
}