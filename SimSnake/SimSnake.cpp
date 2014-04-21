//
//  SimSnake.h
//  SimSnake
//
//  Created by Jeremy Bridon on 4/19/14.
//  Copyright (c) 2014 CoreS2. All rights reserved.
//

#include "SimSnake.h"

#include <stdlib.h>
#include <limits.h>
#include <ctype.h>


/*** Helper Functions ***/


// Serialize to text file
bool WriteGene( const char* fileName, const Gene& gene )
{
    FILE* file = NULL;
    if( (file = fopen( fileName, "wb" )) != NULL )
    {
        const size_t instructionCount = gene.size();
        for( int i = 0; i < instructionCount; i++ )
        {
            fwrite( (void*)&gene[ i ], sizeof( Instruction ), 1, file );
        }
        
        fclose( file );
        return true;
    }
    return false;
}

bool LoadGene( const char* fileName, Gene& gene )
{
    FILE* file = NULL;
    if( (file = fopen( fileName, "r" )) != NULL )
    {
        bool success = false;
        Instruction instruction;
        
        // Keep moving at length of instructions; warning, this makes
        // this code saved files not portal when different systems have different word-sizes
        for( int i = 0; i < INT_MAX; i++ )
        {
            size_t read = fread( (void*)&instruction, sizeof( Instruction ), 1, file );
            
            // End of file
            if( read == 0 )
            {
                success = true;
                break;
            }
            
            // Read more or less than expected, bad
            else if( read != 1 )
            {
                success = false;
                break;
            }
            
            // Else push instruction
            else
            {
                gene.push_back( instruction );
            }
        }
        
        fclose( file );
        return success;
    }
    return false;
}

bool LoadTxtGene( const char* fileName, Gene& gene )
{
    const int cTokenLength = 512;
    int tokenIndex = 0;
    char token[ cTokenLength ] = "\0";
    
    FILE* file = NULL;
    if( (file = fopen( fileName, "r" )) != NULL )
    {
        bool isComment = false;
        
        // For each token...
        while( true )
        {
            // Keep reading if whitespace
            int nextChar = fgetc( file );
            if( feof( file ) || nextChar <= 0 )
            {
                break;
            }
            // If we were in a comment, and hit end-of-line, reset comment state
            else if( isComment )
            {
                if( nextChar == '\n' || nextChar == '\r' )
                {
                    isComment = false;
                }
            }
            // Comments start with ';'
            else if( nextChar == ';' )
            {
                isComment = true;
            }
            // Either end of token or spacing before
            else if( tokenIndex > 0 && isspace( nextChar ) )
            {
                int value = 0;
                Instruction instruction;
                
                // This is a token, convert as instruction or number
                if( isalpha( token[0] ) && MapInstruction( token, instruction ) )
                {
                    gene.push_back( int32_t(instruction) );
                    tokenIndex = 0;
                }
                else if( sscanf( token, "%d", &value ) == 1 )
                {
                    gene.push_back( int32_t(value) );
                    tokenIndex = 0;
                }
                else
                {
                    // Parse error; log and continue
                    printf( "Warning: Bad token read in \"%s\", token: \"%s\"\n", fileName, token );
                    token[ 0 ] = '\0';
                    tokenIndex = 0;
                }
            }
            // If visible, start of token
            else if( isgraph( nextChar ) )
            {
                // Re-start token if too long!
                if( tokenIndex + 1 >= cTokenLength )
                {
                    token[ 0 ] = '\0';
                    tokenIndex = 0;
                }
                
                token[ tokenIndex ] = nextChar;
                token[ tokenIndex + 1 ] = '\0';
                tokenIndex++;
            }
            
            // Else, ignore char
        }
        
        // All done!
        fclose( file );
        return true;
    }
    return false;
}

bool MapInstruction( const char* token, Instruction& instructionOut )
{
    // Linear search
    for( int i = 0; i < cInstructionCount; i++ )
    {
        if( strcmp( token, InstructionNames[ i ] ) == 0 )
        {
            instructionOut = (Instruction)i;
            return true;
        }
    }
    
    return false;
}

/*** Board Simulation ***/


BoardSimulation::BoardSimulation( int worldSize, const Gene& gene )
    : m_memory( NULL )
    , m_boardObjects( NULL )
    , m_instructionPtr( 0 )
    , m_boardSize( worldSize )
    , m_registerA( 0 )
    , m_registerB( 0 )
    , m_errorCode( cError_None )
    , m_stepCount( 0 )
    , m_hungerCount( 0 )
{
    // Set all to zero, copy in gene
    m_memory = new int32_t[ cMemorySize ];
    memset( (void*)m_memory, 0, sizeof( int32_t ) * cMemorySize );
    const int instructionCount = (int)gene.size();
    for( int i = 0; i < instructionCount; i++ )
    {
        m_memory[ i ] = gene[ i ];
    }
    
    // Default board to nothing
    m_boardObjects = new BoardObject[ m_boardSize * m_boardSize ];
    memset( (void*)m_boardObjects, 0, sizeof( BoardObject ) * m_boardSize * m_boardSize );
    
    // Start at center
    BoardPosition pos( m_boardSize / 2, m_boardSize / 2 );
    m_snake.push_back( pos );
    SetBoard( pos.x, pos.y, cBoardObject_Snake );
    
    // Add a pellet
    AddPellet();
}

BoardSimulation::~BoardSimulation()
{
    delete[] m_memory;
    delete[] m_boardObjects;
}

BoardSimulation::BoardObject BoardSimulation::GetBoard( int8_t x, int8_t y ) const
{
    return m_boardObjects[y * m_boardSize + x];
}

void BoardSimulation::SetBoard( int8_t x, int8_t y, const BoardSimulation::BoardObject& boardObj )
{
    m_boardObjects[y * m_boardSize + x] = boardObj;
}

bool BoardSimulation::UpdateSimulation( Error& errorOut )
{
    if( m_errorCode != cError_None )
    {
        errorOut = m_errorCode;
        return false;
    }
    else
    {
        errorOut = cError_None;
    }
    
    // Grab instruction
    Instruction op = (Instruction)m_memory[ m_instructionPtr ];
    int arg0 = ( m_instructionPtr + 1 < cMemorySize) ? m_memory[ m_instructionPtr + 1 ] : 0;
    int arg1 = ( m_instructionPtr + 2 < cMemorySize) ? m_memory[ m_instructionPtr + 2 ] : 0;
    
    // Flags
    bool jumped = false;
    bool moved = false;
    
    // Execute
    switch( op )
    {
        case cInstruction_ZeroA:
        {
            m_registerA = 0;
            break;
        }
            
        case cInstruction_ZeroB:
        {
            m_registerB = 0;
            break;
        }
            
        case cInstruction_Board:
        {
            m_registerA = GetBoard( arg0, arg1 );
            m_instructionPtr++;
            m_instructionPtr++;
            break;
        }
            
        case cInstruction_BSize:
        {
            m_registerA = m_boardSize;
            break;
        }
            
        case cInstruction_SetA:
        {
            m_registerA = arg0;
            m_instructionPtr++;
            break;
        }
            
        case cInstruction_SetB:
        {
            m_registerB = arg0;
            m_instructionPtr++;
            break;
        }
            
        case cInstruction_Swap:
        {
            int temp = m_registerA;
            m_registerA = m_registerB;
            m_registerB = temp;
            break;
        }
            
        case cInstruction_ReadA:
        {
            if( m_registerA >= 0 && m_registerA < cMemorySize )
            {
                m_registerA =  m_memory[ m_registerA ];
            }
            else
            {
                m_errorCode = cError_OutOfBounds;
            }
            break;
        }
            
        case cInstruction_ReadB:
        {
            if( m_registerB >= 0 && m_registerB < cMemorySize )
            {
                m_registerB =  m_memory[ m_registerB ];
            }
            else
            {
                m_errorCode = cError_OutOfBounds;
            }
            break;
        }
            
        case cInstruction_Write:
        {
            if( m_registerA >= 0 && m_registerA < cMemorySize )
            {
                m_memory[ m_registerA ] = m_registerA;
            }
            else
            {
                m_errorCode = cError_OutOfBounds;
            }
            break;
        }
            
        case cInstruction_Add:
        {
            m_registerA += m_registerB;
            break;
        }
            
        case cInstruction_Sub:
        {
            m_registerA -= m_registerB;
            break;
        }
            
        case cInstruction_Mul:
        {
            m_registerA *= m_registerB;
            break;
        }
            
        case cInstruction_Div:
        {
            if( m_registerB != 0 )
            {
                m_registerA /= m_registerB;
            }
            else
            {
                errorOut = cError_DivByZero;
            }
            break;
        }
            
        case cInstruction_Mod:
        {
            m_registerA %= m_registerB;
            break;
        }
            
        case cInstruction_Equal:
        {
            m_registerA = ( m_registerA == m_registerB );
            break;
        }
            
        case cInstruction_NE:
        {
            m_registerA = ( m_registerA != m_registerB );
            break;
        }
            
        case cInstruction_LT:
        {
            m_registerA = ( m_registerA < m_registerB );
            break;
        }
            
        case cInstruction_GT:
        {
            m_registerA = ( m_registerA > m_registerB );
            break;
        }
            
        case cInstruction_LTE:
        {
            m_registerA = ( m_registerA <= m_registerB );
            break;
        }
            
        case cInstruction_GTE:
        {
            m_registerA = ( m_registerA >= m_registerB );
            break;
        }
            
        case cInstruction_And:
        {
            m_registerA = ( (m_registerA != 0) && (m_registerB != 0) );
            break;
        }
            
        case cInstruction_Or:
        {
            m_registerA = ( (m_registerA != 0) || (m_registerB != 0) );
            break;
        }
            
        case cInstruction_Not:
        {
            m_registerA = ( m_registerA == 0 );
            break;
        }
            
        case cInstruction_IfJmp:
        {
            if( m_registerA != 0 )
            {
                m_instructionPtr += arg1;
                jumped = true;
            }
            break;
        }
        
        case cInstruction_Jmp:
        {
            m_instructionPtr += arg0;
            jumped = true;
            break;
        }
            
        case cInstruction_SetJmp:
        {
            m_instructionPtr = arg0;
            jumped = true;
            break;
        }
            
        case cInstruction_GoUp:
        {
            errorOut = MoveSnake( cMove_Up );
            moved = true;
            break;
        }
            
        case cInstruction_GoDown:
        {
            errorOut = MoveSnake( cMove_Down );
            moved = true;
            break;
        }
            
        case cInstruction_GoLeft:
        {
            errorOut = MoveSnake( cMove_Left );
            moved = true;
            break;
        }
            
        case cInstruction_GoRight:
        {
            errorOut = MoveSnake( cMove_Right );
            moved = true;
            break;
        }
        
        // Default, do nothing
        case cInstruction_Nop: default:
        {
            break;
        }
    }
    
    // Only increase pointer if we didnt jump
    if( !jumped )
    {
        m_instructionPtr++;
    }
    
    // Check other error conditions
    if( m_errorCode != cError_None )
    {
        if( m_instructionPtr < 0 || m_instructionPtr >= cMemorySize )
        {
            errorOut = cError_OutOfBounds;
            return false;
        }
        else if( m_snake.size() >= m_boardSize * m_boardSize )
        {
            errorOut = cError_BoardFilled;
            return false;
        }
    }
    
    return moved;
}

void BoardSimulation::AddPellet()
{
    // Keep randomizing until we get no collision
    // Note: board must have empty spaced!
    bool placed = false;
    while( !placed )
    {
        BoardPosition pos = BoardPosition( rand() % m_boardSize, rand() % m_boardSize );
        if( GetBoard( pos.x, pos.y ) == cBoardObject_None )
        {
            m_pellets.push_back( pos );
            SetBoard( pos.x, pos.y, cBoardObject_Pellet );
            placed = true;
        }
    }
}

Error BoardSimulation::MoveSnake( const Move& move )
{
    // Compute head position
    BoardPosition head = m_snake.front();
    if( move == cMove_Up )
        head.y--;
    else if( move == cMove_Down )
        head.y++;
    else if( move == cMove_Left )
        head.x--;
    else if( move == cMove_Right )
        head.x++;
    
    // Bounds check
    if( head.x < 0 || head.y < 0 || head.x >= m_boardSize || head.y >= m_boardSize )
    {
        return cError_OutOfBoard;
    }
    
    bool consumedPellete = false;
    BoardObject boardObject = GetBoard( head.x, head.y );
    
    // Self-hit test
    if( boardObject == cBoardObject_Snake )
    {
        return cError_SelfEat;
    }
    // If we hit a pellet, mark it
    else if( boardObject == cBoardObject_Pellet )
    {
        m_hungerCount = 0;
        consumedPellete = true;
        SetBoard( head.x, head.y, cBoardObject_None );
        
        // Remove this pellet from the list
        for( int i = 0; i < (int)m_pellets.size(); i++ )
        {
            if( m_pellets[ i ].x == head.x &&
                m_pellets[ i ].y == head.y )
            {
                m_pellets.erase( m_pellets.begin() + i );
                break;
            }
        }
    }
    
    // Moving ahead
    m_snake.insert( m_snake.begin(), head );
    SetBoard( head.x, head.y, cBoardObject_Snake );
    
    // Remove tail if we haven't consumed a pellet
    if( consumedPellete == false )
    {
        const BoardPosition& oldTail = m_snake.back();
        SetBoard( oldTail.x, oldTail.y, cBoardObject_None );
        m_snake.pop_back();
    }
    
    // Special rule: has the snake starved?
    m_hungerCount++;
    if( m_hungerCount >= cMaxHunger )
    {
        return cError_Starved;
    }
    
    // All done!
    return cError_None;
}

/*** Simulation Controller ***/

SimSnake::SimSnake( int boardSize, int genePoolCount )
    : m_activeBoard( NULL )
    , m_boardSize( boardSize )
    , m_activeGeneIndex( 0 )
    , m_stepCount( 0 )
    , m_generationCount( 0 )
    , m_genePoolSize( genePoolCount )
{
    // Load for first board game
    Gene firstGene;
    LoadGene( "Gene0", firstGene);
    
    m_activeBoard = new BoardSimulation( boardSize, firstGene );
}

SimSnake::~SimSnake()
{
    delete m_activeBoard;
}

void SimSnake::Update()
{
    // Keep repeating until we hit an error or we've moved
    while( true )
    {
        // Update board
        Error errorOut;
        bool hasMoved = m_activeBoard->UpdateSimulation( errorOut );
        
        // Stall check
        if( m_stepCount > cStallCount )
        {
            errorOut = cError_Stalled;
        }
        
        // Error check first
        if( errorOut != cError_None )
        {
            printf( "Gene has died: \"%s\"\n", ErrorNames[ (int)errorOut ] );
            
            // Update gene count; does a gene pool update if we're starting over the group
            m_activeGeneIndex = ( m_activeGeneIndex + 1 ) % m_genePoolSize;
            m_stepCount = 0;
            
            if( m_activeGeneIndex == 0 )
            {
                FitAndBreed();
                m_generationCount++;
            }
            
            // Load next gene
            char fileName[ 512 ];
            sprintf( fileName, "Gene%d", m_activeGeneIndex );
            
            // Stap to next gene
            Gene nextGene;
            LoadGene( fileName, nextGene );
            
            // Start new sim
            delete m_activeBoard;
            m_activeBoard = new BoardSimulation( m_boardSize, nextGene );
            
            break;
        }
        
        // Else, regular update
        else
        {
            if( hasMoved )
            {
                m_stepCount = 0;
                break;
            }
            else
            {
                m_stepCount++;
            }
        }
    }
}

void SimSnake::FitAndBreed()
{
    // Todo: sort based on gene scores
    
    // Todo: breed top X with eachother, replacing rest
    
    // Todo: Mutate X number (mutate should be based on )
}
