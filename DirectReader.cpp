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
#include <bzlib.h>

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

unsigned char DirectReader::readChar( FILE *fp )
{
    unsigned char ret;
    
    fread( &ret, 1, 1, fp );
    return ret;
}

unsigned short DirectReader::readShort( FILE *fp )
{
    unsigned short ret;
    unsigned char buf[2];
    
    fread( &buf, 1, 2, fp );
    ret = buf[0] << 8 | buf[1];
    return ret;
}

unsigned long DirectReader::readLong( FILE *fp )
{
    unsigned long ret;
    unsigned char buf[4];
    
    fread( &buf, 1, 4, fp );
    ret = buf[0];
    ret = ret << 8 | buf[1];
    ret = ret << 8 | buf[2];
    ret = ret << 8 | buf[3];
    return ret;
}

int DirectReader::open( char *name )
{
    return 0;
}

int DirectReader::close()
{
    return 0;
}
    
char *DirectReader::getArchiveName() const
{
    return "direct";
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

FILE *DirectReader::getFileHandle( char *file_name, bool &nbz_flag, size_t *length )
{
    FILE *fp;
    unsigned int i;
    size_t len;

    nbz_flag = false;
    len = strlen( file_name );
    if ( len > MAX_FILE_NAME_LENGTH ) len = MAX_FILE_NAME_LENGTH;
    memcpy( capital_name, file_name, len );
    capital_name[ len ] = '\0';

    for ( i=0 ; i<len ; i++ ){
        if ( capital_name[i] == '/' || capital_name[i] == '\\' ) capital_name[i] = (char)DELIMITER;
    }

    if ( (fp = fopen( capital_name, "rb" )) != NULL ){
        if ( len >= 3 && (!strncmp( &capital_name[len-3], "NBZ", 3 ) || !strncmp( &capital_name[len-3], "nbz", 3 )) ){
            *length = readLong( fp );
            if ( readChar( fp ) == 'B' && readChar( fp ) == 'Z' ) nbz_flag = true;
            fseek( fp, 0, SEEK_SET );
        }
    }
    
    return fp;
}

size_t DirectReader::getFileLength( char *file_name )
{
    bool nbz_flag;
    size_t len;
    FILE *fp = getFileHandle( file_name, nbz_flag, &len );

    if ( fp ){
        if ( !nbz_flag ){
            fseek( fp, 0, SEEK_END );
            len = ftell( fp );
        }
        fclose( fp );
    }
    else
        len = 0;
    
    return len;
}

size_t DirectReader::getFile( char *file_name, unsigned char *buffer )
{
    bool nbz_flag;
    size_t len, c, total = 0;
    FILE *fp = getFileHandle( file_name, nbz_flag, &len );
    
    if ( fp ){
        if ( nbz_flag ) return decodeNBZ( fp, 0, buffer );

        total = len;
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

#define READ_LENGTH 4096
size_t DirectReader::decodeNBZ( FILE *fp, size_t offset, unsigned char *buf )
{
    unsigned int original_length, count;
	BZFILE *bfp;
	unsigned char *unused;
	int err, len, nunused;

    fseek( fp, offset, SEEK_SET );
    original_length = count = readLong( fp );

	bfp = BZ2_bzReadOpen( &err, fp, 0, 0, NULL, 0 );
	if ( bfp == NULL || err != BZ_OK ) return 0;

	while( err == BZ_OK ){
        if ( count >= READ_LENGTH )
            len = BZ2_bzRead( &err, bfp, buf, READ_LENGTH );
        else
            len = BZ2_bzRead( &err, bfp, buf, count );
        count -= len;
		buf += len;
	}

	BZ2_bzReadGetUnused(&err, bfp, (void **)&unused, &nunused );
	BZ2_bzReadClose( &err, bfp );

    return original_length - count;
}
