/* -*- C++ -*-
 * 
 *  ScriptHandler.h - Script manipulation class
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

#ifndef __SCRIPT_HANDLER_H__
#define __SCRIPT_HANDLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseReader.h"

#define VARIABLE_RANGE 4096
#define ARRAY_VARIABLE_RANGE 200

#define SKIP_SPACE(p) while ( *(p) == ' ' || *(p) == '\t' ) (p)++

class ScriptHandler
{
public:
    struct LabelInfo{
        char *name;
        char *start_address;
        int  num_of_lines;
        bool access_flag;
    };

    struct ArrayVariable{
        int num_dim;
        int dim[20];
        int *data;
        ArrayVariable(){
            data = NULL;
        };
    };

    ScriptHandler();
    ~ScriptHandler();
    FILE *fopen( const char *path, const char *mode );

    char *saveStringBuffer();
    char *getCurrent();
    void setCurrent( char *pos, bool reread_flag=true );
    char *getNext();
    void pushCurrent( char *pos, bool reread_flag=true );
    void popCurrent();

    bool isName( const char *name );
    bool isText();
    void setText( bool val );
    bool isQuat();
    bool isEndWithComma();
    void skipLine( int no=1 );

    bool isKidoku();
    void markAsKidoku();
    bool rereadToken();
    bool readToken();
    char *getStringBuffer();
    char *getSavedStringBuffer();

    const char *readStr( char *dst_buf=NULL, bool reread_flag=false );
    int readInt( bool reread_flag=false );
    int parseInt( char **buf );
    void setInt( char *buf, int val, int offset = 0 );
    void setNumVariable( int no, int val );

    int decodeArraySub( char **buf, ArrayVariable *array );
    void getSJISFromInteger( char *buffer, int no, bool add_space_flag );

    int readScriptSub( FILE *fp, char **buf, bool encrypt_flag );
    int readScript( char *path );
    int labelScript();

    LabelInfo lookupLabel( const char* label );
    LabelInfo lookupLabelNext( const char* label );
    bool getLabelAccessFlag( const char *label );
    void errorAndExit( char *str );

    void saveLabelLog();
    void loadLabelLog();

    void saveKidokuData();
    void loadKidokuData();
    
    void addNumAlias( const char *str, int no );
    void addStrAlias( const char *str1, const char *str2 );
    

    /* ---------------------------------------- */
    /* Variable */
    typedef enum{ VAR_INT, VAR_STR, VAR_ARRAY };
    int num_variables[ VARIABLE_RANGE ];
    int num_limit_upper[ VARIABLE_RANGE ];
    int num_limit_lower[ VARIABLE_RANGE ];
    bool num_limit_flag[ VARIABLE_RANGE ];
    char *str_variables[ VARIABLE_RANGE ];
    ArrayVariable array_variables[ ARRAY_VARIABLE_RANGE ], tmp_array_variable;
    
    bool svga_flag;

    BaseReader *cBR;
    
    bool text_line_flag;
    bool next_text_line_flag;

private:
    struct StackInfo{
        struct StackInfo *next;
        char *current_script;

        StackInfo(){
            StackInfo(NULL);
        };
        StackInfo( char *current ){
            next = NULL;
            current_script = current;
        };
    };

    struct NameAlias{
        struct NameAlias *next;
        char *alias;
        int  num;

        NameAlias(){
            next = NULL;
        };
        NameAlias( const char *str, int no ){
            next  = NULL;
            alias = new char[ strlen(str) + 1];
            strcpy( alias, str );
            num = no;
        };
    };
    
    struct StringAlias{
        struct StringAlias *next;
        char *alias;
        char *str;

        StringAlias(){
            next  = NULL;
        };
        StringAlias( const char *str1, const char *str2 ){
            next  = NULL;
            alias = new char[ strlen(str1) + 1];
            strcpy( alias, str1 );
            str = new char[ strlen(str2) + 1];
            strcpy( str, str2 );
        };
    };


    void addStringBuffer( char ch, int string_counter );
    int findLabel( const char* label );

    int *decodeArray( char **buf, int offset = 0 );


    /* ---------------------------------------- */
    /* Variable */
    NameAlias root_name_alias, *last_name_alias;
    StringAlias root_str_alias, *last_str_alias;
    
    char *archive_path;
    int script_buffer_length;
    char *script_buffer;
    
    int string_buffer_length;
    char *string_buffer; // update only be readToken
    char *saved_string_buffer; // updated only by saveStringBuffer
    char *str_string_buffer; // updated only by readStr

    LabelInfo *label_info;
    int num_of_labels;
    int num_of_labels_accessed;

    char *kidoku_buffer;

    bool text_flag; // true if the current token is text
    bool quat_flag;
    bool end_with_comma_flag;

    StackInfo root_stack_info, *last_stack_info;
    char *current_script;
    char *next_script;
    char *stack_script;
};

#endif // __SCRIPT_HANDLER_H__
