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
    unsigned int i;
    size_t len;

    len = strlen( file_name );
    if ( len > MAX_FILE_NAME_LENGTH ) len = MAX_FILE_NAME_LENGTH;
    memcpy( capital_name, file_name, len );
    capital_name[ len ] = '\0';

    for ( i=0 ; i<len ; i++ ){
        if ( capital_name[i] == '/' || capital_name[i] == '\\' ) capital_name[i] = (char)DELIMITER;
    }

    if ( (fp = fopen( capital_name, "rb" )) != NULL ){
        fseek( fp, 0, SEEK_END );
        len = ftell( fp );
        fclose( fp );
    }
    else
        len = 0;
    
    return len;
}

size_t DirectReader::getFile( char *file_name, unsigned char *buffer )
{
    FILE *fp;
    unsigned int i;
    int c;
    size_t total = 0, len;
    
    len = strlen( file_name );
    if ( len > MAX_FILE_NAME_LENGTH ) len = MAX_FILE_NAME_LENGTH;
    memcpy( capital_name, file_name, len );
    capital_name[ len ] = '\0';

    for ( i=0 ; i<len ; i++ ){
        if ( capital_name[i] == '/' || capital_name[i] == '\\' ) capital_name[i] = (char)DELIMITER;
    }

    if ( (fp = fopen( capital_name, "rb" )) != NULL ){
        fseek( fp, 0, SEEK_END );
        total = len = ftell( fp );
        fseek( fp, 0, SEEK_SET );
        while( len > 0 ){
            if ( len > READ_LENGTH ) c = READ_LENGTH;
            else                     c = len;
            len -= c;
            fread( buffer, 1, c, fp );
            buffer += c;
        }
        fclose( fp );
    }

    return total;
}
