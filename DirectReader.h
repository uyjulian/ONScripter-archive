//$Id:$ -*- C++ -*-
/*
 *  DirectReader.h - Reader from independent files
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

#ifndef __DIRECT_READER_H__
#define __DIRECT_READER_H__

#include "BaseReader.h"
#include <string.h>

#define MAX_FILE_NAME_LENGTH 512

class DirectReader : virtual public BaseReader
{
public:
    DirectReader( char *path=NULL );
    ~DirectReader();

    int open( char *name=NULL );
    int close();

    char *getArchiveName() const;
    int getNumFiles();
    int getNumAccessed();

    bool getAccessFlag( char *file_name );
    struct FileInfo getFileByIndex( int index );
    size_t getFileLength( char *file_name );
    size_t getFile( char *file_name, unsigned char *buffer );

protected:
    char *archive_path;
    int  getbit_mask;

    FILE *fopen(const char *path, const char *mode);
    unsigned char readChar( FILE *fp );
    unsigned short readShort( FILE *fp );
    unsigned long readLong( FILE *fp );
    char capital_name[ MAX_FILE_NAME_LENGTH + 1 ];
    size_t decodeNBZ( FILE *fp, size_t offset, unsigned char *buf );
    int getbit( FILE *fp, int n );
    size_t decodeSPB( FILE *fp, size_t offset, unsigned char *buf );

private:
    FILE *getFileHandle( char *file_name, int &compression_type, size_t *length );
};

#endif // __DIRECT_READER_H__
