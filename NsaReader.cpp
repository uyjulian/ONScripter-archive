/* -*- C++ -*-
 *
 *  NsaReader.cpp - Reader from a NSA archive
 *
 *  Copyright (c) 2001-2002 Ogapee. All rights reserved.
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

#include "NsaReader.h"
#include <string.h>
#define NSA_ARCHIVE_NAME "arc.nsa"
#define NSA_ARCHIVE_NAME2 "arc%d.nsa"

#define EI 8
#define EJ 4
#define P   1  /* If match length <= P then output one character */
#define N (1 << EI)  /* buffer size */
#define F ((1 << EJ) + P)  /* lookahead buffer size */

NsaReader::NsaReader()
{
    int i;
    for ( i=0 ; i<MAX_EXTRA_ARCHIVE ; i++ ){
        archive_info2[i].file_handle = NULL;
        archive_info2[i].fi_list = NULL;
        archive_info2[i].num_of_files = 0;
        archive_info2[i].num_of_accessed = 0;
    }
    sar_flag = true;
    num_of_archives = 0;
}

NsaReader::~NsaReader()
{
}

int NsaReader::open()
{
    int i;
    char archive_name[256];

    if ( !SarReader::open() ) return 0;

    sar_flag = false;
    if ( ( archive_info.file_handle = fopen( NSA_ARCHIVE_NAME, "rb" ) ) == NULL ){
        fprintf( stderr, "can't open file %s\n", NSA_ARCHIVE_NAME );
        return -1;
    }

    readArchive( &archive_info, true );

    for ( i=0 ; i<MAX_EXTRA_ARCHIVE ; i++ ){
        sprintf( archive_name, NSA_ARCHIVE_NAME2, i+1 );
        if ( ( archive_info2[i].file_handle = fopen( archive_name, "rb" ) ) == NULL ){
            //fprintf( stderr, "can't open file %s\n", archive_name );
            return 0;
        }
        num_of_archives = i+1;
        readArchive( &archive_info2[i], true );
    }

    return 0;
}

int NsaReader::getNumFiles(){
    int total = archive_info.num_of_files, i;

    for ( i=0 ; i<num_of_archives ; i++ ) total += archive_info2[i].num_of_files;
    
    return total;
}

int NsaReader::getNumAccessed(){
    int total = archive_info.num_of_accessed, i;

    for ( i=0 ; i<num_of_archives ; i++ ) total += archive_info2[i].num_of_accessed;
    
    return total;
}

bool NsaReader::getAccessFlag( char *file_name )
{
    int i, j;

    j = getIndexFromFile( &archive_info, file_name );
    if ( archive_info.num_of_files != j ) return archive_info.fi_list[j].access_flag;

    for ( i=0 ; i<num_of_archives ; i++ ){
        j = getIndexFromFile( &archive_info2[i], file_name );
        if ( archive_info2[i].num_of_files != j ) return archive_info2[i].fi_list[j].access_flag;
    }

    return false;
}

size_t NsaReader::getFileLengthSub( ArchiveInfo *ai, char *file_name )
{
    char str[30];
    int width, height;

    int i = getIndexFromFile( ai, file_name );

    if ( i == ai->num_of_files ) return 0;

    if ( !ai->fi_list[i].access_flag ){
        ai->num_of_accessed++;
        ai->fi_list[i].access_flag = true;
    }
    if ( ai->fi_list[i].compressed_no != 1 ) return ai->fi_list[i].original_length;
    
    fseek( ai->file_handle, ai->fi_list[i].offset, SEEK_SET );
    width  = readShort( ai->file_handle );
    height = readShort( ai->file_handle );
    sprintf( str, "P6 %d %d 255\n", width , height );

    return width * height * 3 + strlen( str );
}

size_t NsaReader::getFileLength( char *file_name )
{
    if ( sar_flag ) return SarReader::getFileLength( file_name );

    size_t ret;
    int i;
    
    if ( ( ret = DirectReader::getFileLength( file_name ) ) ) return ret;
    
    if ( ( ret = getFileLengthSub( &archive_info, file_name )) ) return ret;

    for ( i=0 ; i<num_of_archives ; i++ ){
        if ( (ret = getFileLengthSub( &archive_info2[i], file_name )) ) return ret;
    }
    
    return 0;
}

size_t NsaReader::getFileSub( ArchiveInfo *ai, char *file_name, unsigned char *buffer )
{
    int i = getIndexFromFile( ai, file_name );
    if ( i == ai->num_of_files ) return 0;

    getbit_mask = 0;
    if ( ai->fi_list[i].compressed_no == 0 )      return SarReader::getFileSub( ai, file_name, buffer );
    else if ( ai->fi_list[i].compressed_no == 1 ) return decodeSPB( ai, i, buffer );
    else if ( ai->fi_list[i].compressed_no == 2 ) return decodeLZSS( ai, i, buffer );

    return 0;
}

size_t NsaReader::getFile( char *file_name, unsigned char *buffer )
{
    int i;
    size_t ret;

    if ( sar_flag ) return SarReader::getFile( file_name, buffer );

    if ( ( ret = DirectReader::getFile( file_name, buffer ) ) ) return ret;

    if ( (ret = getFileSub( &archive_info, file_name, buffer )) ) return ret;

    for ( i=0 ; i<num_of_archives ; i++ ){
        if ( (ret = getFileSub( &archive_info2[i], file_name, buffer )) ) return ret;
    }

    return 0;
}

struct NsaReader::FileInfo NsaReader::getFileByIndex( int index )
{
    int i;
    
    if ( index < archive_info.num_of_files ) return archive_info.fi_list[index];
    index -= archive_info.num_of_files;

    for ( i=0 ; i<num_of_archives ; i++ ){
        if ( index < archive_info2[i].num_of_files ) return archive_info2[i].fi_list[index];
        index -= archive_info2[i].num_of_files;
    }
    assert( false );

    return archive_info.fi_list[0];
}

int NsaReader::getbit( FILE *fp, int n )
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

size_t NsaReader::decodeSPB( struct ArchiveInfo *ai, int no, unsigned char *buf )
{
    unsigned int count;
    unsigned short width, height;
    unsigned char *pbuf, *psbuf;
    int i, j, k, c, n, m;
    char str[30];

    fseek( ai->file_handle, ai->fi_list[no].offset, SEEK_SET );
    width  = readShort( ai->file_handle );
    height = readShort( ai->file_handle );
    sprintf( str, "P6 %d %d 255\n", width , height );

    memcpy( buf, str, strlen( str ) );
    buf += strlen( str );

    unsigned char *spb_buffer = new unsigned char[ width * height ];
    
    for ( i=0 ; i<3 ; i++ ){
        count = 0;
        spb_buffer[ count++ ] = c = getbit( ai->file_handle, 8 );
        while ( count < (unsigned)(width * height) ){
            n = getbit( ai->file_handle, 3 );
            if ( n == 0 ){
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                spb_buffer[ count++ ] = c;
                continue;
            }
            else if ( n == 7 ){
                m = getbit( ai->file_handle, 1 ) + 1;
            }
            else{
                m = n + 2;
            }

            for ( j=0 ; j<4 ; j++ ){
                if ( m == 8 ){
                    c = getbit( ai->file_handle, 8 );
                }
                else{
                    k = getbit( ai->file_handle, m );
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

size_t NsaReader::decodeLZSS( struct ArchiveInfo *ai, int no, unsigned char *buf )
{
    unsigned int count = 0;
    int i, j, k, r, c;
    unsigned char *lzss_buffer = new unsigned char[ N * 2 ];

    fseek( ai->file_handle, ai->fi_list[no].offset, SEEK_SET );
    memset( lzss_buffer, 0, N-F );
    r = N - F;

    while ( count < ai->fi_list[no].original_length ){
        if ( getbit( ai->file_handle, 1 ) ) {
            if ((c = getbit( ai->file_handle, 8 )) == EOF) break;
            buf[ count++ ] = c;
            lzss_buffer[ r++ ] = c;  r &= (N - 1);
        } else {
            if ((i = getbit( ai->file_handle, EI )) == EOF) break;
            if ((j = getbit( ai->file_handle, EJ )) == EOF) break;
            for (k = 0; k <= j + 1  ; k++) {
                c = lzss_buffer[(i + k) & (N - 1)];
                buf[ count++ ] = c;
                lzss_buffer[ r++ ] = c;  r &= (N - 1);
            }
        }
    }

    delete[] lzss_buffer;
    return count;
}
