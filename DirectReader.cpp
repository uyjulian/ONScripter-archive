//$Id:$ -*- C++ -*-
/*
 *  DirectReader.cpp - Reader from independent files
 *
 *  Copyright (c) 2001-2002 Ogapee
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "DirectReader.h"

#ifndef SEEK_END
#define SEEK_END 2
#endif

#define READ_LENGTH 4096

DirectReader::DirectReader()
{
}

DirectReader::~DirectReader()
{
}

int DirectReader::open()
{
    return 0;
}

int DirectReader::close()
{
    return 0;
}
    
int DirectReader::getNumFiles()
{
    return 0;
}
    
int DirectReader::getNumAccessed()
{
    return 0;
}
    
struct DirectReader::FileInfo DirectReader::getFileByIndex( int index )
{
    DirectReader::FileInfo fi;
    
    return fi;
}

bool DirectReader::getAccessFlag( char *file_name )
{
    return false;
}

size_t DirectReader::getFileLength( char *file_name )
{
    FILE *fp;
    size_t length = 0;

    if ( (fp = fopen( file_name, "rb" )) != NULL ){
        fseek( fp, 0, SEEK_END );
        length = ftell( fp );
        fclose( fp );
    }

    return length;
}

size_t DirectReader::getFile( char *file_name, unsigned char *buffer )
{
    FILE *fp;
    size_t length = 0;
    
    if ( (fp = fopen( file_name, "rb" )) != NULL ){
        while( (length = fread( buffer, 1, READ_LENGTH, fp )) ){
            buffer += length;
        }
        fclose( fp );
    }

    return length;
}
