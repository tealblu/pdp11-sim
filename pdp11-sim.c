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
void add(uint16_t operand, uint16_t address);
void asl(uint16_t operand, uint16_t address);
void asr(uint16_t operand, uint16_t address);
void beq(uint16_t operand, uint16_t address);
void bne(uint16_t operand, uint16_t address);
void br(uint16_t operand, uint16_t address);
void cmp(uint16_t operand, uint16_t address);
void halt(uint16_t operand, uint16_t address);
void mov(uint16_t operand, uint16_t address);
void sob(uint16_t operand, uint16_t address);
void sub(uint16_t operand, uint16_t address);

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

        // Get address
        uint16_t address = operand;

        // Get 4 bit opcode
        uint16_t opcode = (instruction & 0xF000) >> 12;
        // opcodes in octal: 01 = mov, 02 = cmp, 06 = add, 016 = sub, 077 = sob, 001 = br, 002 = bne, 003 = beq, 0062 = asr, 0063 = asl, 0000 = halt

#ifdef DEBUG
        printf("PC: %d, instruction: %d, opcode: %d, operand: %d, address: %d\n", PC, instruction, opcode, operand, address);
#endif

        // Execute 4 bit opcode
        if(opcode == 1) // mov
        {
            mov(operand, address);
        }
        else if(opcode == 2) // cmp
        {
            cmp(operand, address);
        }
        else if(opcode == 6) // add
        {
            add(operand, address);
        }
        else if(opcode == 016) // sub
        {
            sub(operand, address);
        }

        // Opcode is not 4 bits, try 7
        else if(((instruction & 0xFE00) >> 9) == 077) // sob
        {
            sob(operand, address);
        }

        // Opcode is not 7 bits, try 8
        else if(((instruction & 0xFF00) >> 8) == 001) // br
        {
            br(operand, address);
        }
        else if(((instruction & 0xFF00) >> 8) == 002) // bne
        {
            bne(operand, address);
        }
        else if(((instruction & 0xFF00) >> 8) == 003) // beq
        {
            beq(operand, address);
        }

        // Opcode is not 8 bits, try 10
        else if(((instruction & 0xFFC0) >> 6) == 0062) // asr
        {
            asr(operand, address);
        }
        else if(((instruction & 0xFFC0) >> 6) == 0063) // asl
        {
            asl(operand, address);
        }

        // Opcode is not 10 bits, try 16
        else if(instruction == 0000) // halt
        {
            halt(operand, address);
        }

        // Invalid opcode
        else
        {
            printf("Invalid opcode: %d\n", opcode);
            exit(1);
        }
    }
}

// Function definitions
void add(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("add\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check if operand is a register
    uint16_t value;
    if (operand >> 8 == 0)
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get value
        value = reg[regNum2];
    }
    else
    {
        // get value from memory
        value = memory[PC++];
    }

    // get sign bit
    uint16_t signBit = value >> 8;

    // get value
    if (signBit == 1)
    {
        value = value | 0xFE00;
    }

    // add value to register
    reg[regNum] += value;

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: add instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, reg[regNum] - value, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void asl(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("asl\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get shift amount
    uint16_t shiftAmount = operand & 0x0007;

    // get sign bit
    uint16_t signBit = reg[regNum] >> 15;

    // shift left
    reg[regNum] = reg[regNum] << shiftAmount;

    // set sign bit
    reg[regNum] = reg[regNum] | (signBit << 15);

    // clamp result to 16 bits
    reg[regNum] = reg[regNum] & 0xFFFF;

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: asl instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", shiftAmount, reg[regNum] >> shiftAmount, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void asr(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("asr\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get shift amount
    uint16_t shiftAmount = operand & 0x0007;

    // get sign bit
    uint16_t signBit = reg[regNum] >> 15;

    // sign extend to 32 bits
    uint32_t value = reg[regNum];

    // shift right
    value = value >> shiftAmount;

    // clamp to 16 bits
    reg[regNum] = value & 0xFFFF;

    // set sign bit
    reg[regNum] = reg[regNum] | (signBit << 15);

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: asr instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", shiftAmount, reg[regNum] << shiftAmount, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void beq(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("beq\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get value
    uint16_t value = reg[regNum];

    // get sign bit
    uint16_t signBit = value >> 15;

    // get value
    if (signBit == 1)
    {
        value = value | 0xFE00;
    }

    // check if zero flag is set
    if (((reg[regNum] >> 14) & 1) == 1)
    {
        // branch
        PC += value;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: beq instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void bne(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("bne\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get value
    uint16_t value = reg[regNum];

    // get sign bit
    uint16_t signBit = value >> 15;

    // get value
    if (signBit == 1)
    {
        value = value | 0xFE00;
    }

    // check if zero flag is set
    if (((reg[regNum] >> 14) & 1) == 0)
    {
        // branch
        PC += value;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: bne instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void br(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("br\n");
    #endif

    // get value
    uint16_t value = operand & 0x01FF;

    // get sign bit
    uint16_t signBit = value >> 8;

    // get value
    if (signBit == 1)
    {
        value = value | 0xFE00;
    }

    // branch
    PC += value;

    // get register number
    uint16_t regNum = operand >> 9;

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: br instruction sm %o, sr %o, dm %o, dr %o\n", PC - value, signBit, 0, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void cmp(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("cmp\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // get value
    uint16_t value = reg[regNum];

    // get sign bit
    uint16_t signBit = value >> 15;

    // get value
    if (signBit == 1)
    {
        value = value | 0xFE00;
    }

    // get register number
    uint16_t regNum2 = operand & 0x0007;

    // get value
    uint16_t value2 = reg[regNum2];

    // get sign bit
    uint16_t signBit2 = value2 >> 15;

    // get value
    if (signBit2 == 1)
    {
        value2 = value2 | 0xFE00;
    }

    // check if zero flag is set
    if (value == value2)
    {
        // set zero flag
        reg[regNum] = reg[regNum] | 0x4000;
    }
    else
    {
        // clear zero flag
        reg[regNum] = reg[regNum] & 0xBFFF;
    }

    // check if negative flag is set
    if (value < value2)
    {
        // set negative flag
        reg[regNum] = reg[regNum] | 0x8000;
    }
    else
    {
        // clear negative flag
        reg[regNum] = reg[regNum] & 0x7FFF;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: cmp instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, signBit2, regNum2);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, value2, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);

        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void halt(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("halt\n");
    #endif

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: halt instruction sm %o, sr %o, dm %o, dr %o\n", PC, 0, 0, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", 0, 0, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[0] >> 15) & 1, (reg[0] >> 14) & 1, (reg[0] >> 13) & 1, (reg[0] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }

    // halt
    exit(1);
}

void mov(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("mov\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check if operand is a register
    uint16_t value;
    if (operand >> 8 == 0)
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get value
        value = reg[regNum2];
    }
    else
    {
        // get value from memory
        value = memory[PC++];
    }

    // move value to register
    reg[regNum] = value;

    // get sm
    uint16_t sm = value >> 15;

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: mov instruction sm %o, sr %o, dm %o, dr %o\n", PC, sm, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, reg[regNum] - value, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void sob(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("sob\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 6;

    // get value
    uint16_t value = reg[regNum];

    // decrement value
    value--;

    // check if value is zero
    if (value == 0)
    {
        // get offset
        uint16_t offset = operand & 0x003F;

        // get address
        uint16_t address = PC - offset;

        // set PC
        PC = address;
    }
    else
    {
        // set value
        reg[regNum] = value;
    }

    // instruction trace
    if (trace || verbose) {
        printf("at %07o: sob instruction sm %o, sr %o, dm %o, dr %o\n", PC, 0, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", 0, value, 0);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void sub(uint16_t operand, uint16_t address)
{
    #ifdef DEBUG
    printf("sub\n");
    #endif

    // get register number
    uint16_t regNum = operand >> 9;

    // check if operand is a register
    uint16_t value;
    if (operand >> 8 == 0)
    {
        // get register number
        uint16_t regNum2 = operand & 0x0007;

        // get value
        value = reg[regNum2];
    }
    else
    {
        // get value from memory
        value = memory[++PC];
    }

    // subtract value from register
    reg[regNum] = reg[regNum] - value;

    // get sm
    uint16_t sm = value >> 15;

    // instruction trace
    if (trace || verbose)
    {
        printf("at %07o: sub instruction sm %o, sr %o, dm %o, dr %o\n", PC, sm, regNum, 0, 0);
        printf("\tsrc.value = %07o, dst.value = %07o, result = %07o\n", value, reg[regNum] + value, reg[regNum]);
        printf("\tnzvc bits = 4'b%d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}