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
#if !defined(WIN32)
#include <dirent.h>
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#define READ_LENGTH 4096
#define WRITE_LENGTH 5000

#define EI 8
#define EJ 4
#define P   1  /* If match length <= P then output one character */
#define N (1 << EI)  /* buffer size */
#define F ((1 << EJ) + P)  /* lookahead buffer size */

DirectReader::DirectReader( char *path )
{
    if ( path ){
        archive_path = new char[ strlen(path) + 1 ];
        memcpy( archive_path, path, strlen(path) + 1 );
    }
    else{
        archive_path = "";
    }

    last_registered_compression_type = &root_registered_compression_type;
    registerCompressionType( "SPB", SPB_COMPRESSION );
    registerCompressionType( "JPG", NO_COMPRESSION );
    registerCompressionType( "GIF", NO_COMPRESSION );
}

DirectReader::~DirectReader()
{
    last_registered_compression_type = root_registered_compression_type.next;
    while ( last_registered_compression_type ){
        RegisteredCompressionType *cur = last_registered_compression_type;
        last_registered_compression_type = last_registered_compression_type->next;
        delete cur;
    }
}

FILE *DirectReader::fopen(const char *path, const char *mode)
{
    char *file_name = new char[ strlen(archive_path) + strlen(path) + 1 ];

    sprintf( file_name, "%s%s", archive_path, path );
    FILE *fp = ::fopen( file_name, mode );
    if ( fp ) return fp;
    
#if !defined(WIN32)
    char *p = strrchr( file_name, (char)DELIMITER );
    if ( !p ){
        DIR *dp = opendir( "." );
        if ( dp ){
            struct dirent *entp;
            while ( (entp = readdir( dp )) != NULL ){
                if ( !strcasecmp( file_name, entp->d_name ) ){
                    fp = ::fopen( entp->d_name, mode );
                    if ( fp ) break;
                }
            }
            closedir( dp );
        }
    }
    else {
        int dlen = p - file_name;
        char *fname = new char[ strlen(file_name) + 1 ];
        char *fbase = p + 1;
        strncpy( fname, file_name, dlen );
        fname[dlen++] = (char)DELIMITER;
        fname[dlen] = '\0';
        DIR *dp = opendir( fname );
        if ( dp ){
            struct dirent *entp;
            while ( (entp = readdir( dp )) != NULL ){
                if ( !strcasecmp( fbase, entp->d_name ) ){
                    strcat( fname, entp->d_name );
                    fp = ::fopen( fname, mode );
                    if ( fp ) break;
                    fname[dlen] = '\0';
                }
            }
            closedir( dp );
        }
        delete[] fname;
    }
#endif
    delete[] file_name;

    return fp;
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

void DirectReader::writeChar( FILE *fp, unsigned char ch )
{
    fwrite( &ch, 1, 1, fp );
}

void DirectReader::writeShort( FILE *fp, unsigned short ch )
{
    unsigned char buf[2];

    buf[0] = ((unsigned char*)&ch)[1];
    buf[1] = ((unsigned char*)&ch)[0];
    fwrite( &buf, 1, 2, fp );
}

void DirectReader::writeLong( FILE *fp, unsigned long ch )
{
    unsigned char buf[4];
    
    buf[0] = ((unsigned char*)&ch)[3];
    buf[1] = ((unsigned char*)&ch)[2];
    buf[2] = ((unsigned char*)&ch)[1];
    buf[3] = ((unsigned char*)&ch)[0];
    fwrite( &buf, 1, 4, fp );
}

int DirectReader::open( char *name, int archive_type )
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
    
void DirectReader::registerCompressionType( const char *ext, int type )
{
    last_registered_compression_type->next = new RegisteredCompressionType(ext, type);
    last_registered_compression_type = last_registered_compression_type->next;
}
    
int DirectReader::getRegisteredCompressionType( const char *file_name )
{
    const char *ext_buf = file_name + strlen(file_name);
    while( *ext_buf != '.' && ext_buf != file_name ) ext_buf--;
    ext_buf++;
    
    strcpy( capital_name, ext_buf );
    for ( unsigned int i=0 ; i<strlen(ext_buf)+1 ; i++ )
        if ( capital_name[i] >= 'a' && capital_name[i] <= 'z' )
            capital_name[i] += 'A' - 'a';
    
    RegisteredCompressionType *reg = root_registered_compression_type.next;
    while (reg){
        if ( !strcmp( capital_name, reg->ext ) ) return reg->type;

        reg = reg->next;
    }

    return NO_COMPRESSION;
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

    *length = 0;
    if ( (fp = fopen( capital_name, "rb" )) != NULL && len >= 3 ){
        compression_type = getRegisteredCompressionType( capital_name );
        if ( compression_type == NBZ_COMPRESSION || compression_type == SPB_COMPRESSION ){
            *length = getDecompressedFileLength( compression_type, fp, 0 );
        }
        else{
            fseek( fp, 0, SEEK_END );
            *length = ftell( fp );
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

    if ( fp ) fclose( fp );
    
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

size_t DirectReader::encodeNBZ( FILE *fp, size_t length, unsigned char *buf )
{
    unsigned int bytes_in, bytes_out;
	int err;

	BZFILE *bfp = BZ2_bzWriteOpen( &err, fp, 9, 0, 30 );
	if ( bfp == NULL || err != BZ_OK ) return 0;

	while( err == BZ_OK && length > 0 ){
        if ( length >= WRITE_LENGTH ){
            BZ2_bzWrite( &err, bfp, buf, WRITE_LENGTH );
            buf += WRITE_LENGTH;
            length -= WRITE_LENGTH;
        }
        else{
            BZ2_bzWrite( &err, bfp, buf, length );
            break;
        }
	}

	BZ2_bzWriteClose( &err, bfp, 0, &bytes_in, &bytes_out );
    
    return bytes_out;
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
    unsigned char *pbuf, *psbuf;
    int i, j, k, c, n, m;

    getbit_mask = 0;
    
    fseek( fp, offset, SEEK_SET );
    size_t width  = readShort( fp );
    size_t height = readShort( fp );

    size_t width_pad  = (4 - width * 3 % 4) % 4;

    size_t total_size = (width * 3 + width_pad) * height + 54;

    /* ---------------------------------------- */
    /* Write header */
    memset( buf, 0, 54 );
    buf[0] = 'B'; buf[1] = 'M';
    buf[2] = total_size & 0xff;
    buf[3] = (total_size >>  8) & 0xff;
    buf[4] = (total_size >> 16) & 0xff;
    buf[5] = (total_size >> 24) & 0xff;
    buf[10] = 54; // offset to the body
    buf[14] = 40; // header size
    buf[18] = width & 0xff;
    buf[19] = (width >> 8)  & 0xff;
    buf[22] = height & 0xff;
    buf[23] = (height >> 8)  & 0xff;
    buf[26] = 1; // the number of the plane
    buf[28] = 24; // bpp
    buf[34] = total_size - 54; // size of the body

    buf += 54;

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

        pbuf  = buf + (width * 3 + width_pad)*(height-1) + i;
        psbuf = spb_buffer;

        for ( j=0 ; j<height ; j++ ){
            if ( j & 1 ){
                for ( k=0 ; k<width ; k++, pbuf -= 3 ) *pbuf = *psbuf++;
                pbuf -= width * 3 + width_pad - 3;
            }
            else{
                for ( k=0 ; k<width ; k++, pbuf += 3 ) *pbuf = *psbuf++;
                pbuf -= width * 3 + width_pad + 3;
            }
        }
    }
    
    delete[] spb_buffer;
    
    return total_size;
}

size_t DirectReader::decodeLZSS( struct ArchiveInfo *ai, int no, unsigned char *buf )
{
    unsigned int count = 0;
    int i, j, k, r, c;
    unsigned char *lzss_buffer = new unsigned char[ N * 2 ];

    getbit_mask = 0;

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

size_t DirectReader::getDecompressedFileLength( int type, FILE *fp, size_t offset )
{
    fpos_t pos;
    size_t length;
    fgetpos( fp, &pos );
    fseek( fp, offset, SEEK_SET );
    
    if ( type == NBZ_COMPRESSION ){
        length = readLong( fp );
    }
    else if ( type == SPB_COMPRESSION ){
        size_t width  = readShort( fp );
        size_t height = readShort( fp );
        size_t width_pad  = (4 - width * 3 % 4) % 4;
            
        length = (width * 3 +width_pad) * height + 54;
    }
    fsetpos( fp, &pos );

    return length;
}
