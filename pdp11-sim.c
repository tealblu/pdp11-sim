#define DEBUG true

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

    // verbose trace
    if (verbose)
    {
        printf("instruction trace:\n");
    }

    // Set PC to 0
    PC = 0;

    // Loop through memory
    while (PC < MEMSIZE)
    {
        // Get instruction
        uint16_t instruction = memory[PC];

        // Get opcode
        uint16_t opcode = instruction >> 12;

        // Get operand
        uint16_t operand = instruction & 0x0FFF;

        // Get address
        uint16_t address = operand;

#ifdef DEBUG
        printf("PC: %d, instruction: %d, opcode: %d, operand: %d, address: %d\n", PC, instruction, opcode, operand, address);
#endif

        // Execute instruction
        // opcodes in octal: 01 = mov, 02 = cmp, 06 = add, 016 = sub, 077 = sob, 001 = br, 002 = bne, 003 = beq, 0062 = asr, 0063 = asl, 0000 = halt
        
        // get number of bits of opcode
        int numBits = (int)log2(opcode) + 1;

#ifdef DEBUG
        printf("numBits: %d\n", numBits);
#endif

        // check for halt
        if (opcode == 0)
        {
            halt(operand, address);
        }

        // for 4 bit opcodes
        else if (numBits <= 4)
        {
            switch (opcode)
            {
            case 1:
                mov(operand, address);
                break;
            case 2:
                cmp(operand, address);
                break;
            case 6:
                add(operand, address);
                break;
            case 16:
                sub(operand, address);
                break;
            default:
                printf("Invalid opcode: %o\n", opcode);
                exit(1);
            }
        }

        // for 7 bit opcodes
        else if (numBits == 7)
        {
            switch (opcode)
            {
            case 77:
                sob(operand, address);
                break;
            default:
                printf("Invalid opcode: %o\n", opcode);
                exit(1);
            }
        }

        // for 8 bit opcodes
        else if (numBits == 8)
        {
            switch (opcode)
            {
            case 001:
                br(operand, address);
                break;
            case 002:
                bne(operand, address);
                break;
            case 003:
                beq(operand, address);
                break;
            default:
                printf("Invalid opcode: %o\n", opcode);
                exit(1);
            }
        }

        // for 10 bit opcodes
        else if (numBits == 10)
        {
            switch (opcode)
            {
            case 062:
                asr(operand, address);
                break;
            case 063:
                asl(operand, address);
                break;
            default:
                printf("Invalid opcode: %o\n", opcode);
                exit(1);
            }
        }

        // error
        else
        {
            printf("Invalid opcode: %o\n", opcode);
            exit(1);
        }

        // increment PC
        PC++;
    }
}

// Function definitions
void add(uint16_t operand, uint16_t address)
{
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
        // get value
        value = operand & 0x00FF;
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

    // verbose trace
    if (verbose)
    {
        printf("at %07o: add instruction sm %o, sr %o, dm %o, dr %o\n", PC, signBit, regNum, 0, 0);
        printf("src.value = %07o, dst.value = %07o, result = %07o\n", value, reg[regNum] - value, reg[regNum]);
        printf("nzvc bits = %d%d%d%d\n", (reg[regNum] >> 15) & 1, (reg[regNum] >> 14) & 1, (reg[regNum] >> 13) & 1, (reg[regNum] >> 12) & 1);
        
        // register dump
        printf("\tR0:%07o R2:%07o R4:%07o R6:%07o\n", reg[0], reg[2], reg[4], reg[6]);
        printf("\tR1:%07o R3:%07o R5:%07o R7:%07o\n", reg[1], reg[3], reg[5], reg[7]);
    }
}

void asl(uint16_t operand, uint16_t address)
{
    puts("asl");
}

void asr(uint16_t operand, uint16_t address)
{
    puts("asr");
}

void beq(uint16_t operand, uint16_t address)
{
    puts("beq");
}

void bne(uint16_t operand, uint16_t address)
{
    puts("bne");
}

void br(uint16_t operand, uint16_t address)
{
    puts("br");
}

void cmp(uint16_t operand, uint16_t address)
{
    puts("cmp");
}

void halt(uint16_t operand, uint16_t address)
{
    puts("halt");
    exit(0);
}

void mov(uint16_t operand, uint16_t address)
{
    puts("mov");
}

void sob(uint16_t operand, uint16_t address)
{
    puts("sob");
}

void sub(uint16_t operand, uint16_t address)
{
    puts("sub");
}