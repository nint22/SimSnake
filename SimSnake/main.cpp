//
//  main.cpp
//  SimSnake
//
//  Created by Jeremy Bridon on 4/19/14.
//  Copyright (c) 2014 CoreS2. All rights reserved.
//

#include <stdio.h>
#include "SimSnake.h"

// Draw the given came on-screen
void DrawGame( const BoardSimulation& activeBoard )
{
    const int boardSize = activeBoard.GetBoardSize();
    for( int y = -1; y <= boardSize; y++ )
    {
        if( y == -1 || y == boardSize )
        {
            // Draw edge
            for( int x = -1; x <= boardSize; x++ )
            {
                printf( "-" );
            }
        }
        else
        {
            for( int x = -1; x <= boardSize; x++ )
            {
                // Draw edge
                if( x == -1 || x == boardSize )
                {
                    printf( "|" );
                }
                else
                {
                    BoardSimulation::BoardObject boardObject = activeBoard.GetBoard( x, y );
                    if( boardObject == BoardSimulation::cBoardObject_Pellet )
                    {
                        printf( "x" );
                    }
                    else if( boardObject == BoardSimulation::cBoardObject_Snake )
                    {
                        printf( "#" );
                    }
                    else
                    {
                        printf(" ");
                    }
                }
            }
        }
        
        printf( "\n" );
    }
}

// Converts all hand-crafted scripts to Gene0, Gene1, etc..
void ExportGenes()
{
    const int cFileCount = 4;
    const char* cFileNames[ cFileCount ] =
    {
        "ScanFillSnake.txt",
        "GoRight.txt",
        "LeftRightCycle.txt",
        "EdgeWalk.txt",
    };
    
    for( int i = 0; i < cFileCount; i++ )
    {
        const char* scriptFileName = cFileNames[ i ];
        char outFileName[ 512 ];
        sprintf( outFileName, "Gene%d", i );
        
        Gene gene;
        if( !LoadTxtGene( scriptFileName, gene ) )
        {
            printf( "Unable to load script \"%s\"!\n", scriptFileName );
        }
        else
        {
            if( !WriteGene( outFileName, gene ) )
            {
                printf( "Unable to serialize script \"%s\"!\n", outFileName );
            }
        }
    }
    
}

// Main application entry point
int main(int argc, const char * argv[])
{
    // Config!
    const int cBoardSize = 32;
    const int cGenePoolCount = 64;
    
    // Convert our assembly-like genes to a set of valid genes
    ExportGenes();
    
    // Begin a simple simulation
    SimSnake simSnake( cBoardSize, cGenePoolCount );
    
    while( true )
    {
        // Print world to console
        printf( "--------------------------------------\n" );
        printf( "  Gene #%d, Generation Count #%d\n", simSnake.GetActiveGeneIndex(), simSnake.GetGenerationCount() );
        printf( "--------------------------------------\n" );
        DrawGame( simSnake.GetActiveBoard() );
        
        // Updates until a snake dies *or* moves a peg
        simSnake.Update();
    }
    
    return 0;
}

