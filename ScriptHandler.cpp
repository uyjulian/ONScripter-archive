/* -*- C++ -*-
 *
 *  ScriptHandler.cpp - Script manipulation class
 *
 *  Copyright (c) 2001-2004 Ogapee. All rights reserved.
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

#include "ScriptHandler.h"

#define SKIP_SPACE(p) while ( *(p) == ' ' || *(p) == '\t' ) (p)++

static struct {
    char *first_file;
    int  encrypt_mode;
    char *format;
} script_file_mode[] = {
    {"0.txt", 0, "%d.txt"},
    {"00.txt", 0, "%2.2d.txt"},
    {"nscr_sec.dat", 2, ""},
    {"nscript.dat", 1, ""},
    {"nscript.___", 3, ""},
    {NULL}
};

ScriptHandler::ScriptHandler()
{
    int i;
    
    script_buffer = NULL;
    end_status = END_NONE;
    log_info[LABEL_LOG].current_log = &log_info[LABEL_LOG].root_log;
    log_info[LABEL_LOG].num_logs = 0;
    log_info[LABEL_LOG].filename = "NScrllog.dat";
    log_info[FILE_LOG].current_log = &log_info[FILE_LOG].root_log;
    log_info[FILE_LOG].num_logs = 0;
    log_info[FILE_LOG].filename = "NScrflog.dat";
    kidokuskip_flag = false;
    kidoku_buffer = NULL;
    skip_enabled = false;

    text_flag = true;
    linepage_flag = false;
    textgosub_flag = false;
    clickstr_num = 0;
    clickstr_list = NULL;
    
    string_buffer_length = 512;
    string_buffer       = new char[ string_buffer_length ];
    str_string_buffer   = new char[ string_buffer_length ];
    saved_string_buffer = new char[ string_buffer_length ];

    num_of_labels = 0;

    /* ---------------------------------------- */
    for ( i=0 ; i<VARIABLE_RANGE ; i++ ){
        num_variables[i] = 0;
        num_limit_flag[i] = false;
        str_variables[i] = NULL;
    }
    root_array_variable = current_array_variable = NULL;

    screen_size = SCREEN_SIZE_640x480;
    global_variable_border = 200;

    last_name_alias = &root_name_alias;
    last_name_alias->next = NULL;
    last_str_alias = &root_str_alias;
    last_str_alias->next = NULL;
}

ScriptHandler::~ScriptHandler()
{
    if ( script_buffer )       delete[] script_buffer;
    if ( string_buffer )       delete[] string_buffer;
    if ( str_string_buffer )   delete[] string_buffer;
    if ( saved_string_buffer ) delete[] saved_string_buffer;
    deleteLog( LABEL_LOG );
    deleteLog( FILE_LOG );
}

FILE *ScriptHandler::fopen( const char *path, const char *mode )
{
    char * file_name = new char[strlen(archive_path)+strlen(path)+1];
    sprintf( file_name, "%s%s", archive_path, path );

    FILE *fp = ::fopen( file_name, mode );
    delete[] file_name;

    return fp;
}

void ScriptHandler::setKeyTable( const unsigned char *key_table )
{
    int i;
    if (key_table){
        key_table_flag = true;
        for (i=0 ; i<256 ; i++) this->key_table[i] = key_table[i];
    }
    else{
        key_table_flag = false;
        for (i=0 ; i<256 ; i++) this->key_table[i] = i;
    }
}

// basic parser function
const char *ScriptHandler::readToken()
{
    current_script = next_script;
    char *buf = current_script;
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    text_flag = false;

    SKIP_SPACE( buf );
    markAsKidoku( buf );

    string_counter = 0;
    char ch = *buf;
    if (ch == ';'){ // comment
        addStringBuffer( ch );
        do{
            ch = *++buf;
            addStringBuffer( ch );
        } while ( ch != 0x0a && ch != '\0' );
    }
    else if (ch & 0x80 ||
             (ch >= '0' && ch <= '9') ||
             ch == '@' || ch == '\\' || ch == '/' ||
             ch == '%' || ch == '?' || ch == '$' ||
             ch == '!' || ch == '#' || ch == ',' || ch == '"'){ // text
        bool loop_flag = true;
        do{
            if (ch & 0x80){
                if ( textgosub_flag && checkClickstr(buf) ) loop_flag = false;
                addStringBuffer( ch );
                ch = *++buf;
                addStringBuffer( ch );
                buf++;
                SKIP_SPACE(buf);
                ch = *buf;
            }
            else{
                for (unsigned int i=0 ; i<2 ; i++){
                    if (ch == '%' || ch == '?'){
                        addIntVariable(&buf);
                        i = 1;
                    }
                    else if (ch == '$'){
                        addStrVariable(&buf);
                        i = 1;
                    }
                    else{
                        if (textgosub_flag && (ch == '@' || ch == '\\'))
                            loop_flag = false;
                        addStringBuffer( ch );
                        buf++;
                    }
                    SKIP_SPACE(buf);
                    ch = *buf;
                    if (ch == 0x0a || ch == '\0' || !loop_flag) break;
                }
            }
        }
        while (ch != 0x0a && ch != '\0' && loop_flag);
        if (linepage_flag) addStringBuffer( '\\' );
        if (!textgosub_flag && loop_flag && ch == 0x0a){
            addStringBuffer( ch );
            markAsKidoku( buf++ );
        }
            
        text_flag = true;
    }
#ifdef ENABLE_1BYTE_CHAR
    else if (ch == '`'){
        ch = *++buf;
        while (ch != '`' && ch != 0x0a && ch !='\0'){
            if (ch & 0x80){
                addStringBuffer( ch );
                ch = *++buf;
            }
            addStringBuffer( ch );
            ch = *++buf;
        }
        if (ch == '`') buf++;
        if (linepage_flag) addStringBuffer( '\\' );
        if (ch == 0x0a){
            addStringBuffer( ch );
            markAsKidoku( buf++ );
        }
        
        text_flag = true;
        end_status |= END_1BYTE_CHAR;
    }
#endif
    else if ((ch >= 'a' && ch <= 'z') || 
             (ch >= 'A' && ch <= 'Z') ||
             ch == '_'){ // command
        do{
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            addStringBuffer( ch );
            ch = *++buf;
        }
        while((ch >= 'a' && ch <= 'z') || 
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_');
    }
    else if (ch == '*'){ // label
        return readLabel();
    }
    else if (ch == '~' || ch == 0x0a || ch == ':'){
        addStringBuffer( ch );
        markAsKidoku( buf++ );
    }
    else if (ch != '\0'){
        fprintf(stderr, "readToken: unknown heading character %c (%x)\n", ch, ch);
    }

    next_script = checkComma(buf);

    //printf("readToken [%s] len=%d [%c(%x)] %x\n", string_buffer, strlen(string_buffer), ch, ch, next_script);

    return string_buffer;
}

const char *ScriptHandler::readLabel()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE( current_script );
    char *buf = current_script;

    string_counter = 0;
    char ch = *buf;
    if ((ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z') ||
        ch == '_' || ch == '*'){
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
        addStringBuffer( ch );
        buf++;
        if (ch == '*') SKIP_SPACE(buf);
        
        ch = *buf;
        while((ch >= 'a' && ch <= 'z') || 
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_'){
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            addStringBuffer( ch );
            ch = *++buf;
        }
    }
    addStringBuffer( '\0' );
    
    next_script = checkComma(buf);

    return string_buffer;
}

const char *ScriptHandler::readStr()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE( current_script );
    char *buf = current_script;
    
    parseStr(&buf);

    next_script = checkComma(buf);
    
    strcpy( string_buffer, str_string_buffer );
    string_counter = strlen(string_buffer);
    
    return string_buffer;
}

int ScriptHandler::readInt()
{
    string_counter = 0;
    string_buffer[string_counter] = '\0';

    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE( current_script );
    char *buf = current_script;
    
    int ret = parseIntExpression(&buf);
    
    next_script = checkComma(buf);

    return ret;
}

void ScriptHandler::skipToken()
{
    current_script = next_script;
    char *buf = current_script;

    bool quat_flag = false;
    bool text_flag = false;
    while(1){
        if ( *buf == 0x0a || 
             (!quat_flag && !text_flag && (*buf == ':' || *buf == ';') ) ) break;
        if ( *buf == '"' ) quat_flag = !quat_flag;
        if ( *buf & 0x80 ){
            buf += 2;
            if ( !quat_flag ) text_flag = true;
        }
        else
            buf++;
    }
    next_script = buf;
}

// string access function
char *ScriptHandler::getStringBuffer()
{
    return string_buffer;
}

char *ScriptHandler::saveStringBuffer()
{
    strcpy( saved_string_buffer, string_buffer );
    return saved_string_buffer;
}

// script address direct manipulation function
char *ScriptHandler::getCurrent()
{
    return current_script;
}

char *ScriptHandler::getNext()
{
    return next_script;
}

void ScriptHandler::setCurrent( char *pos, bool read_flag )
{
    current_script = next_script = pos;
    if ( read_flag ) readToken();
}

void ScriptHandler::pushCurrent( char *pos )
{
    pushed_current_script = current_script;
    pushed_next_script = next_script;

    current_script = pos;
    next_script = pos;
}

void ScriptHandler::popCurrent()
{
    current_script = pushed_current_script;
    next_script = pushed_next_script;
}

int ScriptHandler::getOffset( char *pos )
{
    return pos - script_buffer;
}

char *ScriptHandler::getAddress( int offset )
{
    return script_buffer + offset;
}

int ScriptHandler::getLineByAddress( char *address )
{
    LabelInfo label = getLabelByAddress( address );

    char *addr = label.label_header;
    int line = 0;
    while ( address > addr ){
        if ( *addr == 0x0a ) line++;
        addr++;
    }
    return line;
}

char *ScriptHandler::getAddressByLine( int line )
{
    LabelInfo label = getLabelByLine( line );
    
    int l = line - label.start_line;
    char *addr = label.label_header;
    while ( l > 0 ){
        while( *addr != 0x0a ) addr++;
        addr++;
        l--;
    }
    return addr;
}

ScriptHandler::LabelInfo ScriptHandler::getLabelByAddress( char *address )
{
    int i;
    for ( i=0 ; i<num_of_labels-1 ; i++ ){
        if ( label_info[i+1].start_address > address )
            return label_info[i];
    }
    return label_info[i];
}

ScriptHandler::LabelInfo ScriptHandler::getLabelByLine( int line )
{
    int i;
    for ( i=0 ; i<num_of_labels-1 ; i++ ){
        if ( label_info[i+1].start_line > line )
            return label_info[i];
    }
    return label_info[i];
}

bool ScriptHandler::isName( const char *name )
{
    return (strncmp( name, string_buffer, strlen(name) )==0)?true:false;
}

bool ScriptHandler::isText()
{
    return text_flag;
}

bool ScriptHandler::compareString(const char *buf)
{
    SKIP_SPACE(next_script);
    unsigned int i, num = strlen(buf);
    for (i=0 ; i<num ; i++){
        unsigned char ch = next_script[i];
        if ('A' <= ch && 'Z' >= ch) ch += 'a' - 'A';
        if (ch != buf[i]) break;
    }
    return (i==num)?true:false;
}

void ScriptHandler::setText( bool val )
{
    text_flag = val;
}

int ScriptHandler::getEndStatus()
{
    return end_status;
}

void ScriptHandler::skipLine( int no )
{
    for ( int i=0 ; i<no ; i++ ){
        while ( *current_script != 0x0a ) current_script++;
        current_script++;
    }
    next_script = current_script;
}

void ScriptHandler::setLinepage( bool val )
{
    linepage_flag = val;
}

// function for kidoku history
bool ScriptHandler::isKidoku()
{
    return skip_enabled;
}

void ScriptHandler::markAsKidoku( char *address )
{
    if ( !kidokuskip_flag ) return;
    int offset = current_script - script_buffer;
    if ( address ) offset = address - script_buffer;
    //printf("mark (%c)%x:%x = %d\n", *current_script, offset /8, offset%8, kidoku_buffer[ offset/8 ] & ((char)1 << (offset % 8)));
    if ( kidoku_buffer[ offset/8 ] & ((char)1 << (offset % 8)) )
        skip_enabled = true;
    else
        skip_enabled = false;
    kidoku_buffer[ offset/8 ] |= ((char)1 << (offset % 8));
}

void ScriptHandler::setKidokuskip( bool kidokuskip_flag )
{
    this->kidokuskip_flag = kidokuskip_flag;
}

void ScriptHandler::saveKidokuData()
{
    FILE *fp;

    if ( ( fp = fopen( "kidoku.dat", "wb" ) ) == NULL ){
        fprintf( stderr, "can't write kidoku.dat\n" );
        return;
    }

    fwrite( kidoku_buffer, 1, script_buffer_length/8, fp );
    fclose( fp );
}

void ScriptHandler::loadKidokuData()
{
    FILE *fp;

    setKidokuskip( true );
    kidoku_buffer = new char[ script_buffer_length/8 + 1 ];
    memset( kidoku_buffer, 0, script_buffer_length/8 + 1 );

    if ( ( fp = fopen( "kidoku.dat", "rb" ) ) != NULL ){
        fread( kidoku_buffer, 1, script_buffer_length/8, fp );
        fclose( fp );
    }
}

void ScriptHandler::addIntVariable(char **buf)
{
    char num_buf[10], num_sjis_buf[3];
    int no = parseInt(buf);
    if (no < 0){
        addStringBuffer( "�|"[0] );
        addStringBuffer( "�|"[1] );
        sprintf( num_buf, "%d", -no );
    }
    else{
        sprintf( num_buf, "%d", no );
    }
    for (unsigned int i=0 ; i<strlen( num_buf ) ; i++){
        getSJISFromInteger( num_sjis_buf, num_buf[i] - '0', false );
        addStringBuffer( num_sjis_buf[0] );
        addStringBuffer( num_sjis_buf[1] );
    }
}

void ScriptHandler::addStrVariable(char **buf)
{
    (*buf)++;
    int no = parseInt(buf);
    if ( str_variables[no] ){
        for (unsigned int i=0 ; i<strlen( str_variables[no] ) ; i++){
            addStringBuffer( str_variables[no][i] );
        }
    }
}

void ScriptHandler::enableTextgosub(bool val)
{
    textgosub_flag = val;
}

void ScriptHandler::setClickstr(int num, const char *list)
{
    clickstr_num = num;
    clickstr_list = new char[clickstr_num * 2];
    for ( int i=0 ; i<clickstr_num*2 ; i++ ) clickstr_list[i] = list[i];
}

bool ScriptHandler::checkClickstr(const char *buf)
{
    int i, j, ret = false;

    for (i=0 ; i<clickstr_num ; i++){
        if ( clickstr_list[i*2] == buf[0] && clickstr_list[i*2+1] == buf[1] ){
            ret = true;
            if ( buf[2] == '@' || buf[2] == '\\' ) ret = false;

            for (j=0 ; j<clickstr_num ; j++){
                if ( clickstr_list[j*2] == buf[2] && clickstr_list[j*2+1] == buf[3] ){
                    ret = false;
                    break;
                }
            }
            break;
        }
    }
    
    return ret;
}

int ScriptHandler::getIntVariable( VariableInfo *var_info )
{
    if ( var_info == NULL ) var_info = &current_variable;
    
    if ( var_info->type == VAR_INT )
        return num_variables[ var_info->var_no];
    else if ( var_info->type == VAR_ARRAY )
        return *getArrayPtr( var_info->var_no, var_info->array, 0 );
    return 0;
}

void ScriptHandler::readVariable( bool reread_flag )
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;
    if ( reread_flag ) next_script = current_script;
    current_script = next_script;
    char *buf = current_script;

    SKIP_SPACE(buf);

    bool ptr_flag = false;
    if ( *buf == 'i' || *buf == 'f' ){
        ptr_flag = true;
        buf++;
    }

    if ( *buf == '%' ){
        buf++;
        current_variable.var_no = parseInt(&buf);
        current_variable.type = VAR_INT;
    }
    else if ( *buf == '?' ){
        current_variable.var_no = parseArray( &buf, current_variable.array );
        current_variable.type = VAR_ARRAY;
    }
    else if ( *buf == '$' ){
        buf++;
        current_variable.var_no = parseInt(&buf);
        current_variable.type = VAR_STR;
    }

    if (ptr_flag) current_variable.type |= VAR_PTR;

    next_script = checkComma(buf);
}

void ScriptHandler::setInt( VariableInfo *var_info, int val, int offset )
{
    if ( var_info->type & VAR_INT ){
        setNumVariable( var_info->var_no + offset, val );
    }
    else if ( var_info->type & VAR_ARRAY ){
        *getArrayPtr( var_info->var_no, var_info->array, offset ) = val;
    }
    else{
        errorAndExit( "setInt: no variables." );
    }
}

void ScriptHandler::pushVariable()
{
    pushed_variable = current_variable;
}

void ScriptHandler::setNumVariable( int no, int val )
{
    if ( no >= VARIABLE_RANGE ) errorAndExit( "setNumVariable: range exceeds." );
    if ( num_limit_flag[no] ){
        if      ( val < num_limit_lower[no] ) val = num_limit_lower[no];
        else if ( val > num_limit_upper[no] ) val = num_limit_upper[no];
    }
    num_variables[no] = val;
}

void ScriptHandler::getSJISFromInteger( char *buffer, int no, bool add_space_flag )
{
    int c = 0;
    char num_str[] = "�O�P�Q�R�S�T�U�V�W�X";
    if ( no >= 10 ){
        buffer[c++] = num_str[ no / 10 % 10 * 2];
        buffer[c++] = num_str[ no / 10 % 10 * 2 + 1];
    }
    else if ( add_space_flag ){
        buffer[c++] = ((char*)"�@")[0];
        buffer[c++] = ((char*)"�@")[1];
    }
    buffer[c++] = num_str[ no % 10 * 2];
    buffer[c++] = num_str[ no % 10 * 2 + 1];
    buffer[c++] = '\0';
}

int ScriptHandler::readScriptSub( FILE *fp, char **buf, int encrypt_mode )
{
    char magic[5] = {0x79, 0x57, 0x0d, 0x80, 0x04 };
    int  magic_counter = 0;
    bool newline_flag = true;
    bool cr_flag = false;

    if (encrypt_mode == 3 && !key_table_flag)
        errorAndExit("readScriptSub: key_exe must be specified with --key-exe option.\n");

    while(1)
    {
        int ch = fgetc( fp );
        if ( ch != EOF ){
            if      ( encrypt_mode == 1 ) ch ^= 0x84;
            else if ( encrypt_mode == 2 ){
                ch = (ch ^ magic[magic_counter++]) & 0xff;
                if ( magic_counter == 5 ) magic_counter = 0;
            }
            else if ( encrypt_mode == 3){
                ch = key_table[ch] ^ 0x84;
            }
        }

        if ( cr_flag && ch != 0x0a ){
            *(*buf)++ = 0x0a;
            newline_flag = true;
            cr_flag = false;
        }
        if ( ch == EOF ) break;
    
        if ( ch == '*' && newline_flag ) num_of_labels++;
        if ( ch == 0x0d ){
            cr_flag = true;
            continue;
        }
        if ( ch == 0x0a ){
            *(*buf)++ = 0x0a;
            newline_flag = true;
            cr_flag = false;
        }
        else{
            *(*buf)++ = ch;
            if ( ch != ' ' && ch != '\t' )
                newline_flag = false;
        }
    }

    *(*buf)++ = 0x0a;
    return 0;
}

int ScriptHandler::readScript( char *path )
{
    archive_path = new char[ strlen(path) + 1 ];
    strcpy( archive_path, path );

    FILE *fp;
    char file_name[10];
    int  file_counter = 0;
    int estimated_buffer_length = 0;

    int no=0;
    while( script_file_mode[no].first_file ){
        if ( (fp = fopen( script_file_mode[no].first_file, "rb" )) != NULL ){
            while(1){
                fseek( fp, 0, SEEK_END );
                estimated_buffer_length += ftell( fp ) + 1;
                if (script_file_mode[no].encrypt_mode == 0){
                    fclose( fp );
                    sprintf( file_name, script_file_mode[no].format, ++file_counter );
                    if ((fp = fopen( file_name, "rb" )) != NULL) continue;
                }
                break;
            }
            break;
        }
        no++;
    }
    if (script_file_mode[no].first_file == NULL){
        fprintf( stderr, "can't open file 0.txt or 00.txt or nscript.dat or nscript.___\n" );
        return -1;
    }
    
    if ( script_buffer ) delete[] script_buffer;
    if ( ( script_buffer = new char[ estimated_buffer_length ]) == NULL ){
        fprintf( stderr, " *** can't allocate memory for the script ***\n");
        exit( -1 );
    }

    char *p_script_buffer;
    current_script = p_script_buffer = script_buffer;
    
    if (script_file_mode[no].encrypt_mode > 0){
        fseek( fp, 0, SEEK_SET );
        readScriptSub( fp, &p_script_buffer, script_file_mode[no].encrypt_mode );
        fclose( fp );
    }
    else{
        for ( int i=0 ; i<file_counter ; i++ ){
            sprintf( file_name, script_file_mode[no].format, i );
            fp = fopen( file_name, "rb" );
            readScriptSub( fp, &p_script_buffer, script_file_mode[no].encrypt_mode );
            fclose( fp );
        }
    }

    script_buffer_length = p_script_buffer - script_buffer;
    
    /* ---------------------------------------- */
    /* screen size and value check */
    char *buf = script_buffer+1;
    while( script_buffer[0] == ';' ){
        if ( !strncmp( buf, "mode", 4 ) ){
            buf += 4;
            if      ( !strncmp( buf, "800", 3 ) )
                screen_size = SCREEN_SIZE_800x600;
            else if ( !strncmp( buf, "400", 3 ) )
                screen_size = SCREEN_SIZE_400x300;
            else if ( !strncmp( buf, "320", 3 ) )
                screen_size = SCREEN_SIZE_320x240;
            else
                screen_size = SCREEN_SIZE_640x480;
            buf += 3;
        }
        else if ( !strncmp( buf, "value", 5 ) ){
            buf += 5;
            global_variable_border = 0;
            while ( *buf >= '0' && *buf <= '9' )
                global_variable_border = global_variable_border * 10 + *buf++ - '0';
        }
        else{
            break;
        }
        if ( *buf != ',' ) break;
        buf++;
    }

    return labelScript();
}

int ScriptHandler::labelScript()
{
    int label_counter = -1;
    int current_line = 0;
    char *buf = script_buffer;
    label_info = new LabelInfo[ num_of_labels+1 ];

    while ( buf < script_buffer + script_buffer_length ){
        SKIP_SPACE( buf );
        if ( *buf == '*' ){
            setCurrent( buf );
            readLabel();
            label_info[ ++label_counter ].name = new char[ strlen(string_buffer) ];
            strcpy( label_info[ label_counter ].name, string_buffer+1 );
            label_info[ label_counter ].label_header = buf;
            label_info[ label_counter ].num_of_lines = 1;
            label_info[ label_counter ].start_line   = current_line;
            buf = getNext();
            if ( *buf == 0x0a ){
                buf++;
                SKIP_SPACE(buf);
                current_line++;
            }
            label_info[ label_counter ].start_address = buf;
        }
        else{
            if ( label_counter >= 0 )
                label_info[ label_counter ].num_of_lines++;
            while( *buf != 0x0a ) buf++;
            buf++;
            current_line++;
        }
    }

    label_info[num_of_labels].start_address = NULL;
    
    return 0;
}

struct ScriptHandler::LabelInfo ScriptHandler::lookupLabel( const char *label )
{
    int i = findLabel( label );

    findAndAddLog( LABEL_LOG, label_info[i].name, true );
    return label_info[i];
}

struct ScriptHandler::LabelInfo ScriptHandler::lookupLabelNext( const char *label )
{
    int i = findLabel( label );
    if ( i+1 < num_of_labels ){
        findAndAddLog( LABEL_LOG, label_info[i+1].name, true );
        return label_info[i+1];
    }

    return label_info[num_of_labels];
}

ScriptHandler::LogLink *ScriptHandler::findAndAddLog( LOG_TYPE type, const char *name, bool add_flag )
{
    char capital_name[256];
    for ( unsigned int i=0 ; i<strlen(name)+1 ; i++ ){
        capital_name[i] = name[i];
        if ( 'a' <= capital_name[i] && capital_name[i] <= 'z' ) capital_name[i] += 'A' - 'a';
        else if ( capital_name[i] == '/' ) capital_name[i] = '\\';
    }
    
    LogInfo *info = NULL;
    if ( type == LABEL_LOG )
        info = &log_info[LABEL_LOG];
    else
        info = &log_info[FILE_LOG];

    LogLink *cur = info->root_log.next;
    while( cur ){
        if ( !strcmp( cur->name, capital_name ) ) break;
        cur = cur->next;
    }
    if ( !add_flag || cur ) return cur;

    LogLink *link = new LogLink();
    link->name = new char[strlen(capital_name)+1];
    strcpy( link->name, capital_name );
    info->current_log->next = link;
    info->current_log = info->current_log->next;
    info->num_logs++;
    
    return link;
}

void ScriptHandler::deleteLog( LOG_TYPE type )
{
    LogInfo *info = NULL;
    if ( type == LABEL_LOG )
        info = &log_info[LABEL_LOG];
    else
        info = &log_info[FILE_LOG];
    info->current_log = &info->root_log;

    LogLink *link = info->root_log.next;

    while( link ){
        LogLink *tmp = link;
        link = link->next;
        delete tmp;
    }
    info->num_logs = 0;
    info->root_log.next = NULL;
}

void ScriptHandler::loadLog( LOG_TYPE type )
{
    deleteLog( type );
    
    int i, j, ch, count = 0;
    char buf[100];

    LogInfo *info = NULL;
    if ( type == LABEL_LOG )
        info = &log_info[LABEL_LOG];
    else
        info = &log_info[FILE_LOG];

    FILE *fp; 
    if ( ( fp = fopen( info->filename, "rb" ) ) != NULL ){
        while( (ch = fgetc( fp )) != 0x0a ){
            count = count * 10 + ch - '0';
        }

        for ( i=0 ; i<count ; i++ ){
            fgetc( fp );
            j = 0; 
            while( (ch = fgetc( fp )) != '"' ) buf[j++] = ch ^ 0x84;
            buf[j] = '\0';

            findAndAddLog( type, buf, true );
        }

        fclose( fp );
    }
}

void ScriptHandler::saveLog( LOG_TYPE type )
{
    int  i,j;
    char buf[10];

    LogInfo *info = NULL;
    if ( type == LABEL_LOG )
        info = &log_info[LABEL_LOG];
    else
        info = &log_info[FILE_LOG];

    FILE *fp;
    if ( ( fp = fopen( info->filename, "wb" ) ) == NULL ){
        fprintf( stderr, "can't write %s\n", info->filename );
        exit( -1 );
    }

    sprintf( buf, "%d", info->num_logs );
    for ( i=0 ; i<(int)strlen( buf ) ; i++ ) fputc( buf[i], fp );
    fputc( 0x0a, fp );

    LogLink *cur = info->root_log.next;
    for ( i=0 ; i<info->num_logs ; i++ ){
        fputc( '"', fp );
        for ( j=0 ; j<(int)strlen( cur->name ) ; j++ )
            fputc( cur->name[j] ^ 0x84, fp );
        fputc( '"', fp );
        cur = cur->next;
    }
    
    fclose( fp );
}

void ScriptHandler::saveArrayVariable( FILE *fp )
{
    ArrayVariable *av = root_array_variable;

    while(av){
        int i, dim = 1;
        for ( i=0 ; i<av->num_dim ; i++ )
            dim *= av->dim[i];
        
        for ( i=0 ; i<dim ; i++ ){
            unsigned long ch = av->data[i];
            char buf[4];
            buf[3] = (ch>>24) & 0xff;
            buf[2] = (ch>>16) & 0xff;
            buf[1] = (ch>>8)  & 0xff;
            buf[0] = ch & 0xff;
            fwrite( &buf, 1, 4, fp );
        }
        av = av->next;
    }
}

void ScriptHandler::loadArrayVariable( FILE *fp )
{
    ArrayVariable *av = root_array_variable;

    while(av){
        int i, dim = 1;
        for ( i=0 ; i<av->num_dim ; i++ )
            dim *= av->dim[i];
        
        for ( i=0 ; i<dim ; i++ ){
            unsigned long ret;
            char buf[4];
            fread( &buf, 1, 4, fp );
            ret = buf[3];
            ret = ret << 8 | buf[2];
            ret = ret << 8 | buf[1];
            ret = ret << 8 | buf[0];
            av->data[i] = ret;
        }
        av = av->next;
    }
}

void ScriptHandler::addNumAlias( const char *str, int no )
{
    NameAlias *p_name_alias = new NameAlias( str, no );
    last_name_alias->next = p_name_alias;
    last_name_alias = last_name_alias->next;
}

void ScriptHandler::addStrAlias( const char *str1, const char *str2 )
{
    StringAlias *p_str_alias = new StringAlias( str1, str2 );
    last_str_alias->next = p_str_alias;
    last_str_alias = last_str_alias->next;
}

void ScriptHandler::errorAndExit( char *str )
{
    fprintf( stderr, " **** Script error, %s [%s] ***\n", str, string_buffer );
    exit(-1);
}

void ScriptHandler::addStringBuffer( char ch )
{
    /* In case of string bufer over flow */
    if ( string_counter+1 == string_buffer_length ){
        string_buffer_length += 512;
        char *tmp_buffer = new char[ string_buffer_length ];
        memcpy( tmp_buffer, string_buffer, string_counter+1 );

        delete[] string_buffer;
        string_buffer = tmp_buffer;

        delete[] str_string_buffer;
        str_string_buffer = new char[ string_buffer_length ];

        delete[] saved_string_buffer;
        saved_string_buffer = new char[ string_buffer_length ];
    }

    string_buffer[string_counter++] = ch;
    string_buffer[string_counter] = '\0';
}

// ----------------------------------------
// Private methods

int ScriptHandler::findLabel( const char *label )
{
    int i;
    char capital_label[256];

    for ( i=0 ; i<(int)strlen( label )+1 ; i++ ){
        capital_label[i] = label[i];
        if ( 'A' <= capital_label[i] && capital_label[i] <= 'Z' ) capital_label[i] += 'a' - 'A';
    }
    for ( i=0 ; i<num_of_labels ; i++ ){
        if ( !strcmp( label_info[i].name, capital_label ) )
            return i;
    }

    char *p = new char[ strlen(label) + 32 ];
    sprintf(p, "Label \"%s\" is not found.", label);
    errorAndExit( p );
    
    return -1; // dummy
}

char *ScriptHandler::checkComma( char *buf )
{
    SKIP_SPACE( buf );
    if (*buf == ','){
        end_status |= END_COMMA;
        buf++;
        SKIP_SPACE( buf );
    }
    
    return buf;
}

void ScriptHandler::parseStr( char **buf )
{
    SKIP_SPACE( *buf );

    if ( **buf == '(' ){
        (*buf)++;
        parseStr(buf);
        SKIP_SPACE( *buf );
        if ( (*buf)[0] != ')' ) errorAndExit("parseStr: ) is not found.");
        (*buf)++;

        if ( findAndAddLog( FILE_LOG, str_string_buffer, false ) ){
            parseStr(buf);
            char *tmp_buf = new char[ strlen( str_string_buffer ) + 1 ];
            strcpy( tmp_buf, str_string_buffer );
            parseStr(buf);
            strcpy( str_string_buffer, tmp_buf );
            delete[] tmp_buf;
        }
        else{
            parseStr(buf);
            parseStr(buf);
        }
        current_variable.type |= VAR_CONST;
    }
    else if ( **buf == '$' ){
        (*buf)++;
        int no = parseInt(buf);
        if ( str_variables[no] )
            strcpy( str_string_buffer, str_variables[no] );
        else
            str_string_buffer[0] = '\0';
        current_variable.type = VAR_STR;
        current_variable.var_no = no;
    }
    else if ( **buf == '"' ){
        int c=0;
        (*buf)++;
        while ( **buf != '"' && **buf != 0x0a )
            str_string_buffer[c++] = *(*buf)++;
        str_string_buffer[c] = '\0';
        if ( **buf == '"' ) (*buf)++;
        current_variable.type |= VAR_CONST;
        //end_status |= END_QUAT;
    }
    else if ( **buf == '#' ){ // for color
        for ( int i=0 ; i<7 ; i++ )
            str_string_buffer[i] = *(*buf)++;
        str_string_buffer[7] = '\0';
        current_variable.type = VAR_NONE;
    }
    else{ // str alias
        char ch, alias_buf[512];
        int alias_buf_len = 0;
        bool first_flag = true;
        
        while(1){
            if ( alias_buf_len == 511 ) break;
            ch = **buf;
            
            if ( (ch >= 'a' && ch <= 'z') || 
                 (ch >= 'A' && ch <= 'Z') || 
                 ch == '_' ){
                if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
                first_flag = false;
                alias_buf[ alias_buf_len++ ] = ch;
            }
            else if ( ch >= '0' && ch <= '9' ){
                if ( first_flag ) errorAndExit("parseStr: number is not allowed for the first letter of str alias.");
                first_flag = false;
                alias_buf[ alias_buf_len++ ] = ch;
            }
            else break;
            (*buf)++;
        }
        alias_buf[alias_buf_len] = '\0';
        
        if ( alias_buf_len == 0 ){
            str_string_buffer[0] = '\0';
            current_variable.type = VAR_NONE;
            return;
        }
        
        StringAlias *p_str_alias = root_str_alias.next;

        while( p_str_alias ){
            if ( !strcmp( p_str_alias->alias, (const char*)alias_buf ) ){
                strcpy( str_string_buffer, p_str_alias->str );
                break;
            }
            p_str_alias = p_str_alias->next;
        }
        if ( !p_str_alias ){
            printf("can't find str alias for %s...\n", alias_buf );
            exit(-1);
        }
        current_variable.type |= VAR_CONST;
    }
}

int ScriptHandler::parseInt( char **buf )
{
    int ret = 0;
    
    SKIP_SPACE( *buf );

    if ( **buf == '%' ){
        (*buf)++;
        current_variable.var_no = parseInt(buf);
        current_variable.type = VAR_INT;
        return num_variables[ current_variable.var_no ];
    }
    else if ( **buf == '?' ){
        current_variable.var_no = parseArray( buf, current_variable.array );
        current_variable.type = VAR_ARRAY;
        return *getArrayPtr( current_variable.var_no, current_variable.array, 0 );
    }
    else{
        char ch, alias_buf[256];
        int alias_buf_len = 0, alias_no = 0;
        bool direct_num_flag = false;
        bool num_alias_flag = false;

        char *buf_start = *buf;
        while( 1 ){
            ch = **buf;
            
            if ( (ch >= 'a' && ch <= 'z') || 
                 (ch >= 'A' && ch <= 'Z') || 
                 ch == '_' ){
                if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
                if ( direct_num_flag ) break;
                num_alias_flag = true;
                alias_buf[ alias_buf_len++ ] = ch;
            }
            else if ( ch >= '0' && ch <= '9' ){
                if ( !num_alias_flag ) direct_num_flag = true;
                if ( direct_num_flag ) 
                    alias_no = alias_no * 10 + ch - '0';
                else
                    alias_buf[ alias_buf_len++ ] = ch;
            }
            else break;
            (*buf)++;
        }

        if ( *buf - buf_start  == 0 ){
            current_variable.type = VAR_NONE;
            return 0;
        }
        
        /* ---------------------------------------- */
        /* Solve num aliases */
        if ( num_alias_flag ){
            alias_buf[ alias_buf_len ] = '\0';
            NameAlias *p_name_alias = root_name_alias.next;

            while( p_name_alias ){
                if ( !strcmp( p_name_alias->alias,
                              (const char*)alias_buf ) ){
                    alias_no = p_name_alias->num;
                    break;
                }
                p_name_alias = p_name_alias->next;
            }
            if ( !p_name_alias ){
                //printf("can't find name alias for %s... assume 0.\n", alias_buf );
                current_variable.type = VAR_NONE;
                *buf = buf_start;
                return 0;
            }
        }
        current_variable.type = VAR_INT | VAR_CONST;
        ret = alias_no;
    }

    SKIP_SPACE( *buf );

    return ret;
}

int ScriptHandler::parseIntExpression( char **buf )
{
    int num[3], op[2]; // internal buffer

    SKIP_SPACE( *buf );

    readNextOp( buf, NULL, &num[0] );

    readNextOp( buf, &op[0], &num[1] );
    if ( op[0] == OP_INVALID )
        return num[0];

    while(1){
        readNextOp( buf, &op[1], &num[2] );
        if ( op[1] == OP_INVALID ) break;

        if ( !(op[0] & 0x04) && (op[1] & 0x04) ){ // if priority of op[1] is higher than op[0]
            num[1] = calcArithmetic( num[1], op[1], num[2] );
        }
        else{
            num[0] = calcArithmetic( num[0], op[0], num[1] );
            op[0] = op[1];
            num[1] = num[2];
        }
    }
    return calcArithmetic( num[0], op[0], num[1] );
}

/*
 * Internal buffer looks like this.
 *   num[0] op[0] num[1] op[1] num[2]
 * If priority of op[0] is higher than op[1], (num[0] op[0] num[1]) is computed,
 * otherwise (num[1] op[1] num[2]) is computed.
 * Then, the next op and num is read from the script.
 * Num is an immediate value, a variable or a bracketed expression.
 */
void ScriptHandler::readNextOp( char **buf, int *op, int *num )
{
    bool minus_flag = false;
    SKIP_SPACE(*buf);
    char *buf_start = *buf;
    
    if ( op ){
        if      ( (*buf)[0] == '+' ) *op = OP_PLUS;
        else if ( (*buf)[0] == '-' ) *op = OP_MINUS;
        else if ( (*buf)[0] == '*' ) *op = OP_MULT;
        else if ( (*buf)[0] == '/' ) *op = OP_DIV;
        else if ( (*buf)[0] == 'm' &&
                  (*buf)[1] == 'o' &&
                  (*buf)[2] == 'd' &&
                  ( (*buf)[3] == ' '  ||
                    (*buf)[3] == '\t' ||
                    (*buf)[3] == '$' ||
                    (*buf)[3] == '%' ||
                    (*buf)[3] == '?' ||
                    ( (*buf)[3] >= '0' && (*buf)[3] <= '9') ))
            *op = OP_MOD;
        else{
            *op = OP_INVALID;
            return;
        }
        if ( *op == OP_MOD ) *buf += 3;
        else                 (*buf)++;
        SKIP_SPACE(*buf);
    }
    else{
        if ( (*buf)[0] == '-' ){
            minus_flag = true;
            (*buf)++;
            SKIP_SPACE(*buf);
        }
    }

    if ( (*buf)[0] == '(' ){
        (*buf)++;
        *num = parseIntExpression( buf );
        if (minus_flag) *num = -*num;
        SKIP_SPACE(*buf);
        if ( (*buf)[0] != ')' ) errorAndExit(") is not found.\n");
        (*buf)++;
    }
    else{
        *num = parseInt( buf );
        if (minus_flag) *num = -*num;
        if ( current_variable.type == VAR_NONE ){
            if (op) *op = OP_INVALID;
            *buf = buf_start;
        }
    }
}

int ScriptHandler::calcArithmetic( int num1, int op, int num2 )
{
    int ret;
    
    if      ( op == OP_PLUS )  ret = num1+num2;
    else if ( op == OP_MINUS ) ret = num1-num2;
    else if ( op == OP_MULT )  ret = num1*num2;
    else if ( op == OP_DIV )   ret = num1/num2;
    else if ( op == OP_MOD )   ret = num1%num2;

    current_variable.type = VAR_INT | VAR_CONST;

    return ret;
}

int ScriptHandler::parseArray( char **buf, struct ArrayVariable &array )
{
    SKIP_SPACE( *buf );
    
    (*buf)++; // skip '?'
    int no = parseInt( buf );

    SKIP_SPACE( *buf );
    array.num_dim = 0;
    while ( **buf == '[' ){
        (*buf)++;
        array.dim[array.num_dim] = parseIntExpression(buf);
        array.num_dim++;
        SKIP_SPACE( *buf );
        if ( **buf != ']' ) errorAndExit( "parseArray: no ']' is found." );
        (*buf)++;
    }
    for ( int i=array.num_dim ; i<20 ; i++ ) array.dim[i] = 0;

    return no;
}

int *ScriptHandler::getArrayPtr( int no, ArrayVariable &array, int offset )
{
    ArrayVariable *av = root_array_variable;
    while(av){
        if (av->no == no) break;
        av = av->next;
    }
    if (av == NULL) errorAndExit( "Array No. is not declared." );
    
    int dim = 0, i;
    for ( i=0 ; i<av->num_dim ; i++ ){
        if ( av->dim[i] <= array.dim[i] ) errorAndExit( "dim[i] <= array.dim[i]." );
        dim = dim * av->dim[i] + array.dim[i];
    }
    if ( av->dim[i-1] <= array.dim[i-1] + offset ) errorAndExit( "dim[i-1] <= array.dim[i-1] + offset." );

    return &av->data[dim+offset];
}

void ScriptHandler::declareDim()
{
    current_script = next_script;
    char *buf = current_script;

    if (current_array_variable){
        current_array_variable->next = new ArrayVariable();
        current_array_variable = current_array_variable->next;
    }
    else{
        root_array_variable = new ArrayVariable();
        current_array_variable = root_array_variable;
    }

    ScriptHandler::ArrayVariable array;
    current_array_variable->no = parseArray( &buf, array );

    int dim = 1;
    current_array_variable->num_dim = array.num_dim;
    for ( int i=0 ; i<array.num_dim ; i++ ){
        current_array_variable->dim[i] = array.dim[i]+1;
        dim *= (array.dim[i]+1);
    }
    current_array_variable->data = new int[dim];
    memset( current_array_variable->data, 0, sizeof(int) * dim );

    next_script = buf;
}
