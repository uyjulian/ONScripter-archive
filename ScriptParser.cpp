/* -*- C++ -*-
 *
 *  ScriptParser.cpp - Define block analyzer of ONScripter
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

#include "ScriptParser.h"

#define READ_LENGTH 4096
#define VERSION_STR1 "ONScripter"
#define VERSION_STR2 "Copyright (C) 2001 Studio O.G.A. All Rights Reserved."

#define DEFAULT_SAVE_MENU_NAME "ƒƒZ[ƒu„"
#define DEFAULT_LOAD_MENU_NAME "ƒƒ[ƒh„"
#define DEFAULT_SAVE_ITEM_NAME "‚µ‚¨‚è"

typedef int (ScriptParser::*FuncList)();
static struct FuncLUT{
    char command[40];
    FuncList method;
} func_lut[100] = {
    {"windoweffect",      &ScriptParser::effectCommand},
    {"versionstr",      &ScriptParser::versionstrCommand},
    {"underline", &ScriptParser::underlineCommand},
    {"transmode", &ScriptParser::transmodeCommand},
    {"time", &ScriptParser::timeCommand},
    {"sub", &ScriptParser::subCommand},
    {"stralias", &ScriptParser::straliasCommand},
    {"skip",     &ScriptParser::skipCommand},
    {"savenumber",     &ScriptParser::savenumberCommand},
    {"savename",     &ScriptParser::savenameCommand},
    {"sar",    &ScriptParser::arcCommand},
    {"rmenu",    &ScriptParser::rmenuCommand},
    {"return",   &ScriptParser::returnCommand},
    {"numalias", &ScriptParser::numaliasCommand},
    {"notif",    &ScriptParser::ifCommand},
    {"next",    &ScriptParser::nextCommand},
    {"nsa",    &ScriptParser::arcCommand},
    {"mul",      &ScriptParser::mulCommand},
    {"mov",      &ScriptParser::movCommand},
    {"mod",      &ScriptParser::modCommand},
    {"menusetwindow",      &ScriptParser::menusetwindowCommand},
    {"labellog",      &ScriptParser::labellogCommand},
    {"itoa", &ScriptParser::itoaCommand},
    {"intlimit", &ScriptParser::intlimitCommand},
    {"inc",      &ScriptParser::incCommand},
    {"if",       &ScriptParser::ifCommand},
    {"humanz",       &ScriptParser::humanzCommand},
    {"goto",     &ScriptParser::gotoCommand},
    {"gosub",    &ScriptParser::gosubCommand},
    {"globalon",    &ScriptParser::globalonCommand},
    {"game",    &ScriptParser::gameCommand},
    {"for",   &ScriptParser::forCommand},
    {"filelog",   &ScriptParser::filelogCommand},
    {"effectblank",   &ScriptParser::effectblankCommand},
    {"effect",   &ScriptParser::effectCommand},
    {"div",   &ScriptParser::divCommand},
    {"dec",   &ScriptParser::decCommand},
    {"date",   &ScriptParser::dateCommand},
    {"cmp",      &ScriptParser::cmpCommand},
    {"clickstr",   &ScriptParser::clickstrCommand},
    {"break",   &ScriptParser::breakCommand},
    {"atoi",      &ScriptParser::atoiCommand},
    {"arc",      &ScriptParser::arcCommand},
    {"add",      &ScriptParser::addCommand},
    {"", NULL}
};
    
ScriptParser::ScriptParser()
{
    unsigned int i;
    
    cBR = new DirectReader();
    cBR->open();
    script_buffer = NULL;

    globalon_flag = false;
    filelog_flag = false;
    labellog_flag = false;
    num_of_label_accessed = 0;
    underline_value = 479;
    
    num_of_labels = 0;
    label_info = NULL;

    string_buffer_offset = 0;
    string_buffer_length = 512;
    string_buffer = new char[ string_buffer_length ];
    tmp_string_buffer = new char[ string_buffer_length ];

    /* ---------------------------------------- */
    /* Global definitions */    
    version_str = new char[strlen(VERSION_STR1)+
                          strlen("\n")+
                          strlen(VERSION_STR2)+
                          strlen("\n")+
                          +1];
    sprintf( version_str, "%s\n%s\n", VERSION_STR1, VERSION_STR2 );
    jumpf_flag = false;
    srand( time(NULL) );
    z_order = 25;
    
    /* ---------------------------------------- */
    /* For loop related variables */
    root_for_link.previous = NULL;
    root_for_link.next = NULL;
    root_for_link.current_line = 0;
    root_for_link.offset = 0;
    root_for_link.label_info = lookupLabel("start");
    current_for_link = &root_for_link;
    for_stack_depth = 0;
    break_flag = false;
    
    /* ---------------------------------------- */
    /* Transmode related variables */
    trans_mode = TRANS_TOPLEFT;
    
    /* ---------------------------------------- */
    /* Save/Load related variables */
    save_menu_name = new char[ strlen( DEFAULT_SAVE_MENU_NAME ) + 1 ];
    memcpy( save_menu_name, DEFAULT_SAVE_MENU_NAME, strlen( DEFAULT_SAVE_MENU_NAME ) + 1 );
    load_menu_name = new char[ strlen( DEFAULT_LOAD_MENU_NAME ) + 1 ];
    memcpy( load_menu_name, DEFAULT_LOAD_MENU_NAME, strlen( DEFAULT_LOAD_MENU_NAME ) + 1 );
    save_item_name = new char[ strlen( DEFAULT_SAVE_ITEM_NAME ) + 1 ];
    memcpy( save_item_name, DEFAULT_SAVE_ITEM_NAME, strlen( DEFAULT_SAVE_ITEM_NAME ) + 1 );

    num_save_file = 9;
    for ( i=0 ; i<MAX_SAVE_FILE ; i++ ){
        save_file_info[i].valid = false;
        getSJISFromInteger( save_file_info[i].no, i+1 );
    }

    /* ---------------------------------------- */
    /* Text related variables */
    for ( i=0 ; i<MAX_TEXT_BUFFER-1 ; i++ ){
        text_buffer[i].next = &text_buffer[i+1];
        text_buffer[i].buffer = NULL;
    }
    text_buffer[MAX_TEXT_BUFFER-1].next = &text_buffer[0];
    text_buffer[MAX_TEXT_BUFFER-1].buffer = NULL;
    current_text_buffer = &text_buffer[0];

    clickstr_num = 0;
    clickstr_list = NULL;
    clickstr_line = 0;
    clickstr_state = CLICK_NONE;
    
    /* ---------------------------------------- */
    /* Menu related variables */
    menu_font.ttf_font  = system_font.ttf_font  = NULL;
    menu_font.color[0] = menu_font.color[1] = menu_font.color[2] = 0xff;
    system_font.color[0] = system_font.color[1] = system_font.color[2] = 0xff;
    menu_font.font_size = system_font.font_size = 18;
    menu_font.top_xy[0] = system_font.top_xy[0] = 0;
    menu_font.top_xy[1] = system_font.top_xy[1] = 16;
    menu_font.num_xy[0] = system_font.num_xy[0] = 32;
    menu_font.num_xy[1] = system_font.num_xy[1] = 23;
    menu_font.pitch_xy[0] = system_font.pitch_xy[0] = 2 + system_font.font_size;
    menu_font.pitch_xy[1] = system_font.pitch_xy[1] = 2 + system_font.font_size;
    menu_font.display_bold = system_font.display_bold = true;
    menu_font.display_shadow = system_font.display_shadow = true;
    menu_font.display_transparency = system_font.display_transparency = true;
    memcpy( system_font.window_color, "#cccccc", 8 );
    memcpy( menu_font.window_color, "#cccccc", 8 );
    system_font.window_color_mask[0] = system_font.window_color_mask[1] = system_font.window_color_mask[2] = 0xcc;
    menu_font.window_color_mask[0] = menu_font.window_color_mask[1] = menu_font.window_color_mask[2] = 0xcc;
        
    last_menu_link = &root_menu_link;
    last_menu_link->next = NULL;
    last_menu_link->label = NULL;
    menu_link_num = 0;
    menu_link_width = 0;
    
    /* ---------------------------------------- */
    for ( i=0 ; i<VARIABLE_RANGE ; i++ ){
        num_variables[i] = 0;
        num_limit_flag[i] = false;
        str_variables[i] = new char[1];
        str_variables[i][0] = '\0';
    }

    effect_blank = MINIMUM_TIMER_RESOLUTION;
    
    last_name_alias = &root_name_alias;
    last_name_alias->next = NULL;
    last_str_alias = &root_str_alias;
    last_str_alias->next = NULL;

    window_effect.effect = 10;
    window_effect.duration = 1000;
    window_effect.image = NULL;
    print_effect.effect = 10;
    print_effect.duration = 1000;
    print_effect.image = NULL;
    tmp_effect.image = NULL;
    
    root_effect_link.num = 0;
    root_effect_link.effect = 0;
    root_effect_link.duration = 0;
    last_effect_link = &root_effect_link;
    last_effect_link->next = NULL;

    strcpy( string_buffer, "effect 1,1" );
    current_mode = DEFINE_MODE;
    effectCommand();
    current_mode = NORMAL_MODE;
}

ScriptParser::~ScriptParser()
{
    printf("Deconstructor\n");
    int i;

    for ( i=0 ; i<num_of_labels ; i++ )
        delete[] label_info[i].name;
    if ( label_info ) delete[] label_info;
    if ( script_buffer ) delete[] script_buffer;
    delete[] string_buffer;
    delete[] tmp_string_buffer;
}

int ScriptParser::open()
{
    if ( readScript() ) return -1;
    if ( labelScript() ) return -1;
    
    label_stack_depth = 0;

    root_link_label_info.previous = NULL;
    root_link_label_info.next = NULL;
    root_link_label_info.current_line = 0;
    root_link_label_info.offset = 0;
    root_link_label_info.label_info = lookupLabel("define");
    current_mode = DEFINE_MODE;
    current_link_label_info = &root_link_label_info;

    last_tilde.label_info = lookupLabel( "start" );
    last_tilde.current_line = 0;
    last_tilde.offset = 0;
    
    return 0;
}

void ScriptParser::getSJISFromInteger( char *buffer, int no, bool add_space_flag )
{
    int c = 0;
    char num_str[] = "‚O‚P‚Q‚R‚S‚T‚U‚V‚W‚X";
    if ( no >= 10 ){
        buffer[c++] = num_str[ no / 10 % 10 * 2];
        buffer[c++] = num_str[ no / 10 % 10 * 2 + 1];
    }
    else if ( add_space_flag ){
        buffer[c++] = ((char*)"@")[0];
        buffer[c++] = ((char*)"@")[1];
    }
    buffer[c++] = num_str[ no % 10 * 2];
    buffer[c++] = num_str[ no % 10 * 2 + 1];
    buffer[c++] = '\0';
}

int ScriptParser::readScript()
{
    FILE *fp;
    char file_name[10];
    int  i, j, file_counter = 0, c;
    long len;
    bool encrypt_flag = false;
    char *p_script_buffer;
    
    script_buffer_length = 0;
    
    if ( (fp = fopen( "0.txt", "rb" )) != NULL ){
        do{
            fseek( fp, 0, SEEK_END );
            script_buffer_length += ftell( fp );
            sprintf( file_name, "%d.txt", ++file_counter );
            fclose( fp );
        }
        while( (fp = fopen( file_name, "rb" )) != NULL );
    }
    else if ( (fp = fopen( "nscript.dat", "rb" )) != NULL ){
        encrypt_flag = true;
        fseek( fp, 0, SEEK_END );
        script_buffer_length += ftell( fp );
        fclose( fp );
    }
    else{
        fprintf( stderr, "can't open file 0.txt or nscript.dat\n" );
        return -1;
    }
    
    if ( script_buffer ) delete[] script_buffer;
    if ( ( script_buffer = new char[ script_buffer_length ]) == NULL ){
        printf(" *** can't allocate memory for the script ***\n");
        exit( -1 );
    }
    p_script_buffer = script_buffer;
    
    if ( encrypt_flag ){
        fp = fopen( "nscript.dat", "rb" );
        fseek( fp, 0, SEEK_END );
        len = ftell( fp );
        fseek( fp, 0, SEEK_SET );
        while( len > 0 ){
            if ( len > READ_LENGTH ) c = READ_LENGTH;
            else                     c = len;
            len -= c;
            fread( p_script_buffer, 1, c, fp );
            for ( j=0 ; j<c ; j++ ) p_script_buffer[j] ^= 0x84;
            p_script_buffer += c;
        }
        fclose( fp );
    }
    else{
        for ( i=0 ; i<file_counter ; i++ ){
            sprintf( file_name, "%d.txt", i );
            fp = fopen( file_name, "rb" );
            fseek( fp, 0, SEEK_END );
            len = ftell( fp );
            fseek( fp, 0, SEEK_SET );
            while( len > 0 ){
                if ( len > READ_LENGTH ) c = READ_LENGTH;
                else                     c = len;
                len -= c;
                fread( p_script_buffer, 1, c, fp );
                p_script_buffer += c;
            }
            fclose( fp );
        }
    }
    
    return 0;
}

void ScriptParser::addStringBuffer( char ch, int string_counter )
{
    /* In case of string bufer over flow */
    if ( string_counter == string_buffer_length ){
        string_buffer_length += 512;
        char *tmp_buffer = new char[ string_buffer_length ];
        memcpy( tmp_buffer, string_buffer, string_counter );

        delete[] string_buffer;
        string_buffer = tmp_buffer;

        delete[] tmp_string_buffer;
        tmp_string_buffer = new char[ string_buffer_length ];
    }

    string_buffer[string_counter] = ch;
}

unsigned char ScriptParser::convHexToDec( char ch )
{
    if ( '0' <= ch && ch <= '9' ) return ch - '0';
    else if ( 'a' <= ch && ch <= 'f' ) return ch - 'a' + 10;
    else if ( 'A' <= ch && ch <= 'F' ) return ch - 'F' + 10;
    else return 0;
}

int ScriptParser::parseNumAlias( char **buf )
{
    char ch;
    char *end_point = script_buffer + script_buffer_length;
    int num_stack = 0;
    bool first_flag = true;
    bool num_alias_flag = true;
    char alias_buf[128];
    int  alias_buf_len = 0;
    int  alias_no = 0;

    while( 1 ){
        if ( *buf == end_point ) return -1;
        ch = **buf;
        if ( ch == '%' ){
            num_stack++;
            (*buf)++;
            continue;
        }
        if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ){
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            alias_buf[ alias_buf_len++ ] = ch;
        }
        else if ( ch >= '0' && ch <= '9' ){
            if ( first_flag ) num_alias_flag = false;
            if ( num_alias_flag ) alias_buf[ alias_buf_len++ ] = ch;
            else alias_no = alias_no * 10 + ch - '0';
        }
        else{
            break;
        }

        first_flag = false;
        (*buf)++;
    }

    /* ---------------------------------------- */
    /* Solve num aliases */
    if ( num_alias_flag ){
        alias_buf[ alias_buf_len ] = '\0';
        //printf(" alias_buf %s\n",alias_buf);
        //int len1, len2;
        //len1 = (int)strlen(alias_buf);
        struct NameAlias *p_name_alias = root_name_alias.next;

        while( p_name_alias ){
            //len2 = strlen( p_name_alias->alias );
            /* In case of constant */
            if ( !strcmp( p_name_alias->alias,
                          (const char*)alias_buf ) ){ //,
                //( len1 > len2 )? len1 : len2 ) ){
                alias_no = p_name_alias->num;
                break;
            }
            p_name_alias = p_name_alias->next;
        }
        if ( !p_name_alias ){
            printf("can't find name alias\n");
            exit(-1);
        }
    }

    /* ---------------------------------------- */
    /* Solve num aliases recursively */
    while ( num_stack-- ){
        //printf("alias_no = %d\n", alias_no );
        alias_no = num_variables[ alias_no ];
    }
    //printf("alias_no = %d\n", alias_no );
    return alias_no;
}

int ScriptParser::readLine( char **buf, bool raw_flag )
{
    char ch;
    int string_counter=0;
    char *end_point = script_buffer + script_buffer_length;
    bool head_flag = true;
    text_line_flag = true;
    char num_buf[10], num_sjis_buf[3];
    bool quat_flag = false, comment_flag = false;
    unsigned int i, no;
    
    while( 1 ){
        if ( *buf == end_point ) return -1;
        ch = *(*buf)++;
        
        if ( head_flag && (ch == ' ' || ch == '\t' || ch == ':' ) ){
            continue;
        }

        if ( ch == 0x0d ) continue;
        if ( ch == 0x0a ) break;

        if ( comment_flag ) continue;
        if ( raw_flag ){
            addStringBuffer( ch, string_counter++ );
            head_flag = false;
            continue;
        }

        /* Parser */
        if ( ch == ';' && head_flag ){
            comment_flag = true;
            addStringBuffer( ch, string_counter++ );
        }
        else if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ){
            if ( head_flag ){
                text_line_flag = false;
                head_flag = false;
            }
            if ( !quat_flag && ch >= 'A' && ch <= 'Z' ) ch += 'a' - 'A';
            addStringBuffer( ch, string_counter++ );
        }
        else if ( ch == '$' ){
            no = parseNumAlias( buf );
            if ( text_line_flag ){
                if ( str_variables[no] ){
                    for ( i=0 ; i<strlen( str_variables[no] ) ; i++ ){
                        addStringBuffer( str_variables[no][i], string_counter++ );
                    }
                }
            }
            else{
                addStringBuffer( '$', string_counter++ );
                sprintf( num_buf, "%d", no );
                for ( i=0 ; i<strlen( num_buf ) ; i++ ){
                    addStringBuffer( num_buf[i], string_counter++ );
                }
            }
        }
        else if ( ch == '%' ){
            no = parseNumAlias( buf );
            if ( text_line_flag ){
                if ( num_variables[no]<0 ){
                    addStringBuffer( "|"[0], string_counter++ );
                    addStringBuffer( "|"[1], string_counter++ );
                    sprintf( num_buf, "%d", -num_variables[no] );
                }
                else{
                    sprintf( num_buf, "%d", num_variables[no] );
                }
                for ( i=0 ; i<strlen( num_buf ) ; i++ ){
                    getSJISFromInteger( num_sjis_buf, num_buf[i] - '0', false );
                    addStringBuffer( num_sjis_buf[0], string_counter++ );
                    addStringBuffer( num_sjis_buf[1], string_counter++ );
                }
            }
            else{
                addStringBuffer( '%', string_counter++ );
                sprintf( num_buf, "%d", no );
                for ( i=0 ; i<strlen( num_buf ) ; i++ ){
                    addStringBuffer( num_buf[i], string_counter++ );
                }
            }
        }
        else if ( ch == '"' ){
            quat_flag = !quat_flag;
            addStringBuffer( ch, string_counter++ );
        }
        else if ( ch & 0x80 ){
            addStringBuffer( ch, string_counter++ );
            ch = *(*buf)++;
            addStringBuffer( ch, string_counter++ );
        }
        else{
            addStringBuffer( ch, string_counter++ );
        }

        head_flag = false;
    }

    addStringBuffer( '\0', string_counter++ );

    string_buffer_offset = 0;
    while ( string_buffer[ string_counter ] == ' ' || string_buffer[ string_counter ] == '\t' )
        string_counter++;
    //if ( !raw_flag )
    //printf("end of readLine %s\n",string_buffer );

    return 0;
}

const char* ScriptParser::getVersion()
{
    return (const char*)version_str;
}

int ScriptParser::findLabel( const char *label )
{
    int i;
    char *capital_label = new char[ strlen( label ) + 1 ];

    for ( i=0 ; i<(int)strlen( label )+1 ; i++ ){
        capital_label[i] = label[i];
        if ( 'a' <= capital_label[i] && capital_label[i] <= 'z' ) capital_label[i] += 'A' - 'a';
    }

    for ( i=0 ; i<num_of_labels ; i++ )
        if ( !strcmp( label_info[i].name, capital_label ) ){
            delete[] capital_label;
            return i;
        }

    delete[] capital_label;
    return -1;
}

int ScriptParser::getNumLabelAccessed()
{
    return num_of_label_accessed;
}

bool ScriptParser::getLabelAccessFlag( const char *label )
{
    int i = findLabel( label );
    if ( i >= 0 ){
        return label_info[i].access_flag;
    }

    return false;
}

struct ScriptParser::LabelInfo ScriptParser::lookupLabel( const char *label )
{
    struct LabelInfo ret;

    int i = findLabel( label );
    if ( i >= 0 ){
        if ( !label_info[i].access_flag ) num_of_label_accessed++;
        label_info[i].access_flag = true;
        return label_info[i];
    }

    ret.start_address = NULL;
    return ret;
}

struct ScriptParser::LabelInfo ScriptParser::lookupLabelNext( const char *label )
{
    struct LabelInfo ret;

    int i = findLabel( label );
    if ( i >= 0 && i+1 < num_of_labels ){
        if ( !label_info[i+1].access_flag ) num_of_label_accessed++;
        label_info[i+1].access_flag = true;
        return label_info[i+1];
    }

    ret.start_address = NULL;
    return ret;
}

int ScriptParser::labelScript()
{
    char *p_script_buffer = script_buffer, *p_script_buffer_old, *p_start_buffer = NULL;
    char *p_string_buffer = NULL;
    bool first_flag = true;
    int  label_counter = -1;
    
    num_of_labels = 0;

    p_script_buffer_old = p_script_buffer;
    while( !readLine( &p_script_buffer, true ) ){
        if ( string_buffer[0] == '*' ){
            if ( first_flag ){
                first_flag = false;
                p_start_buffer = p_script_buffer_old;
            }
            num_of_labels++;
        }
        p_script_buffer_old = p_script_buffer;
    }

    label_info = new struct LabelInfo[ num_of_labels ];

    p_script_buffer = p_start_buffer;
    while( !readLine( &p_script_buffer, true ) ){
        if ( string_buffer[0] == '*' ){
            p_string_buffer = string_buffer + 1;
            readToken( &p_string_buffer, tmp_string_buffer );
            label_info[++label_counter].name = new char[ strlen(tmp_string_buffer) + 1 ];
            for ( unsigned int i=0 ; i<strlen(tmp_string_buffer) + 1 ; i++ ){
                label_info[label_counter].name[i] = tmp_string_buffer[i];
                if ( 'a' <= label_info[label_counter].name[i] && label_info[label_counter].name[i] <= 'z' )
                    label_info[label_counter].name[i] += 'A' - 'a';
            }
            label_info[label_counter].start_address = p_script_buffer;
            label_info[label_counter].num_of_lines = 0;
            label_info[label_counter].access_flag = false;
        }
        else{
            label_info[label_counter].num_of_lines++;
        }
    }
#if 0
    for ( int i=0 ; i<num_of_labels ; i++ ){
        printf("name [%s] %p %d\n", label_info[i].name, label_info[i].start_address - script_buffer, label_info[i].num_of_lines );
    }
    printf("num_of_labels = %d %d\n", num_of_labels, script_buffer_length );
#endif

    return 0;
}

int ScriptParser::parseLine()
{
    int ret = RET_NOMATCH, lut_counter = 0;
    unsigned int command_len;

    while( string_buffer[ string_buffer_offset ] == ' ' ||
           string_buffer[ string_buffer_offset ] == '\t' ) string_buffer_offset++;
    if ( string_buffer[ string_buffer_offset ] == '\0' ) return RET_COMMENT;
    else if ( string_buffer[ string_buffer_offset ] & 0x80 ) return RET_NOMATCH;

    char *p_string_buffer = string_buffer + string_buffer_offset;
    readToken( &p_string_buffer, tmp_string_buffer );
    command_len = strlen( tmp_string_buffer );

    while( func_lut[ lut_counter ].method ){
        if ( !strcmp( func_lut[ lut_counter ].command, tmp_string_buffer ) ){
            ret = (this->*func_lut[ lut_counter ].method)();

            if ( ret == RET_CONTINUE || ret == RET_WAIT_NEXT ){
                skipToken();
            }
            break;
        }
        lut_counter++;
    }

    if ( func_lut[ lut_counter ].method == 0 ){
        //printf("[%s] is not recognized by ScriptParser\n",&string_buffer[string_buffer_offset]);
        return RET_NOMATCH;
    }

    return ret;
}


bool ScriptParser::readToken( char **src_buf, char *dst_buf, bool comma_check_flag )
{
    char *dst_buf_org = dst_buf;
    bool quat_flag = false;
    bool end_with_comma_flag = false;
    
    /* If it reaces to the end of the buffer, just return. */
    if ( *src_buf >= string_buffer + strlen(string_buffer) ){
        dst_buf[0] = '\0';
        return end_with_comma_flag;
    }

    /* ---------------------------------------- */
    /* Obatin a single word */
    while ( **src_buf == ' ' || **src_buf == '\t' ) (*src_buf)++;

    //printf("token start %c:\n", **src_buf);
    if ( **src_buf == '"' ){
        quat_flag = true;
        *dst_buf++ = *(*src_buf)++;
    }
    while ( **src_buf != '\0' &&
            **src_buf != '"' &&
            (quat_flag || !comma_check_flag || **src_buf != ',' ) &&
            (quat_flag || ( **src_buf != ':' &&
                            **src_buf != ';' &&
                            **src_buf != '<' &&
                            **src_buf != '>' &&
                            **src_buf != '=' &&
                            **src_buf != '!' &&
                            **src_buf != '&' &&
                            **src_buf != ' ' &&
                            **src_buf != '\t' &&
                            !(**src_buf & 0x80) ) ) ){
        
        if ( **src_buf & 0x80 ) {
            *dst_buf++ = *(*src_buf)++;
            *dst_buf++ = *(*src_buf)++;
        }
        else{
            *dst_buf = *(*src_buf)++;
            dst_buf++;
        }
    }

    if ( quat_flag && **src_buf == '"' ) *dst_buf++ = *(*src_buf)++;

    while ( **src_buf == ' ' || **src_buf == '\t' ) (*src_buf)++;
    if ( comma_check_flag && **src_buf == ',' ){
        (*src_buf)++;
        end_with_comma_flag = true;
    }
    *dst_buf++ = '\0';
    //printf("dst_buf %s\n",dst_buf_org);
    
    /* ---------------------------------------- */
    /* Strip dobule quat */
    if ( dst_buf_org[0] == '"' ){
        //printf("strip %s\n",dst_buf_org);
        
        int length = strlen( dst_buf_org ) - 2;
        memmove ( dst_buf_org, dst_buf_org+1, length );
        dst_buf_org[ length ] = '\0';
    }
    return end_with_comma_flag;
}

int ScriptParser::readInt( char **src_buf, char *dst_buf, bool comma_check_flag )
{
    readToken( src_buf, dst_buf, comma_check_flag );
    //printf("readInt %s\n",dst_buf);
    if ( dst_buf[0] == '%' ) return num_variables[atoi( dst_buf+1 )];
    else{
        /* ---------------------------------------- */
        /* Solve num aliases */
        //int len1, len2;
        //len1 = (int)strlen(alias_buf);
        struct NameAlias *p_name_alias = root_name_alias.next;

        while( p_name_alias ){
            //len2 = strlen( p_name_alias->alias );
            /* In case of constant */
            if ( !strcmp( p_name_alias->alias,
                           (const char*)dst_buf ) ){//,
                //( len1 > len2 )? len1 : len2 ) ){
                return p_name_alias->num;
                break;
            }
            p_name_alias = p_name_alias->next;
        }
    }
    
    return atoi( dst_buf );
}

bool ScriptParser::readStr( char **src_buf, char *dst_buf, bool comma_check_flag )
{
    bool ret = readToken( src_buf, dst_buf, comma_check_flag );
    if ( dst_buf[0] == '$' )
        sprintf( dst_buf, "%s", str_variables[atoi( dst_buf+1 )] );
    else{
        /* ---------------------------------------- */
        /* Solve str aliases */
        struct StringAlias *p_str_alias = root_str_alias.next;

        while( p_str_alias ){
            if ( !strcmp( p_str_alias->alias, (const char*)dst_buf ) ){
                memcpy( dst_buf, p_str_alias->str, strlen( p_str_alias->str ) + 1 );
                break;
            }
            p_str_alias = p_str_alias->next;
        }
    }
    return ret;
}

void ScriptParser::skipToken( void )
{
    bool quat_flag = false;

    while(1){
        if ( string_buffer[ string_buffer_offset ] == '\0' ||
             ( !quat_flag && string_buffer[ string_buffer_offset ] == ':' )) break;
        if ( string_buffer[ string_buffer_offset ] == '"' )
            quat_flag = !quat_flag;
        string_buffer_offset++;
    }
    if ( string_buffer[ string_buffer_offset ] == ':' ){
        string_buffer_offset++;
        while( string_buffer[ string_buffer_offset ] == ' ' || string_buffer[ string_buffer_offset ] == '\t' ) string_buffer_offset++;
    }
}

struct ScriptParser::EffectLink ScriptParser::getEffect( int effect_no )
{
    if ( effect_no == WINDOW_EFFECT )    return window_effect;
    else if (effect_no == PRINT_EFFECT ) return print_effect;
    else if (effect_no == TMP_EFFECT )   return tmp_effect;
    
    struct EffectLink *p_effect_link = &root_effect_link;
    while( p_effect_link ){
        if ( p_effect_link->num == effect_no ) return *p_effect_link;
        p_effect_link = p_effect_link->next;
    }

    fprintf( stderr, "no effect was found [%d]\n", effect_no );
    exit(-1);
}

int ScriptParser::getSystemCallNo( char *buffer )
{
    if ( !strcmp( buffer, "skip" ) )             return SYSTEM_SKIP;
    else if ( !strcmp( buffer, "reset" ) )       return SYSTEM_RESET;
    else if ( !strcmp( buffer, "save" ) )        return SYSTEM_SAVE;
    else if ( !strcmp( buffer, "load" ) )        return SYSTEM_LOAD;
    else if ( !strcmp( buffer, "lookback" ) )    return SYSTEM_LOOKBACK;
    else if ( !strcmp( buffer, "windowerase" ) ) return SYSTEM_WINDOWERASE;
    else return -1;
}

void ScriptParser::saveGlovalData()
{
    if ( !globalon_flag ) return;
    printf(" saveGlobalData\n" );

    FILE *fp;

    if ( ( fp = fopen( "global.sav", "wb" ) ) == NULL ){
        fprintf( stderr, "can't write global.sav\n" );
        exit( -1 );
    }

    saveVariables( fp, 200, VARIABLE_RANGE );
    fclose( fp );
}

void ScriptParser::saveFileLog()
{
    if ( !filelog_flag ) return;
    printf(" saveFileLog\n" );

    FILE *fp;
    int  i,j;
    char buf[10];

    if ( ( fp = fopen( "NScrflog.dat", "wb" ) ) == NULL ){
        fprintf( stderr, "can't write NScrflog.dat\n" );
        exit( -1 );
    }

    sprintf( buf, "%d", cBR->getNumAccessed() );
    for ( i=0 ; i<(int)strlen( buf ) ; i++ ) fputc( buf[i], fp );
    fputc( 0x0a, fp );
    
    SarReader::FileInfo fi;
    for ( i=0 ; i<cBR->getNumFiles() ; i++ ){
        fi = cBR->getFileByIndex( i );
        if ( fi.access_flag ){
            fputc( '"', fp );
            for ( j=0 ; j<(int)strlen( fi.name ) ; j++ )
                fputc( fi.name[j] ^ 0x84, fp );
            fputc( '"', fp );
        }
    }
    
    fclose( fp );
}

void ScriptParser::saveLabelLog()
{
    if ( !labellog_flag ) return;
    printf(" saveLabelLog\n" );

    FILE *fp;
    int  i;
    char buf[10];

    if ( ( fp = fopen( "NScrllog.dat", "wb" ) ) == NULL ){
        fprintf( stderr, "can't write NScrllog.dat\n" );
        exit( -1 );
    }

    sprintf( buf, "%d", getNumLabelAccessed() );
    for ( i=0 ; i<(int)strlen( buf ) ; i++ ) fputc( buf[i], fp );
    fputc( 0x0a, fp );
    
    for ( i=0 ; i<num_of_labels ; i++ ){
        if ( label_info[i].access_flag ){
            fputc( '"', fp );
            for ( unsigned j=0 ; j<strlen( label_info[i].name ) ; j++ )
                fputc( label_info[i].name[j] ^ 0x84, fp );
            fputc( '"', fp );
        }
    }
    
    fclose( fp );
}

void ScriptParser::saveInt( FILE *fp, int var )
{
    char tmp_int[4];
    
    tmp_int[0] = var & 0xff;
    tmp_int[1] = (var >> 8) & 0xff;
    tmp_int[2] = (var >> 16) & 0xff;
    tmp_int[3] = (var >> 24) & 0xff;
        
    fwrite( tmp_int, 1, 4, fp );
}

void ScriptParser::loadInt( FILE *fp, int *var )
{
    unsigned char tmp_int[4];
    
    fread( tmp_int, 1, 4, fp );
    *var =
        (unsigned int)tmp_int[3] << 24 |
        (unsigned int)tmp_int[2] << 16 |
        (unsigned int)tmp_int[1] << 8 |
        (unsigned int)tmp_int[0];
}

void ScriptParser::saveStr( FILE *fp, char *str )
{
    if ( str && str[0] ) fprintf( fp, "%s", str );
    fputc( 0, fp );
}

#define INITIAL_LOAD_STR 128
void ScriptParser::loadStr( FILE *fp, char **str )
{
    int counter = 0, counter_max = INITIAL_LOAD_STR;
    char tmp_buffer[INITIAL_LOAD_STR], *p_buffer = tmp_buffer, *p_buffer2;
    
    while( (p_buffer[ counter++ ] = fgetc( fp )) != '\0' ){
        if ( counter >= counter_max ){
            p_buffer2 = p_buffer;
            p_buffer = new char[ counter_max + INITIAL_LOAD_STR ];
            memcpy( p_buffer, p_buffer2, counter_max );
            if ( counter_max != INITIAL_LOAD_STR ) delete[] p_buffer2;
            counter_max += INITIAL_LOAD_STR;
        }
    }
    if ( *str ) delete[] *str;
    if ( counter == 1 ){
        *str = NULL;
    }
    else{
        *str = new char[ counter ];
        memcpy( *str, p_buffer, counter );
        if ( counter_max != INITIAL_LOAD_STR ) delete[] p_buffer;
    }
}

void ScriptParser::saveVariables( FILE *fp, int from, int to )
{
    for ( int i=from ; i<to ; i++ ){
        saveInt( fp, num_variables[i] );
        saveStr( fp, str_variables[i] );
    }
}

void ScriptParser::loadVariables( FILE *fp, int from, int to )
{
    for ( int i=from ; i<to ; i++ ){
        loadInt( fp, &num_variables[i] );
        loadStr( fp, &str_variables[i] );
        if ( str_variables[i] == NULL ){
            str_variables[i] = new char[1];
            str_variables[i][0] = '\0';
        }
    }
}

void ScriptParser::errorAndExit( char *str )
{
    fprintf( stderr, " *** Invalid command %s ***\n", str );
    exit(-1);
}
