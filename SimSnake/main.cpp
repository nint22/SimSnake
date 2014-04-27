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

//#define __ConsoleBuild__
#ifdef __ConsoleBuild__

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
    const int cBoardSize = 32;
    const int cGenePoolCount = 64;
    
    // Seed the world, but only if the files do not yet exist
    ExportGenes( cGenePoolCount );
    
    // Begin a simple simulation
    SimSnake simSnake( cBoardSize, cGenePoolCount );
    
    while( true )
    {
        // Print world to console
        int mostMoveCount, mostPelletsCount;
        simSnake.GetStats( mostMoveCount, mostPelletsCount );
        
        printf( "Gene #%d, Generation Count #%d\n", simSnake.GetActiveGeneIndex(), simSnake.GetGenerationCount() );
        printf( "Most snake moves: %d, most pellets eaten: %d\n", mostMoveCount, mostPelletsCount );
        
        // Updates until a snake dies *or* moves a peg
        simSnake.Update();
    }
    
    return 0;
}

#endif //__ConsoleBuild__

