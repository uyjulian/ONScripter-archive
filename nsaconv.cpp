/* -*- C++ -*-
 * 
 *  nsaconv.cpp - Images in NSA archive are re-scaled to 320x240 size
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "NsaReader.h"

extern int errno;
extern float scale_ratio;

extern size_t rescaleJPEG( unsigned char *original_buffer, size_t length, unsigned char **rescaled_buffer );
extern size_t rescaleBMP( unsigned char *original_buffer, size_t length, unsigned char **rescaled_buffer );

int main( int argc, char **argv )
{
    NsaReader cSR;
    unsigned long length, offset = 0, buffer_length = 0;
    unsigned char *buffer = NULL, *rescaled_buffer = NULL;
    unsigned int i, count;
    FILE *fp;

    if ( argc == 4 ){
        int s = atoi( argv[1] );
        if      ( s == 640 ) scale_ratio = 2.0;
        else if ( s == 800 ) scale_ratio = 2.5;
        else argc = 1;
    }
    if ( argc != 4 ){
        fprintf( stderr, "Usage: sardec 640 arc_file rescaled_arc_file\n");
        fprintf( stderr, "Usage: sardec 800 arc_file rescaled_arc_file\n");
        exit(-1);
    }

    if ( (fp = fopen( argv[3], "wb" ) ) == NULL ){
        fprintf( stderr, "can't open file %s for writing.\n", argv[3] );
        exit(-1);
    }
    cSR.openForConvert( argv[2] );
    count = cSR.getNumFiles();

    SarReader::FileInfo sFI;

    for ( i=0 ; i<count ; i++ ){
        sFI = cSR.getFileByIndex( i );
        printf( "%d/%d\n", i, count );
        if ( i==0 ) offset = sFI.offset;
        length = cSR.getFileLength( sFI.name );
        if ( length > buffer_length ){
            if ( buffer ) delete[] buffer;
            buffer = new unsigned char[length];
            buffer_length = length;
        }
        if ( cSR.getFile( sFI.name, buffer ) != length ){
            fprintf( stderr, "file %s can't be retrieved %ld\n", sFI.name, length );
            continue;
        }

        sFI.offset = offset;
        if ( (strlen( sFI.name ) > 3 && !strcmp( sFI.name + strlen( sFI.name ) - 3, "JPG") ||
              strlen( sFI.name ) > 4 && !strcmp( sFI.name + strlen( sFI.name ) - 4, "JPEG") ) ){
            sFI.length = rescaleJPEG( buffer, length, &rescaled_buffer );
            cSR.putFile( fp, i, sFI.offset, sFI.length, true, rescaled_buffer );
        }
        else if ( strlen( sFI.name ) > 3 && !strcmp( sFI.name + strlen( sFI.name ) - 3, "BMP") ){
            sFI.length = rescaleBMP( buffer, length, &rescaled_buffer );
            cSR.putFile( fp, i, sFI.offset, sFI.length, true, rescaled_buffer );
        }
        else{
            cSR.putFile( fp, i, sFI.offset, sFI.length, false, buffer );
        }
        
        offset += sFI.length;
    }
    cSR.writeHeader( fp );

    fclose(fp);

    if ( rescaled_buffer ) delete[] rescaled_buffer;
    if ( buffer ) delete[] buffer;
    
    exit(0);
}
