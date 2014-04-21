//
//  SimSnake.h
//  SimSnake
//
//  Created by Jeremy Bridon on 4/19/14.
//  Copyright (c) 2014 CoreS2. All rights reserved.
//
// Experiment goal: can a self-evolving program optimally
//  solve the Snake game?
//
// Rules:
//  n^2 board, does not wrap around; 32x32 by default
//  Snakes move in whichever direciton on this 2D board
//  Pellets grow the snake by one tail length, added at the end's location
//  World edges are deadly: snakes going out of bound die
//  Self-eating is deadly: snakes that touch themselves, and will die
//  Added to this: snakes that move backwards, will die
//  Starving snakes die: snakes cannot have more that 4 unneaten pellets
//  Snakes are machines whose genes are instructions and data
//
// Simulation: Sim of a snake's gene is implemented as the
//  execution of a Harvard Architecture machine: instructions
//  and memory are mixed, so code can be self-editing.
//  words are 32-bit, instructions have variable sizes,
//  working memory is 1 MB, and there is a 32*32+4+4+4 Byte read-only
//  input array at the top: this maps to the board state, mouse length,
//  mouse position. There are two working registers, that can be
//  loaded, operated on, and saved with.

// Notes:
//  Will the snakes evolve to optimally find and move towards the pellet?
//  Will it do a sort of "scan-line filling" to avoid self eating, but
//    only when needed?
//  Serialization is specific to endianness and assumes system int == 32-bit int

#ifndef __SIMSNAKE_H__
#define __SIMSNAKE_H__

#include <stdint.h>
#include <stdio.h>
#include <vector>

/*** Config Constants ***/

// Size of gene, in bytes; remember a gene is made of int32 (4 bytes)
// 1 MB, 262,144 instructions / memory
static const int cMemorySize = (1048576 / 4);

// How many movements can happen before eating which will kill the snake
static const int cMaxHunger = 20;

// Stalls after executing 9000 instructions with no movement=
static const int cStallCount = 10000;

/*** Common Structures ***/

// Instruction are multi-word
enum Instruction
{
    cInstruction_Nop = 0,   // Do nothing; just increas instruction ptr
    
    cInstruction_ZeroA,     // a = 0
    cInstruction_ZeroB,     // b = 0
    cInstruction_GetPos,    // a = x, b = y; xy are snake head pos
    cInstruction_Board,     // a = board[b][a]
    cInstruction_BSize,     // a = board-size
    cInstruction_SetA,      // a = <literal>
    cInstruction_SetB,      // b = <literal>
    cInstruction_Swap,      // swap contents of a and b
    
    cInstruction_ReadA,     // a = mem[a]
    cInstruction_ReadB,     // b = mem[b]
    cInstruction_Write,     // mem[a] = b
    
    cInstruction_Add,       // a += b
    cInstruction_Sub,       // a -= b
    cInstruction_Mul,       // a *= b
    cInstruction_Div,       // a /= b
    cInstruction_Mod,       // a %= b
    
    cInstruction_Equal,     // a = a == b
    cInstruction_NE,        // a = a != b
    cInstruction_LT,        // a = a < b
    cInstruction_GT,        // a = a > b
    cInstruction_LTE,       // a = a <= b
    cInstruction_GTE,       // a = a >= b
    
    cInstruction_And,       // a = a && a; all these assume true is non-zero
    cInstruction_Or,        // a = a || b
    cInstruction_Not,       // a = !a
    
    cInstruction_IfJmp,     // If a != 0, set InstructionPtr += b
    cInstruction_Jmp,       // InstructionPtr += a
    cInstruction_SetJmp,    // InstructionPtr = a
    
    cInstruction_GoUp,      // Move snake head up
    cInstruction_GoDown,    // Down
    cInstruction_GoLeft,    // Left
    cInstruction_GoRight,   // Right
    
    // Must be last!
    cInstructionCount
};

static const char* InstructionNames[ cInstructionCount ] =
{
    "Nop",
    "ZeroA",
    "ZeroB",
    "GetPos",
    "Board",
    "BSize",
    "SetA",
    "SetB",
    "Swap",
    "ReadA",
    "ReadB",
    "Write",
    "Add",
    "Sub",
    "Mul",
    "Div",
    "Mod",
    "Equal",
    "NE",
    "LT",
    "GT",
    "LTE",
    "GTE",
    "And",
    "Or",
    "Not",
    "IfJmp",
    "Jmp",
    "SetJmp",
    "GoUp",
    "GoDown",
    "GoLeft",
    "GoRight"
};

// Possible gene or simulation errors
enum Error
{
    cError_None = 0,
    cError_DivByZero,
    cError_OutOfBounds,
    cError_OutOfBoard,
    cError_BoardFilled,
    cError_SelfEat,
    cError_Stalled,
    cError_Starved,
    
    cErrorCount
};

// English human readable
static const char* ErrorNames[ cErrorCount ] =
{
    "No Error",
    "Divided by Zero",
    "Out of Bounds Memory Access",
    "Moved Out of Board",
    "Board Filled by Snake",
    "Snake Ate Self",
    "Gene Too Slow",
    "Snake Starved to Death",
};


// Gene and size of each memory unit; 1MB
typedef std::vector< int32_t > Gene;

// Serialize to/from text file
bool WriteGene( const char* fileName, const Gene& gene );
bool LoadGene( const char* fileName, Gene& gene );

// Lodas the human-readable txt file; comments start with semi-colon,
// uses same instruction syntax
bool LoadTxtGene( const char* fileName, Gene& gene );

// Maps the given string to an instruction; case-sensitive! Returns true if found, else false
bool MapInstruction( const char* token, Instruction& instructionOut );

// Board position
struct BoardPosition
{
    BoardPosition() : x( 0 ), y( 0 ) { }
    BoardPosition( const BoardPosition& given ) { x = given.x; y = given.y; }
    BoardPosition( int8_t _x, int8_t _y ) { x = _x; y = _y; }
    int8_t x, y;
};

// A board game that simulates a gene
// Pellets are randomly placed
// Todo: different patterns
class BoardSimulation
{
public:
    
    enum BoardObject
    {
        cBoardObject_None,
        cBoardObject_Snake,
        cBoardObject_Pellet,
    };
    
    // Snake always starts at center
    BoardSimulation( int worldSize, const Gene& gene );
    ~BoardSimulation();
    
    // Get size
    int GetBoardSize() const { return m_boardSize; }
    
    // Get back the state of the board
    BoardObject GetBoard( int8_t x, int8_t y ) const;
    void SetBoard( int8_t x, int8_t y, const BoardObject& boardObj );
    
    // Executes one instruction, returns true on movement of snake
    // Any errors are given through "errorOut"
    bool UpdateSimulation( Error& errorOut );
    
    // Returns the array of snake positions; starts from head to tail
    const std::vector< BoardPosition >& GetSnake() const { return m_snake; }
    const std::vector< BoardPosition >& GetPellets() const { return m_pellets; }
    
protected:
    
    void AddPellet();
    
    // Snake wants to move in a given direction
    enum Move { cMove_Up, cMove_Down, cMove_Left, cMove_Right };
    Error MoveSnake( const Move& move );
    
private:
    
    // Memory maps
    int32_t* m_memory;
    BoardObject* m_boardObjects;
    int32_t m_instructionPtr;
    uint8_t m_boardSize;
    int m_registerA, m_registerB;
    
    // Did system halt, e.g. error?
    Error m_errorCode;
    
    // Active entities
    std::vector< BoardPosition > m_snake;
    std::vector< BoardPosition > m_pellets;
    
    // Number of snake movements
    uint32_t m_stepCount;
    
    // How many times the snake has moved since last eating
    uint32_t m_hungerCount;
};

/*** Simulation Controller ***/

// Todo
class SimSnake
{
public:
    
    SimSnake( int boardSize, int genePoolCount );
    ~SimSnake();
    
    // Give the entire simulation one step, which means the current
    // gene will move ahead or die; you can get the current board state
    void Update();
    
    const BoardSimulation& GetActiveBoard() const { return *m_activeBoard; }
    
    // Stats getters
    const int GetActiveGeneIndex() const { return m_activeGeneIndex; }
    const int GetGenerationCount() const { return m_generationCount; }
    
protected:
    
    // Core tweak / editable feature of this simulation
    void FitAndBreed();
    
private:
    
    // Active board; gets reset, etc.
    int m_boardSize;
    BoardSimulation* m_activeBoard;
    
    // Active gene, which goes ahead
    int m_activeGeneIndex;
    int m_stepCount;
    int m_generationCount;
    int m_genePoolSize;
    
};

#endif
