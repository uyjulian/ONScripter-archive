/* -*- C++ -*-
 *
 *  SarReader.cpp - Reader from a SAR archive
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

#ifndef __SAR_READER_H__
#define __SAR_READER_H__

#include "DirectReader.h"

class SarReader : virtual public DirectReader
{
public:
    SarReader();
    ~SarReader();

    int open();
    int close();
    int getNumFiles();
    int getNumAccessed();
    
    bool getAccessFlag( char *file_name );
    size_t getFileLength( char *file_name );
    size_t getFile( char *file_name, unsigned char *buf );
    struct FileInfo getFileByIndex( int index );
    
protected:
    struct ArchiveInfo archive_info;

    unsigned char readChar( FILE *fp );
    unsigned short readShort( FILE *fp );
    unsigned long readLong( FILE *fp );
    int readArchive( struct ArchiveInfo *ai, bool nsa_flag = false );
    int getIndexFromFile( struct ArchiveInfo *ai, char *file_name );
    size_t getFileSub( ArchiveInfo *ai, char *file_name, unsigned char *buf );
};

#endif // __SAR_READER_H__
