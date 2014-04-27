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
#include <string>
#include <map>

/*** Helper Functions ***/

namespace
{
    void LowerStdString( std::string& givenString )
    {
        const int strLen = int( givenString.length() );
        for( int i = 0; i < strLen; i++ )
        {
            givenString.at( i ) = tolower( givenString.at( i ) );
        }
    }
    
    bool IsStdStringAlphaNum( const std::string& givenString )
    {
        const int strLen = int( givenString.length() );
        for( int i = 0; i < strLen; i++ )
        {
            if( !isalnum( givenString.at( i ) ) )
            {
                return false;
            }
        }
        
        return true;
    }
    
    bool GeneFitnessSortFunc( const SimSnake::GeneFitnessPair& a, const SimSnake::GeneFitnessPair& b )
    {
        return a.m_fitnessValue < b.m_fitnessValue;
    }
    
    void GetGeneName( int geneIndex, char* geneNameOut )
    {
        sprintf( geneNameOut, "Gene%d", geneIndex );
    }
}



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
        
        // Fill rest with random numbers
        for( size_t i = instructionCount; i < (size_t)std::max( (int)instructionCount,  cMemorySize ); i++ )
        {
            int random = rand() % INT_MAX;
            fwrite( (void*)&(random), sizeof( Instruction ), 1, file );
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
    
    // Map of labels (lower-cased keys) to integer address positions
    std::map< std::string, int > labelAddresses;
    std::map< std::string, int >::iterator labelAddressIterator;
    
    // Map of label instructions that need to be resolved; address location -> string to find
    std::map< int, std::string > labelResolutions;
    std::map< int, std::string >::iterator labelResolutionsIterator;
    
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
                
                bool isHandled = false;
                
                // This is a token, convert as instruction or number
                if( isalpha( token[0] ) && MapInstruction( token, instruction ) )
                {
                    gene.push_back( int32_t(instruction) );
                    tokenIndex = 0;
                    
                    isHandled = true;
                }
                // Else, 32-bit signed integer
                else if( sscanf( token, "%d", &value ) == 1 )
                {
                    gene.push_back( int32_t(value) );
                    tokenIndex = 0;
                    
                    isHandled = true;
                }
                // Else, could be a label definition
                else if( token[ tokenIndex - 1 ] == ':' )
                {
                    std::string tokenString = token;
                    tokenString.erase( tokenString.end() - 1 );
                    
                    LowerStdString( tokenString );
                    
                    // Make sure it doesn't already exist
                    labelAddressIterator = labelAddresses.find( tokenString );
                    if( labelAddressIterator == labelAddresses.end() )
                    {
                        labelAddresses[ tokenString ] = int( gene.size() );
                        isHandled = true;
                    }
                    else
                    {
                        printf( "Error: Redefinition of label \"%s\"\n", token );
                    }
                }
                // Else, was the previous instruction a jump, and this is the target?
                else if( gene.size() > 0 && ( gene.back() == cInstruction_IfJmp || gene.back() == cInstruction_Jmp ) )
                {
                    std::string tokenString = token;
                    LowerStdString( tokenString );
                    
                    // Make sure this is a reasonable token
                    if( IsStdStringAlphaNum( tokenString ) )
                    {
                        labelResolutions[ int( gene.size() ) ] = tokenString;
                        gene.push_back( 0 );
                        isHandled = true;
                    }
                    else
                    {
                        printf( "Error: Label \"%s\" is not a valid label name\n", token );
                    }
                }
                
                // Report not being able to read token
                if( !isHandled )
                {
                    // Parse error; log and continue
                    printf( "Error: Bad token read in \"%s\", token: \"%s\" (Missing Jmp infront of label name?)\n", fileName, token );
                }
                
                token[ 0 ] = '\0';
                tokenIndex = 0;
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
        
        // Map all label jumps to relative addresses
        bool labelsLoaded = true;
        for( labelResolutionsIterator = labelResolutions.begin(); labelResolutionsIterator != labelResolutions.end() && labelsLoaded; ++labelResolutionsIterator )
        {
            // Was this label ever defined?
            labelAddressIterator = labelAddresses.find( labelResolutionsIterator->second );
            if( labelAddressIterator == labelAddresses.end() )
            {
                printf( "Error: Was not able to find the jump label name \"%s\"\n", labelResolutionsIterator->second.c_str() );
                labelsLoaded = false;
            }
            else
            {
                // Change the jump argument to a relative offsent
                gene.at( labelResolutionsIterator->first ) = labelAddressIterator->second - labelResolutionsIterator->first + 1;
            }
        }
        
        // All done, though return goto-label matching errors
        fclose( file );
        return labelsLoaded;
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
    , m_instructionCount( 0 )
    , m_movementCount( 0 )
    , m_pelletCount( 0 )
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

BoardSimulation::BoardObject BoardSimulation::GetBoard( int x, int y ) const
{
    return m_boardObjects[y * m_boardSize + x];
}

void BoardSimulation::SetBoard( int x, int y, const BoardSimulation::BoardObject& boardObj )
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
    
    m_instructionCount++;
    
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
            
        case cInstruction_GetPos:
        {
            m_registerA = m_snake.front().x;
            m_registerB = m_snake.front().y;
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
                m_memory[ m_registerA ] = m_registerB;
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
                m_instructionPtr += arg0;
                jumped = true;
            }
            else
            {
                // Read pass the argument
                m_instructionPtr++;
            }
            break;
        }
        
        case cInstruction_Jmp:
        {
            m_instructionPtr += arg0;
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
    if( m_instructionPtr < 0 || m_instructionPtr >= cMemorySize )
    {
        errorOut = cError_OutOfBounds;
    }
    else if( m_snake.size() >= m_boardSize * m_boardSize )
    {
        errorOut = cError_BoardFilled;
    }
    
    // Save error state for future lookup
    m_errorCode = errorOut;
    
    return moved;
}

int BoardSimulation::GetFitness() const
{
    // What's best: low instructions, low movement, high pellet
    return ( m_instructionCount / 1000 + m_movementCount ) - m_pelletCount * 100;
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
                m_pelletCount++;
                m_pellets.erase( m_pellets.begin() + i );
                break;
            }
        }
        
        // Add a new pellet
        AddPellet();
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
    m_movementCount++;
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
    , m_maxMovementCount( 0 )
    , m_maxPelletEattenCount( 0 )
{
    // Initialize all gene ranks to -1 (not yet measured)
    for( int i = 0; i < m_genePoolSize; i++ )
    {
        m_geneFitness.push_back( GeneFitnessPair( i, INT_MAX ) );
    }
    
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
        Error errorOut = cError_None;
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
            
            // Save performance
            m_geneFitness.at( m_activeGeneIndex ) = GeneFitnessPair( m_activeGeneIndex, m_activeBoard->GetFitness() );
            m_maxMovementCount = std::max( m_maxMovementCount, m_activeBoard->GetMovementCount() );
            m_maxPelletEattenCount = std::max( m_maxPelletEattenCount, m_activeBoard->GetPelletCount() );
            
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
            GetGeneName( m_activeGeneIndex, fileName );
            
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

void SimSnake::GetStats( int& longestLivedMovementCount, int& mostPelletsEatenCount ) const
{
    longestLivedMovementCount = m_maxMovementCount;
    mostPelletsEatenCount = m_maxPelletEattenCount;
}

void SimSnake::FitAndBreed()
{
    // Sort gene scores, lower is best; dead genes are ranked with int_max
    std::sort( m_geneFitness.begin(), m_geneFitness.end(), GeneFitnessSortFunc );
    const int cHalfPoolSize = m_genePoolSize / 2;
    
    // Copy the top best genes into their new file names
    std::vector< Gene > bestGenes;
    for( int i = 0; i < cHalfPoolSize; i++ )
    {
        Gene gene;
        int geneIndex = m_geneFitness.at( i ).m_geneIndex;
        
        char fileName[ 512 ];
        GetGeneName( geneIndex, fileName );
        
        if( !LoadGene( fileName, gene ) )
        {
            printf( "Error: Unable to load the gene at index %d for re-sorting\n", geneIndex );
        }
        
        bestGenes.push_back( gene );
    }
    
    // Write out this list, nuking the original set
    for( int i = 0; i < cHalfPoolSize; i++ )
    {
        char fileName[ 512 ];
        GetGeneName( i, fileName );
        WriteGene( fileName, bestGenes.at( i ) );
    }
    
    // Top 50% replicate with the next ranked gene, replacing bottom 50%
    for( int i = 0; i < cHalfPoolSize; i += 2 )
    {
        int geneIndex = m_geneFitness.at( i ).m_geneIndex;
        
        // Self-breeding results in mutation
        int geneIndexA = geneIndex;
        int geneIndexB = (geneIndex + 1) % m_genePoolSize;
        
        Breed( geneIndexA, geneIndexB, cHalfPoolSize + geneIndex );
        Breed( geneIndexB, geneIndexA, cHalfPoolSize + geneIndex + 1 );
    }
    
    printf( "Breeding and generatng a population\n" );
    
    // Reset array
    for( int i = 0; i < m_genePoolSize; i++ )
    {
        m_geneFitness.at( i ) = GeneFitnessPair( i, 0 );
    }
}

void SimSnake::Breed( int geneIndexA, int geneIndexB, int geneReplacementIndex )
{
    char fileName[ 512 ];
    
    // Load both genes; remember that the A gene will be dominant here
    GetGeneName( geneIndexA, fileName );
    Gene geneA;
    LoadGene( fileName, geneA );
    
    GetGeneName( geneIndexB, fileName );
    Gene geneB;
    LoadGene( fileName, geneB );
    
    if( geneA.empty() || geneB.empty() )
    {
        printf( "Internal error: gene length inconsistency!\n" );
        return;
    }
    
    Gene childGene = geneB;
    
    // We cut up based on this division:
    const int cSegmentCount = 128;
    const int cSelectionLength = cMemorySize / cSegmentCount;
    
    // Swap up to three chunks at a time
    const int cChunkCount = 5;
    const int cChunkNum = (rand() % cChunkCount) + 1;
    
    // Shuffle genes from either self or given B gene
    for( int chunk = 0; chunk < cChunkNum * 2; chunk++ )
    {
        // 2x because 0 - cSegmentCount is gene A, cSegmentCount - cSegmentCount * 2 is gene B
        int sourceIndex = rand() % (cSegmentCount * 2);
        int destIndex = rand() % cSegmentCount;
        
        // Swap the chunk's instructions
        for( int i = 0; i < cSelectionLength; i++ )
        {
            Gene& srcGene = ( sourceIndex >= cSegmentCount ) ? geneA : geneB;
            childGene.at( destIndex * cSelectionLength + i ) = srcGene.at( (sourceIndex % cSegmentCount) * cSelectionLength + i );
        }
    }
    
    // Mutate 0.01% of data
    const int cMutationCount = int( float( cMemorySize ) * 0.0001f );
    for( int i = 0; i < cMutationCount; i++ )
    {
        childGene.at( rand() % cMemorySize ) = int32_t(rand() % INT32_MAX);
    }
    
    // Write out
    GetGeneName( geneReplacementIndex, fileName );
    WriteGene( fileName, childGene );
}
