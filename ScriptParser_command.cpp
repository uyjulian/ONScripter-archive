/* -*- C++ -*-
 *
 *  ScriptParser_command.cpp - Define command executer of ONScripter
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

int ScriptParser::underlineCommand()
{
    assert( current_mode == DEFINE_MODE );
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("underline") = 9
    underline_value = readInt( &p_string_buffer, tmp_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::versionstrCommand()
{
    char *p_string_buffer, *tmp_buffer;
    int  length[2];
    
    delete[] version_str;
    
    p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("versionstr") = 10

    printf("versionstr %s\n",p_string_buffer);
    
    readStr( &p_string_buffer, tmp_string_buffer );
    length[0] = strlen(tmp_string_buffer) + strlen("\n");
    tmp_buffer = new char[ length[0] ];
    memcpy( tmp_buffer, tmp_string_buffer, length[0] );

    readStr( &p_string_buffer, tmp_string_buffer );
    length[1] = strlen(tmp_string_buffer) + strlen("\n");
    version_str = new char[ length[0] + length[1] + 1 ];
    sprintf( version_str, "%s\n%s\n", tmp_buffer, tmp_string_buffer );

    delete[] tmp_buffer;
    
    return RET_CONTINUE;
}

int ScriptParser::transmodeCommand()
{
    assert( current_mode == DEFINE_MODE );
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("transmode") = 9

    readStr( &p_string_buffer, tmp_string_buffer );

    if ( !strcmp( tmp_string_buffer, "leftup" ) ){
        trans_mode = TRANS_TOPLEFT;
    }
    else if ( !strcmp( tmp_string_buffer, "copy" ) ){
        trans_mode = TRANS_COPY;
    }
    else if ( !strcmp( tmp_string_buffer, "alpha" ) ){
        trans_mode = TRANS_ALPHA;
    }
    return RET_CONTINUE;
}

int ScriptParser::timeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("time") = 4
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_hour );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_min );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_sec );

    return RET_CONTINUE;
}

int ScriptParser::subCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("sub") = 3
    int no;
    
    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] - readInt( &p_string_buffer, tmp_string_buffer ) );
    

    return RET_CONTINUE;
}

int ScriptParser::straliasCommand()
{
    assert( current_mode == DEFINE_MODE );
    
    StringAlias *p_str_alias = new StringAlias();
    char *p_string_buffer;
    
    p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("stralias") = 8
    readStr( &p_string_buffer, tmp_string_buffer );
    p_str_alias->alias = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_str_alias->alias, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    p_str_alias->str = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_str_alias->str, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    last_str_alias->next = p_str_alias;
    last_str_alias = last_str_alias->next;
    last_str_alias->next = NULL;

    //printf("straliasCommand [%s] [%s]\n", last_str_alias->alias, last_str_alias->str );

    return RET_CONTINUE;
}

int ScriptParser::skipCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("skip") = 4
    int skip_num = readInt( &p_string_buffer, tmp_string_buffer );
    int skip_offset = current_link_label_info->current_line + skip_num;
    //printf("skipCommand %d\n", skip_num );

    while ( skip_offset >= current_link_label_info->label_info.num_of_lines ){ 
        skip_offset -= current_link_label_info->label_info.num_of_lines;
        current_link_label_info->label_info = lookupLabelNext( current_link_label_info->label_info.name );
    }
    current_link_label_info->current_line = skip_offset;
    current_link_label_info->offset = 0;

    return RET_JUMP;
}

int ScriptParser::selectcolorCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("selectcolor") = 11

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    sentence_font.on_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    sentence_font.on_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    sentence_font.on_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    sentence_font.off_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    sentence_font.off_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    sentence_font.off_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
    
    return RET_CONTINUE;
}

int ScriptParser::savenumberCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("savenumber") = 10

    num_save_file = readInt( &p_string_buffer, tmp_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::savenameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("savename") = 8

    readStr( &p_string_buffer, tmp_string_buffer );
    delete[] save_menu_name;
    save_menu_name = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( save_menu_name, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    readStr( &p_string_buffer, tmp_string_buffer );
    delete[] load_menu_name;
    load_menu_name = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( load_menu_name, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    readStr( &p_string_buffer, tmp_string_buffer );
    delete[] save_item_name;
    save_item_name = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( save_item_name, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    return RET_CONTINUE;
}

int ScriptParser::rmenuCommand()
{
    MenuLink *menu;
    
    /* ---------------------------------------- */
    /* Delete old MenuLink */
    last_menu_link = root_menu_link.next;
    while( last_menu_link ){
        menu = last_menu_link->next;
        if ( last_menu_link->label ) delete[] last_menu_link->label;
        delete last_menu_link;
        last_menu_link = menu;
    }
    last_menu_link = &root_menu_link;
    menu_link_num = 0;
    menu_link_width = 0;
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("rmode") = 5

    while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;

    while ( *p_string_buffer ){
        MenuLink *menu = new MenuLink();

        readStr( &p_string_buffer, tmp_string_buffer );
        menu->label = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( menu->label, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        if ( menu_link_width < strlen( tmp_string_buffer ) / 2 ) menu_link_width = strlen( tmp_string_buffer ) / 2;

        readStr( &p_string_buffer, tmp_string_buffer );
        menu->system_call_no = getSystemCallNo( tmp_string_buffer );

        menu->next = NULL;
        last_menu_link->next = menu;
        last_menu_link = menu;
        menu_link_num++;
    }
    
    return RET_CONTINUE;
}

int ScriptParser::returnCommand()
{
    //printf(" returnCommand (%d>%d)\n", stack_depth, stack_depth-1);
    label_stack_depth--;
    
    current_link_label_info = current_link_label_info->previous;
    delete current_link_label_info->next;
    current_link_label_info->next = NULL;

    return RET_JUMP;
}

int ScriptParser::numaliasCommand()
{
    assert( current_mode == DEFINE_MODE );

    NameAlias *p_name_alias = new NameAlias();
    char *p_string_buffer;
    
    p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("numalias") = 8
    readStr( &p_string_buffer, tmp_string_buffer );
    p_name_alias->alias = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_name_alias->alias, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    p_name_alias->num = readInt( &p_string_buffer, tmp_string_buffer );

    last_name_alias->next = p_name_alias;
    last_name_alias = last_name_alias->next;
    last_name_alias->next = NULL;
    
    //printf("numaliasCommand [%s] [%d]\n", last_name_alias->alias, last_name_alias->num );

    return RET_CONTINUE;
}

int ScriptParser::nextCommand()
{
    printf("nextCommand %d( %d -> %d)\n", for_stack_depth, num_variables[ current_for_link->variable_no ], num_variables[ current_for_link->variable_no ] + current_for_link->step );

    setNumVariable( current_for_link->variable_no, num_variables[ current_for_link->variable_no ] + current_for_link->step );
    if ( current_for_link->step >= 0 && num_variables[ current_for_link->variable_no ] > current_for_link->to ||
         current_for_link->step < 0 && num_variables[ current_for_link->variable_no ] < current_for_link->to ){
        for_stack_depth--;
        current_for_link = current_for_link->previous;
        delete current_for_link->next;
        current_for_link->next = NULL;

        return RET_CONTINUE;
    }
    else{
        current_link_label_info->label_info = current_for_link->label_info;
        current_link_label_info->current_line = current_for_link->current_line;
        current_link_label_info->offset = current_for_link->offset;
        
        return RET_JUMP;
    }
}

int ScriptParser::mulCommand()
{
    int no;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mul") = 3

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] * readInt( &p_string_buffer, tmp_string_buffer ) );

    return RET_CONTINUE;
}

int ScriptParser::movCommand()
{
    int no;
    bool num_flag;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mov") = 3

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' && tmp_string_buffer[0] != '$' ) errorAndExit( string_buffer + string_buffer_offset );

    if ( tmp_string_buffer[0] == '%' ) num_flag = true;
    else                               num_flag = false;

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );
    
    if ( num_flag ){
        setNumVariable( no, readInt( &p_string_buffer, tmp_string_buffer ) );
    }
    else{
        readStr( &p_string_buffer, tmp_string_buffer );
        delete[] str_variables[ no ];
        str_variables[ no ] = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( str_variables[ no ], tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    }
    
    return RET_CONTINUE;
}

int ScriptParser::modCommand()
{
    int no;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mod") = 3

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] % readInt( &p_string_buffer, tmp_string_buffer ) );

    return RET_CONTINUE;
}

int ScriptParser::menusetwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 13; // strlen("menusetwindow") = 13
    char mask[5] = "0x00";

    menu_font.font_valid_flag = false;
    menu_font.font_size = readInt( &p_string_buffer, tmp_string_buffer );
    //if ( menu_font.font_size < 18 ) menu_font.font_size = 18; // work aroud for embedded bitmaps
    readInt( &p_string_buffer, tmp_string_buffer ); // Ignore font size along Y axis
    menu_font.pitch_xy[0] = readInt( &p_string_buffer, tmp_string_buffer ) + menu_font.font_size;
    menu_font.pitch_xy[1] = readInt( &p_string_buffer, tmp_string_buffer ) + menu_font.font_size;
    menu_font.display_bold = readInt( &p_string_buffer, tmp_string_buffer )?true:false;
    menu_font.display_shadow = readInt( &p_string_buffer, tmp_string_buffer )?true:false;

    //menu_font.font.setPixelSize( menu_font.font_size );
    //menu_font.top_xy[1] += menu_font.font_size;
#if 0
    printf("ONScripterLabel::menusetwindowCommand font=%d (%d,%d) bold=%d, shadow=%d\n",
           menu_font.font_size, menu_font.pitch_xy[0], menu_font.pitch_xy[1],
           menu_font.display_bold, menu_font.display_shadow );
#endif
    readStr( &p_string_buffer, tmp_string_buffer );

    menu_font.display_transparency = true;
    assert( strlen( tmp_string_buffer ) == 7 );
    memcpy( menu_font.window_color, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    mask[2] = menu_font.window_color[1]; mask[3] = menu_font.window_color[2];
    sscanf( mask, "%i", &menu_font.window_color_mask[0] );
    mask[2] = menu_font.window_color[3]; mask[3] = menu_font.window_color[4];
    sscanf( mask, "%i", &menu_font.window_color_mask[1] );
    mask[2] = menu_font.window_color[5]; mask[3] = menu_font.window_color[6];
    sscanf( mask, "%i", &menu_font.window_color_mask[2] );
#if 0
    printf("    trans %s\n",
           menu_font.window_color );
#endif
    return RET_CONTINUE;
}

int ScriptParser::menuselectcolorCommand()
{
    int i;
    char *p_string_buffer = string_buffer + string_buffer_offset + 15; // strlen("menuselectcolor") = 15

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    menu_font.on_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    menu_font.on_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    menu_font.on_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
    for ( i=0 ; i<3 ; i++ ) system_font.on_color[i] = menu_font.on_color[i];

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    menu_font.off_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    menu_font.off_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    menu_font.off_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
    for ( i=0 ; i<3 ; i++ ) system_font.off_color[i] = menu_font.off_color[i];
    
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    menu_font.nofile_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    menu_font.nofile_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    menu_font.nofile_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
    for ( i=0 ; i<3 ; i++ ) system_font.nofile_color[i] = menu_font.nofile_color[i];
    
    return RET_CONTINUE;
}

int ScriptParser::lookbackcolorCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 13; // strlen("lookbackcolor") = 13

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );

    lookback_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
    lookback_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
    lookback_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );

    return RET_CONTINUE;
}

int ScriptParser::lookbackbuttonCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 14; // strlen("lookbackbutton") = 14

    for ( int i=0 ; i<4 ; i++ ){
        readStr( &p_string_buffer, tmp_string_buffer );
        delete lookback_image_name[i];
        lookback_image_name[i] = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( lookback_image_name[i], tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    }
    return RET_CONTINUE;
}

int ScriptParser::labellogCommand()
{
    printf(" labellogCommand\n" );
    FILE *fp;
    int i, j, ch, count = 0;
    char buf[100];
    
    if ( ( fp = fopen( "NScrllog.dat", "rb" ) ) != NULL ){
        //printf("read from NScrllog.dat\n");
        while( (ch = fgetc( fp )) != 0x0a ){
            count = count * 10 + ch - '0';
        }
        //printf("count %d\n",count);

        for ( i=0 ; i<count ; i++ ){
            fgetc( fp );
            j = 0;
            while( (ch = fgetc( fp )) != '"' ) buf[j++] = ch ^ 0x84;
            buf[j] = '\0';
            lookupLabel( buf );
        }
        
        fclose( fp );
    }
    labellog_flag = true;
    return RET_CONTINUE;
}

int ScriptParser::ifCommand()
{
    char *p_string_buffer;
    int left_value, right_value;
    bool if_flag, condition_flag = true, f;
    char eval_str[3], *token_start;

    if ( string_buffer[0] == 'n' ){
        if_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("notif") = 5
        //printf("notifCommand %s\n", p_string_buffer );
    }
    else{
        if_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 2; // strlen("if") = 2
        //printf("ifCommand %s\n", p_string_buffer );
    }
    //readStr( &p_string_buffer, tmp_string_buffer, false );

    while(1){
        while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;

        if ( *p_string_buffer == 'f' ){ // fchk
            readStr( &p_string_buffer, tmp_string_buffer );
            readStr( &p_string_buffer, tmp_string_buffer );
            f = cBR->getAccessFlag( tmp_string_buffer );
            //printf("fchk %s(%d,%d) ", tmp_string_buffer, cBR->getAccessFlag( tmp_string_buffer ), condition_flag );
        }
        else if ( *p_string_buffer == 'l' ){ // lchk
            readStr( &p_string_buffer, tmp_string_buffer );
            readStr( &p_string_buffer, tmp_string_buffer );
            f = getLabelAccessFlag( tmp_string_buffer+1 );
            //printf("lchk %s(%d,%d) ", tmp_string_buffer, getLabelAccessFlag( tmp_string_buffer+1 ), condition_flag );
        }
        else{
            left_value = readInt( &p_string_buffer, tmp_string_buffer );
            //printf("%s(%d) ", tmp_string_buffer, left_value );

            while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;
            token_start = p_string_buffer;
            while ( *p_string_buffer == '>' || *p_string_buffer == '<' ||
                    *p_string_buffer == '=' || *p_string_buffer == '!' ) p_string_buffer++;
            memcpy( eval_str, token_start, p_string_buffer-token_start );
            eval_str[ p_string_buffer-token_start ] = '\0';
            //printf("%s ", eval_str );

            while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;

            right_value = readInt( &p_string_buffer, tmp_string_buffer );
            //printf("%s(%d) ", tmp_string_buffer, right_value );

            if ( !strcmp( eval_str, ">=" ) )      f = (left_value >= right_value);
            else if ( !strcmp( eval_str, "<=" ) ) f = (left_value <= right_value);
            else if ( !strcmp( eval_str, "==" ) ) f = (left_value == right_value);
            else if ( !strcmp( eval_str, "!=" ) ) f = (left_value != right_value);
            else if ( !strcmp( eval_str, "<>" ) ) f = (left_value != right_value);
            else if ( !strcmp( eval_str, "<" ) )  f = (left_value < right_value);
            else if ( !strcmp( eval_str, ">" ) )  f = (left_value > right_value);
            else if ( !strcmp( eval_str, "=" ) )  f = (left_value == right_value);
        }
        condition_flag &= (if_flag)?(f):(!f);
        while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;

        if ( *p_string_buffer == '&' ){
            //printf("& " );
            while ( *p_string_buffer == '&' ) p_string_buffer++;
            continue;
        }
        else break;
    };

    /* Execute command */
    if ( condition_flag ){
        //printf(" = true (%d, %d)\n", if_flag, condition_flag );
        string_buffer_offset = p_string_buffer - string_buffer;
        return RET_CONTINUE_NONEXT;
    }
    else{
        //printf(" = false\n");
        while ( *p_string_buffer != '\0' ) p_string_buffer++;
        string_buffer_offset = p_string_buffer - string_buffer;
        return RET_CONTINUE;
    }
}

int ScriptParser::itoaCommand()
{
    int no, num;
    char num_str[20], *tmp_buffer;
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("itoa") = 4
    
    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '$' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    num = readInt( &p_string_buffer, tmp_string_buffer );

    tmp_buffer = str_variables[ no ];
    sprintf( num_str, "%d", num );
    str_variables[ no ] = new char[ strlen( num_str ) + 1 ];
    memcpy( str_variables[ no ], num_str, strlen( num_str ) + 1 );
    delete[] tmp_buffer;
    
    return RET_CONTINUE;
}

int ScriptParser::intlimitCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("intlimit") = 8
    int no = readInt( &p_string_buffer, tmp_string_buffer );

    num_limit_flag[ no ] = true;
    num_limit_lower[ no ] = readInt( &p_string_buffer, tmp_string_buffer );
    num_limit_upper[ no ] = readInt( &p_string_buffer, tmp_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::incCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("inc") = 3
    int no;

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    
    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] + 1 );

    return RET_CONTINUE;
}

int ScriptParser::humanzCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("humanz") = 6
    z_order = readInt( &p_string_buffer, tmp_string_buffer );
    
    return RET_CONTINUE;
}

int ScriptParser::gotoCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("goto") = 4
    readStr( &p_string_buffer, tmp_string_buffer );
    //printf(" gotoCommand %s\n", tmp_string_buffer );

    current_link_label_info->label_info = lookupLabel( tmp_string_buffer + 1 );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;
    
    return RET_JUMP;
}

void ScriptParser::gosubReal( char *label )
{
    label_stack_depth++;
    
    LinkLabelInfo *info = new LinkLabelInfo();
    info->previous = current_link_label_info;
    info->next = NULL;
    info->label_info = lookupLabel( label );
    info->current_line = 0;
    info->offset = 0;

    current_link_label_info->next = info;

    /* ---------------------------------------- */
    /* Step forward callee's label info */
    if ( string_buffer[ string_buffer_offset ] == '\0' ){
        current_link_label_info->current_line++;
        current_link_label_info->offset = 0;
    }
    else
        current_link_label_info->offset = string_buffer_offset;

    current_link_label_info = current_link_label_info->next;
}

int ScriptParser::gosubCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("gosub") = 5
    readStr( &p_string_buffer, tmp_string_buffer );
    //printf(" gosubCommand (%d > %d) %s\n", label_stack_depth, label_stack_depth+1, tmp_string_buffer );
    skipToken();
    gosubReal( tmp_string_buffer + 1 );

    return RET_JUMP;
}

int ScriptParser::globalonCommand()
{
    printf(" globalonCommand\n" );
    FILE *fp;

    if ( ( fp = fopen( "global.sav", "rb" ) ) != NULL ){
        printf("read from global.sav\n");
        loadVariables( fp, 200, VARIABLE_RANGE );
        fclose( fp );
    }
    globalon_flag = true;
    return RET_CONTINUE;
}

int ScriptParser::forCommand()
{
    for_stack_depth++;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("for") = 3
    ForInfo *info = new ForInfo();
    info->next = NULL;
    
    readToken( &p_string_buffer, tmp_string_buffer );
    info->variable_no = atoi( tmp_string_buffer + 1 );
    
    if ( *p_string_buffer != '=' ) errorAndExit( string_buffer + string_buffer_offset );
    p_string_buffer++;

    setNumVariable( info->variable_no, readInt( &p_string_buffer, tmp_string_buffer ) );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( strcmp( tmp_string_buffer, "to" ) ) errorAndExit( string_buffer + string_buffer_offset );
    
    info->to = readInt( &p_string_buffer, tmp_string_buffer );

    while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;
    if ( !strncmp( p_string_buffer, "step", 4 ) ){
        readStr( &p_string_buffer, tmp_string_buffer );
        info->step = readInt( &p_string_buffer, tmp_string_buffer );
    }
    else{
        info->step = 1;
    }
    
    /* ---------------------------------------- */
    /* Step forward callee's label info */
    while ( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;
    info->label_info = current_link_label_info->label_info;
    if ( *p_string_buffer == ':' ){
        info->current_line = current_link_label_info->current_line;
        info->offset = p_string_buffer + 1 - string_buffer;
    }
    else{
        info->current_line = current_link_label_info->current_line + 1;
        info->offset = 0;
    }

    info->previous = current_for_link;
    current_for_link->next = info;
    current_for_link = current_for_link->next;

    printf("stack %d forCommand %d = %d to %d step %d\n", for_stack_depth,
           info->variable_no, num_variables[info->variable_no], info->to, info->step );

    return RET_CONTINUE;
}

int ScriptParser::filelogCommand()
{
    printf(" filelogCommand\n" );
    FILE *fp;
    int i, j, ch, count = 0;
    char buf[100];
    
    if ( ( fp = fopen( "NScrflog.dat", "rb" ) ) != NULL ){
        //printf("read from NScrflog.dat\n");
        while( (ch = fgetc( fp )) != 0x0a ){
            count = count * 10 + ch - '0';
        }
        //printf("count %d\n",count);

        for ( i=0 ; i<count ; i++ ){
            fgetc( fp );
            j = 0;
            while( (ch = fgetc( fp )) != '"' ) buf[j++] = ch ^ 0x84;
            buf[j] = '\0';
            //printf("read %s\n",buf );
            cBR->getFileLength( buf );
        }
        
        fclose( fp );
    }
    filelog_flag = true;
    return RET_CONTINUE;
}

int ScriptParser::effectblankCommand()
{
    assert( current_mode == DEFINE_MODE );

    char *p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("effectblank") = 11
    effect_blank = readInt( &p_string_buffer, tmp_string_buffer );
    printf("effectblankCoomand %d\n", effect_blank );
    return RET_CONTINUE;
}

int ScriptParser::effectCommand()
{
    bool window_effect_flag = false;
    char *p_string_buffer;
    struct EffectLink *elink;

    if ( !strncmp( string_buffer + string_buffer_offset, "windoweffect", 12 ) ) window_effect_flag = true;
    
    if ( window_effect_flag ){
        //printf("windoweffectCommand\n");
        p_string_buffer = string_buffer + string_buffer_offset + 12; // strlen("windoweffect") = 12
        elink = &window_effect;
    }
    else{
        assert( current_mode == DEFINE_MODE );
        //printf("effectCoomand\n");
        last_effect_link->next = new EffectLink();
        last_effect_link = last_effect_link->next;
        last_effect_link->next = NULL;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("effect") = 6
        elink = last_effect_link;
    }
    
    if ( !window_effect_flag ) elink->num = readInt( &p_string_buffer, tmp_string_buffer );
    elink->effect = readInt( &p_string_buffer, tmp_string_buffer );
    elink->duration = readInt( &p_string_buffer, tmp_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    elink->image = new char[ strlen(tmp_string_buffer ) + 1 ];
    
    memcpy( elink->image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
#if 0
    printf("effect [%d] [%d] [%d] [%s]\n",
           elink->num,
           elink->effect,
           elink->duration,
           elink->image );
#endif
    return RET_CONTINUE;
}

int ScriptParser::divCommand()
{
    int no;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("div") = 3

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] / readInt( &p_string_buffer, tmp_string_buffer ) );

    return RET_CONTINUE;
}

int ScriptParser::decCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("dec") = 3
    int no;
    
    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, num_variables[ no ] - 1 );

    return RET_CONTINUE;
}

int ScriptParser::dateCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("date") = 4
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_year + 1900 );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_mon + 1 );

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
    setNumVariable( atoi( tmp_string_buffer + 1 ), tm->tm_mday );

    //printf("dateCommand %d, %d %d\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday );

    return RET_CONTINUE;
}

int ScriptParser::cmpCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("cmp") = 3
    char *tmp_buffer;
    int no;
    
    readToken( &p_string_buffer, tmp_string_buffer );

    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    tmp_buffer = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( tmp_buffer, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    readStr( &p_string_buffer, tmp_string_buffer );

    setNumVariable( no, strcmp( tmp_buffer, tmp_string_buffer ) );
    delete[] tmp_buffer;
    
    return RET_CONTINUE;
}

int ScriptParser::clickstrCommand()
{
    int i;
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("clickstr") = 8

    readToken( &p_string_buffer, tmp_string_buffer );

    clickstr_num = strlen( tmp_string_buffer ) / 2;
    clickstr_list = new char[clickstr_num * 2];
    for ( i=0 ; i<clickstr_num*2 ; i++ ) clickstr_list[i] = tmp_string_buffer[i];

    clickstr_line = readInt( &p_string_buffer, tmp_string_buffer );
           
    return RET_CONTINUE;
}

int ScriptParser::breakCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("break") = 5

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] == '*' ){
        current_link_label_info->label_info = lookupLabel( tmp_string_buffer + 1 );
        current_link_label_info->current_line = 0;
        current_link_label_info->offset = 0;
        
        return RET_JUMP;
    }
    else{
        break_flag = true;
        return RET_CONTINUE;
    }
}

int ScriptParser::atoiCommand()
{
    int no;
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("atoi") = 4
    
    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    readStr( &p_string_buffer, tmp_string_buffer );

    setNumVariable( no, atoi( tmp_string_buffer ) );
    
    return RET_CONTINUE;
}

int ScriptParser::arcCommand()
{
    delete cBR;
    cBR = new NsaReader();
    if ( cBR->open() ){
        printf(" *** failed to open archive, exitting ...  ***\n");
        exit(-1);
    }
    return RET_CONTINUE;
}

int ScriptParser::addCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("add") = 3
    char *tmp_buffer;
    int no, a;
    
    readToken( &p_string_buffer, tmp_string_buffer );

    if ( tmp_string_buffer[0] != '%' && tmp_string_buffer[0] != '$' ) errorAndExit( string_buffer + string_buffer_offset );

    no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    if ( tmp_string_buffer[0] == '%' ){
        a = readInt( &p_string_buffer, tmp_string_buffer );
        setNumVariable( no, num_variables[ no ] + a );
        
        //printf("addCommand %d = %d + %d\n", num_variables[ no ], num_variables[ no ]-a, a );
    }
    else{
        readStr( &p_string_buffer, tmp_string_buffer );
        tmp_buffer = str_variables[ no ];
        str_variables[ no ] = new char[ strlen( tmp_buffer ) + strlen( tmp_string_buffer ) + 1 ];
        memcpy( str_variables[ no ], tmp_buffer, strlen( tmp_buffer ) );
        memcpy( str_variables[ no ] + strlen( tmp_buffer ), tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        //printf("addCommand %s = %s + %s\n", str_variables[ no ], tmp_buffer, tmp_string_buffer );
        delete[] tmp_buffer;
    }


    return RET_CONTINUE;
}

