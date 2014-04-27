//
//  main.cpp
//  SimSnake
//
//  Created by Jeremy Bridon on 4/19/14.
//  Copyright (c) 2014 CoreS2. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>

#include "SimSnake.h"

#include <curses.h>

#define __NCursesBuild__
#ifdef __NCursesBuild__

// NCurses screen buffer
static WINDOW* ncScreenBuffer = NULL;

// Initialize board
void InitBoard( int boardSize )
{
    // Initialize screen, refresh it for writing
    initscr();
    
    noecho();
    crmode();
    cbreak();
    
    // Create with added padding for border and text below
    ncScreenBuffer = newwin( boardSize + 5, boardSize + 2, 0, 0 );
    wrefresh( ncScreenBuffer );
    refresh();
}

// Draw, and stall enough to make it visible to humans
void DrawBoard( const SimSnake& simSnake )
{
    wclear( ncScreenBuffer );
    
    wborder( ncScreenBuffer, '|', '|', '-', '-', '+', '+', '+', '+' );
    box( ncScreenBuffer, 0, 0 );
    
    const BoardSimulation& activeBoard = simSnake.GetActiveBoard();
    
    // Draw the border outeline
    int boardSize = activeBoard.GetBoardSize();
    
    for( int y = 0; y < boardSize; y++ )
    {
        for( int x = 0; x < boardSize; x++ )
        {
            BoardSimulation::BoardObject boardObject = activeBoard.GetBoard( x, y );
            if( boardObject == BoardSimulation::cBoardObject_Pellet )
            {
                mvwaddch(ncScreenBuffer, y + 1, x + 1, 'x');
            }
            else if( boardObject == BoardSimulation::cBoardObject_Snake )
            {
                mvwaddch(ncScreenBuffer, y + 1, x + 1, '#');
            }
        }
    }
    
    // Draw helper stat strings
    int mostMoveCount, mostPelletsCount;
    simSnake.GetStats( mostMoveCount, mostPelletsCount );
    
    // Draw a line to show the bottom edge of the map and before the text
    wmove( ncScreenBuffer, boardSize + 1, 1 );
    whline( ncScreenBuffer, '-', boardSize );
    
    mvwprintw( ncScreenBuffer, boardSize + 2, 2, "Gene #%d, Generation Count #%d", simSnake.GetActiveGeneIndex(), simSnake.GetGenerationCount() );
    mvwprintw( ncScreenBuffer, boardSize + 3, 2, "Most moves %d, most pellets %d", mostMoveCount, mostPelletsCount );
    
    wmove( ncScreenBuffer, 0, 0 );
    wrefresh( ncScreenBuffer );
    
    usleep(50000); // 0.05 second stall, that's 20 hz draw / update
}

// Does the file exist?
bool DoesFileExist( const char* fileName )
{
    bool doesExist = false;
    FILE* fHandle = fopen( fileName, "r" );
    if( fHandle != NULL )
    {
        doesExist = true;
        fclose( fHandle );
    }
    return doesExist;
}

// Converts all hand-crafted scripts to Gene0, Gene1, etc..
void ExportGenes( int genePoolCount )
{
    // List of "seeding" programs (in assembly-like syntax)
    const int cFileCount = 4;
    const char* cFileNames[ cFileCount ] =
    {
        "ScanFillSnake.txt",
        "GoRight.txt",
        "LeftRightCycle.txt",
        "EdgeWalk.txt",
    };
    
    for( int i = 0; i < genePoolCount; i++ )
    {
        const char* scriptFileName = cFileNames[ i % cFileCount ];
        char outFileName[ 512 ];
        sprintf( outFileName, "Gene%d", i );
        
        // Only write out if the file does not yet exist
        if( DoesFileExist( outFileName ) )
        {
            continue;
        }
        
        Gene gene;
        if( !LoadTxtGene( scriptFileName, gene ) )
        {
            printf( "Unable to load script \"%s\"!\n", scriptFileName );
            continue;
        }
        
        if( !WriteGene( outFileName, gene ) )
        {
            printf( "Unable to serialize script \"%s\"!\n", outFileName );
        }
    }
    
}

// Main application entry point
int main(int argc, const char * argv[])
{
    #ifdef __APPLE__
        // Hack: gives us enough time to launch the term app (*specific to OSX)
        // To debug, launc this xcode session, cd into the target dir, then ./SimSnake
        // Note that you should cange your console preferences so that the term starts big enough
        sleep( 5 );
    #endif // __APPLE__
    
    const int cBoardSize = 32;
    const int cGenePoolCount = 64;
    
    InitBoard( cBoardSize );
    
    // Seed the world, but only if the files do not yet exist
    ExportGenes( cGenePoolCount );
    
    // Begin a simple simulation
    SimSnake simSnake( cBoardSize, cGenePoolCount );
    
    while( true )
    {
        // Draw this out if you want...
        DrawBoard( simSnake );
        
        // Updates until a snake dies *or* moves a peg
        simSnake.Update();
    }
    
    return 0;
}

#endif // __NCursesBuild__

