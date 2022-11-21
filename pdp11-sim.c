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
#define MEMSIZE 4000

// Global variables
uint16_t memory[MEMSIZE]; // Memory array
uint16_t reg[8]; // R0-R7
uint16_t pc; // Program counter
uint16_t ir; // Instruction register
uint16_t sr; // Status register
uint16_t sp; // Stack pointer
uint16_t base; // Base register
uint16_t limit;  // Limit register

// idk if this is needed
uint16_t operand;
uint16_t address;
uint16_t opcode;
uint16_t operand2;
uint16_t address2;
uint16_t opcode2;

// Function prototypes
void loadFile(char *filename);
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

// idk if this is needed
void printMemory();
void printRegisters();
void printStatus();
void printStack();
void printHelp();
void printAll();

// Main function
int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 2)
    {
        printf("Usage: ./a.out <filename>\n");
        return 1;
    }

    // Load file into memory
    loadFile(argv[1]);

    // Read first instruction
    ir = memory[pc];

    // Main loop
    while(true) {
        // Decode instruction
        opcode = ir >> 12;

        // Execute instruction
        switch(opcode) {
            case 0:
                halt(operand, address);
                break;
            case 1:
                br(operand, address);
                break;
            case 2:
                bne(operand, address);
                break;
            case 3:
                beq(operand, address);
                break;
            case 6:
                bvc(operand, address);
                break;
            case 7:
                bvs(operand, address);
                break;
            case 8:
                bcc(operand, address);
                break;
            case 9:
                bcs(operand, address);
                break;
            case 10:
                emt(operand, address);
                break;
            case 11:
                trap(operand, address);
                break;
            case 12:
                clrb(operand, address);
                break;
            case 13:
                comb(operand, address);
                break;
            case 14:
                incb(operand, address);
                break;
            case 15:
                decb(operand, address);
                break;
            case 16:
                negb(operand, address);
                break;
            case 17:
                adcb(operand, address);
                break;
            case 18:
                sbcb(operand, address);
                break;
            case 19:
                tstb(operand, address);
                break;
            case 20:
                rorb(operand, address);
                break;
            case 21:
                rolb(operand, address);
                break;
            case 22:
                asrb(operand, address);
                break;
            case 23:
                aslb(operand, address);
                break;
            case 24:
                mtpi(operand, address);
                break;
            case 25:
                mfpd(operand, address);
                break;
            case 26:
                sxt(operand, address);
                break;
            case 27:
                mtpd(operand, address);
                break;
            case 28:
                mfpi(operand, address);
                break;
            case 29:
                mfps(operand, address);
                break;
            case 30:
                mfps(operand, address);
                break;
    }

    return 0;
}

// Function definitions
void loadFile(char *filename)
{
    // Open file
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: Could not open file %s\n", filename);
        exit(1);
    }

    // Read file into memory
    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), file))
    {
        // Convert string to int
        memory[i] = (uint16_t)strtol(line, NULL, 16);
        i++;
    }

    // Close file
    fclose(file);

    // Set PC to 0
    pc = 0;

    // Set SP to 0
    sp = 0;

    // Set base to 0
    base = 0;

    // Set limit to 0
    limit = 0;

    // Set SR to 0
    sr = 0;

    // Set registers to 0
    for (int i = 0; i < 8; i++)
    {
        reg[i] = 0;
    }
}

void add(uint16_t operand, uint16_t address)
{
    // Add operand to register
    reg[operand] += memory[address];

    // Update status register
    if (reg[operand] == 0)
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < 0)
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > 0xFFFF)
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}

void asl(uint16_t operand, uint16_t address)
{
    // Shift left operand
    reg[operand] = reg[operand] << 1;

    // Update status register
    if (reg[operand] == 0)
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < 0)
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > 0xFFFF)
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}

void asr(uint16_t operand, uint16_t address)
{
    // Shift right operand
    reg[operand] = reg[operand] >> 1;

    // Update status register
    if (reg[operand] == 0)
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < 0)
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > 0xFFFF)
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}

void beq(uint16_t operand, uint16_t address)
{
    // Branch if equal
    if (sr & 0x0004)
    {
        pc = address;
    }
}

void bne(uint16_t operand, uint16_t address)
{
    // Branch if not equal
    if (!(sr & 0x0004))
    {
        pc = address;
    }
}

void br(uint16_t operand, uint16_t address)
{
    // Branch
    pc = address;
}

void cmp(uint16_t operand, uint16_t address)
{
    // Compare operand to register
    if (reg[operand] == memory[address])
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < memory[address])
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > memory[address])
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}

void halt(uint16_t operand, uint16_t address)
{
    // Halt
    exit(0);
}

void mov(uint16_t operand, uint16_t address)
{
    // Move operand to register
    reg[operand] = memory[address];

    // Update status register
    if (reg[operand] == 0)
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < 0)
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > 0xFFFF)
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}

void sob(uint16_t operand, uint16_t address)
{
    // Subtract one from operand
    reg[operand]--;

    // Branch if not zero
    if (reg[operand] != 0)
    {
        pc = address;
    }
}

void sub(uint16_t operand, uint16_t address)
{
    // Subtract operand from register
    reg[operand] -= memory[address];

    // Update status register
    if (reg[operand] == 0)
    {
        sr |= 0x0004;
    }
    else
    {
        sr &= 0xFFFB;
    }
    if (reg[operand] < 0)
    {
        sr |= 0x0002;
    }
    else
    {
        sr &= 0xFFFD;
    }
    if (reg[operand] > 0xFFFF)
    {
        sr |= 0x0001;
    }
    else
    {
        sr &= 0xFFFE;
    }
}