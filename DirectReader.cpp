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

DirectReader::DirectReader( char *path )
{
    if ( path ){
        archive_path = new char[ strlen(path) + 1 ];
        memcpy( archive_path, path, strlen(path) + 1 );
    }
    else{
        archive_path = "";
    }
}

DirectReader::~DirectReader()
{
}

FILE *DirectReader::fopen(const char *path, const char *mode)
{
    char file_name[256];

    sprintf( file_name, "%s%s", archive_path, path );
    return ::fopen( file_name, mode );
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

bool DirectReader::getAccessFlag( const char *file_name )
{
    return false;
}

FILE *DirectReader::getFileHandle( const char *file_name, int &compression_type, size_t *length )
{
    FILE *fp;
    unsigned int i;
    size_t len;

    compression_type = NO_COMPRESSION;
    len = strlen( file_name );
    if ( len > MAX_FILE_NAME_LENGTH ) len = MAX_FILE_NAME_LENGTH;
    memcpy( capital_name, file_name, len );
    capital_name[ len ] = '\0';

    for ( i=0 ; i<len ; i++ ){
        if ( capital_name[i] == '/' || capital_name[i] == '\\' ) capital_name[i] = (char)DELIMITER;
    }

    if ( (fp = fopen( capital_name, "rb" )) != NULL && len >= 3 ){
        if ( (!strncmp( &capital_name[len-3], "NBZ", 3 ) || !strncmp( &capital_name[len-3], "nbz", 3 )) ){
            *length = readLong( fp );
            if ( readChar( fp ) == 'B' && readChar( fp ) == 'Z' ) compression_type = NBZ_COMPRESSION;
            fseek( fp, 0, SEEK_SET );
        }
        else if ( (!strncmp( &capital_name[len-3], "SPB", 3 ) || !strncmp( &capital_name[len-3], "spb", 3 )) ){
            char str[30];
            int width  = readShort( fp );
            int height = readShort( fp );
            sprintf( str, "P6 %d %d 255\n", width , height );
            *length = width * height * 3 + strlen( str );
            compression_type = SPB_COMPRESSION;
            fseek( fp, 0, SEEK_SET );
        }
    }
    
    return fp;
}

size_t DirectReader::getFileLength( const char *file_name )
{
    int compression_type;
    size_t len;
    FILE *fp = getFileHandle( file_name, compression_type, &len );

    if ( fp ){
        if ( compression_type == NO_COMPRESSION ){
            fseek( fp, 0, SEEK_END );
            len = ftell( fp );
        }
        fclose( fp );
    }
    else
        len = 0;
    
    return len;
}

size_t DirectReader::getFile( const char *file_name, unsigned char *buffer )
{
    int compression_type;
    size_t len, c, total = 0;
    FILE *fp = getFileHandle( file_name, compression_type, &len );
    
    if ( fp ){
        if      ( compression_type & NBZ_COMPRESSION ) return decodeNBZ( fp, 0, buffer );
        else if ( compression_type & SPB_COMPRESSION ) return decodeSPB( fp, 0, buffer );

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

	while( err == BZ_OK && count > 0 ){
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

int DirectReader::getbit( FILE *fp, int n )
{
    int i, x = 0;
    static int getbit_buf;
    
    for ( i=0 ; i<n ; i++ ){
        if ( getbit_mask == 0 ){
            if ( (getbit_buf = fgetc( fp )) == EOF ) return EOF;
            getbit_mask = 128;
        }
        x <<= 1;
        if ( getbit_buf & getbit_mask ) x++;
        getbit_mask >>= 1;
    }
    return x;
}

size_t DirectReader::decodeSPB( FILE *fp, size_t offset, unsigned char *buf )
{
    unsigned int count;
    unsigned short width, height;
    unsigned char *pbuf, *psbuf;
    int i, j, k, c, n, m;
    char str[30];

    getbit_mask = 0;
    
    fseek( fp, offset, SEEK_SET );
    width  = readShort( fp );
    height = readShort( fp );
    sprintf( str, "P6 %d %d 255\n", width , height );

    memcpy( buf, str, strlen( str ) );
    buf += strlen( str );

    unsigned char *spb_buffer = new unsigned char[ width * height + 4 ];
    
    for ( i=0 ; i<3 ; i++ ){
        count = 0;
        spb_buffer[ count++ ] = c = getbit( fp, 8 );
        while ( count < (unsigned)(width * height) ){
            n = getbit( fp, 3 );
            if ( n == 0 ){
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                continue;
            }
            else if ( n == 7 ){
                m = getbit( fp, 1 ) + 1;
            }
            else{
                m = n + 2;
            }

            for ( j=0 ; j<4 ; j++ ){
                if ( m == 8 ){
                    c = getbit( fp, 8 );
                }
                else{
                    k = getbit( fp, m );
                    if ( k & 1 ) c += (k>>1) + 1;
                    else         c -= (k>>1);
                }
                spb_buffer[ count++ ] = c;
            }
        }

        pbuf  = buf + 2 - i;
        psbuf = spb_buffer;

        for ( j=0 ; j<height ; j++ ){
            if ( j & 1 ){
                for ( k=0 ; k<width ; k++, pbuf -= 3 ) *pbuf = *psbuf++;
                pbuf += (width+1) * 3;
            }
            else{
                for ( k=0 ; k<width ; k++, pbuf += 3 ) *pbuf = *psbuf++;
                pbuf += (width-1) * 3;
            }
        }
    }
    
    delete[] spb_buffer;
    
    return width * height * 3 + strlen( str );
}
