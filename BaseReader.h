/* -*- C++ -*-
 *
 *  BaseReader.h - Base class of archive reader
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

#ifndef __BASE_READER_H__
#define __BASE_READER_H__

#include <stdio.h>
#include <assert.h>

#ifndef SEEK_END
#define SEEK_END 2
#endif

struct BaseReader
{
    struct FileInfo{
        char name[256];
        int  compressed_no;
        size_t offset;
        size_t length;
        size_t original_length;
        bool access_flag;
    };

    struct ArchiveInfo{
        FILE *file_handle;
        struct FileInfo *fi_list;
        int num_of_files;
        int num_of_accessed;
    };

    virtual ~BaseReader();
    
    virtual int open() = 0;
    virtual int close() = 0;
    
    virtual int getNumFiles() = 0;
    virtual int getNumAccessed() = 0;

    virtual bool getAccessFlag( char *file_name ) = 0;
    virtual struct FileInfo getFileByIndex( int index ) = 0;
    virtual size_t getFileLength( char *file_name ) = 0;
    virtual size_t getFile( char *file_name, unsigned char *buffer ) = 0;
};

#endif // __BASE_READER_H__
