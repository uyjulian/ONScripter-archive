/* -*- C++ -*-
 *
 *  NsaReader.h - Reader from a NSA archive
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

#ifndef __NSA_READER_H__
#define __NSA_READER_H__

#include "SarReader.h"
#define MAX_EXTRA_ARCHIVE 9

class NsaReader : virtual public SarReader
{
public:
    NsaReader();
    ~NsaReader();

    int open( char *name=NULL );
    char *getArchiveName() const;
    int getNumFiles();
    int getNumAccessed();
    
    size_t getFileLength( char *file_name );
    size_t getFile( char *file_name, unsigned char *buf );
    struct FileInfo getFileByIndex( int index );
    bool getAccessFlag( char *file_name );
    
private:
    bool sar_flag;
    int  getbit_mask;
    struct ArchiveInfo archive_info2[MAX_EXTRA_ARCHIVE];
    int num_of_nsa_archives;

    size_t getFileLengthSub( ArchiveInfo *ai, char *file_name );
    size_t getFileSub( ArchiveInfo *ai, char *file_name, unsigned char *buffer );
    int getbit( FILE *fp, int n );
    size_t decodeSPB( struct ArchiveInfo *ai, int no, unsigned char *buf );
    size_t decodeLZSS( struct ArchiveInfo *ai, int no, unsigned char *buffer );
};

#endif // __NSA_READER_H__
