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
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("underline") = 9

    underline_value = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::versionstrCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("versionstr") = 10
    int  length[2];
    
    delete[] version_str;

    printf("versionstr %s\n",p_string_buffer);
    
    readStr( &p_string_buffer, tmp_string_buffer );
    length[0] = strlen(tmp_string_buffer) + strlen("\n");

    char *buf = new char[ length[0] ];
    memcpy( buf, tmp_string_buffer, length[0] );

    readStr( &p_string_buffer, tmp_string_buffer );
    length[1] = strlen(tmp_string_buffer) + strlen("\n");
    version_str = new char[ length[0] + length[1] + 1 ];
    sprintf( version_str, "%s\n%s\n", buf, tmp_string_buffer );

    delete[] buf;
    
    return RET_CONTINUE;
}

int ScriptParser::transmodeCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("transmode") = 9

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( !strcmp( tmp_string_buffer, "leftup" ) )        trans_mode = TRANS_TOPLEFT;
    else if ( !strcmp( tmp_string_buffer, "copy" ) )     trans_mode = TRANS_COPY;
    else if ( !strcmp( tmp_string_buffer, "alpha" ) )    trans_mode = TRANS_ALPHA;
    else if ( !strcmp( tmp_string_buffer, "righttup" ) ) trans_mode = TRANS_TOPRIGHT;

    return RET_CONTINUE;
}

int ScriptParser::timeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("time") = 4
    char *p_buf;
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_hour );
    
    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_min );
    
    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_sec );

    return RET_CONTINUE;
}

int ScriptParser::subCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("sub") = 3
    char *p_buf = p_string_buffer;

    int val1 = readInt( &p_string_buffer );
    int val2 = readInt( &p_string_buffer );
    setInt( p_buf, val1 - val2 );

    return RET_CONTINUE;
}

int ScriptParser::straliasCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("stralias") = 8
    StringAlias *p_str_alias = new StringAlias();
    
    readStr( &p_string_buffer, tmp_string_buffer );
    p_str_alias->alias = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_str_alias->alias, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    p_str_alias->str = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_str_alias->str, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    last_str_alias->next = p_str_alias;
    last_str_alias = last_str_alias->next;
    last_str_alias->next = NULL;

    return RET_CONTINUE;
}

int ScriptParser::skipCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("skip") = 4
    int skip_num    = readInt( &p_string_buffer );
    int skip_offset = current_link_label_info->current_line + skip_num;

    while ( skip_offset >= current_link_label_info->label_info.num_of_lines ){ 
        skip_offset -= current_link_label_info->label_info.num_of_lines + 1;
        current_link_label_info->label_info = lookupLabelNext( current_link_label_info->label_info.name );
    }
    if ( skip_offset == -1 ) skip_offset = 0; // -1 indicates a label line
    current_link_label_info->current_line = skip_offset;
    current_link_label_info->offset = 0;

    return RET_JUMP;
}

int ScriptParser::selectcolorCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("selectcolor") = 11

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &sentence_font.on_color, tmp_string_buffer + 1 );

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &sentence_font.off_color, tmp_string_buffer + 1 );
    
    return RET_CONTINUE;
}

int ScriptParser::savenumberCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("savenumber") = 10

    num_save_file = readInt( &p_string_buffer );

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

int ScriptParser::roffCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    rmode_flag = false;

    return RET_CONTINUE;
}

int ScriptParser::rmenuCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("rmode") = 5
    MenuLink *menu, *link;
    
    /* ---------------------------------------- */
    /* Delete old MenuLink */
    link = root_menu_link.next;
    while( link ){
        menu = link->next;
        if ( link->label ) delete[] link->label;
        delete link;
        link = menu;
    }

    link = &root_menu_link;
    menu_link_num = 0;
    menu_link_width = 0;

    SKIP_SPACE( p_string_buffer );
    while ( *p_string_buffer ){
        MenuLink *menu = new MenuLink();

        readStr( &p_string_buffer, tmp_string_buffer );
        menu->label = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( menu->label, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        if ( menu_link_width < strlen( tmp_string_buffer ) / 2 ) menu_link_width = strlen( tmp_string_buffer ) / 2;

        readStr( &p_string_buffer, tmp_string_buffer );
        menu->system_call_no = getSystemCallNo( tmp_string_buffer );

        menu->next = NULL;
        link->next = menu;
        link = menu;
        menu_link_num++;
    }
    
    return RET_CONTINUE;
}

int ScriptParser::returnCommand()
{
    if ( --label_stack_depth < 0 ) errorAndExit( string_buffer + string_buffer_offset );
    
    current_link_label_info = current_link_label_info->previous;
    delete current_link_label_info->next;
    current_link_label_info->next = NULL;

    return RET_JUMP;
}

int ScriptParser::numaliasCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("numalias") = 8
    NameAlias *p_name_alias = new NameAlias();
    
    readStr( &p_string_buffer, tmp_string_buffer );
    p_name_alias->alias = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( p_name_alias->alias, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    
    p_name_alias->num = readInt( &p_string_buffer );

    last_name_alias->next = p_name_alias;
    last_name_alias = last_name_alias->next;
    last_name_alias->next = NULL;
    
    return RET_CONTINUE;
}

int ScriptParser::nsaCommand()
{
    printf("nsa\n");

    delete cBR;
    cBR = new NsaReader();
    if ( cBR->open() ){
        printf(" *** failed to open archive, exitting ...  ***\n");
        exit(-1);
    }

    return RET_CONTINUE;
}

int ScriptParser::nextCommand()
{
    //printf("nextCommand %d( %d -> %d)\n", for_stack_depth, *current_for_link->p_var, *current_for_link->p_var + current_for_link->step );

    /* ***** Ugly code !! ***** */
    if ( current_for_link->var_type == VAR_INT )
        setNumVariable( current_for_link->var_no, *current_for_link->p_var + current_for_link->step );
    else{
        *current_for_link->p_var += current_for_link->step;
    }
    
    if ( break_flag ||
         current_for_link->step >= 0 && *current_for_link->p_var > current_for_link->to ||
         current_for_link->step < 0 && *current_for_link->p_var < current_for_link->to ){
        break_flag = false;
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mul") = 3
    char *p_buf = p_string_buffer;

    int val1 = readInt( &p_string_buffer );
    int val2 = readInt( &p_string_buffer );
    setInt( p_buf, val1 * val2 );

    return RET_CONTINUE;
}

int ScriptParser::movCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset;
    int count, no;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "mov10", 5 ) ){
        p_string_buffer += 5; // strlen("mov10") = 5
        count = 10;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "movl", 4 ) ){
        p_string_buffer += 4; // strlen("movl") = 4
        count = 20;
    }
    else if ( string_buffer[string_buffer_offset+3] >= '3' && string_buffer[string_buffer_offset+3] <= '9' ){
        p_string_buffer += 4; // strlen("mov?") = 4
        count = string_buffer[string_buffer_offset+3] - '0';
    }
    else{
        p_string_buffer += 3; // strlen("mov") = 3
        count = 1;
    }

    SKIP_SPACE( p_string_buffer );
    
    if ( p_string_buffer[0] == '%' || p_string_buffer[0] == '?' ){
        char *p_buf = p_string_buffer;
        readInt( &p_string_buffer );
        bool loop_flag = end_with_comma_flag;
        int i=0;
        while ( i<count && loop_flag ){
            no = readInt( &p_string_buffer );
            loop_flag = end_with_comma_flag;
            setInt( p_buf, no, i++ );
        }
    }
    else if ( p_string_buffer[0] == '$'){
        p_string_buffer++;
        no = readInt( &p_string_buffer );

        readStr( &p_string_buffer, tmp_string_buffer );
        delete[] str_variables[ no ];
        str_variables[ no ] = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( str_variables[ no ], tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        //printf("movCommand %d, %s\n", no, tmp_string_buffer );
    }
    else errorAndExit( string_buffer + string_buffer_offset );
    
    return RET_CONTINUE;
}

int ScriptParser::modCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mod") = 3
    char *p_buf = p_string_buffer;

    int val1 = readInt( &p_string_buffer );
    int val2 = readInt( &p_string_buffer );
    setInt( p_buf, val1 % val2 );

    return RET_CONTINUE;
}

int ScriptParser::midCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mid") = 3
    int no, start, len;
    
    SKIP_SPACE( p_string_buffer );

    if ( p_string_buffer[0] == '$'){
        p_string_buffer++;
        no = readInt( &p_string_buffer );
        readStr( &p_string_buffer, tmp_string_buffer );
        start = readInt( &p_string_buffer );
        len = readInt( &p_string_buffer );

        delete[] str_variables[ no ];
        str_variables[ no ] = new char[ len + 1 ];
        memcpy( str_variables[ no ], tmp_string_buffer + start, len );
        str_variables[ no ][ len ] = '\0';
        //printf(" mid %d -> %s\n",no,str_variables[ no ]);
    }
    else errorAndExit( string_buffer + string_buffer_offset );

    return RET_CONTINUE;
}

int ScriptParser::menusetwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 13; // strlen("menusetwindow") = 13

    menu_font.font_valid_flag = false;
    menu_font.font_size = readInt( &p_string_buffer );
    readInt( &p_string_buffer ); // Ignore font size along Y axis
    menu_font.pitch_xy[0] = readInt( &p_string_buffer ) + menu_font.font_size;
    menu_font.pitch_xy[1] = readInt( &p_string_buffer ) + menu_font.font_size;
    menu_font.display_bold = readInt( &p_string_buffer )?true:false;
    menu_font.display_shadow = readInt( &p_string_buffer )?true:false;

    //menu_font.top_xy[1] += menu_font.font_size;
#if 0
    printf("ONScripterLabel::menusetwindowCommand font=%d (%d,%d) bold=%d, shadow=%d\n",
           menu_font.font_size, menu_font.pitch_xy[0], menu_font.pitch_xy[1],
           menu_font.display_bold, menu_font.display_shadow );
#endif
    readStr( &p_string_buffer, tmp_string_buffer );

    menu_font.display_transparency = true;
    if ( strlen( tmp_string_buffer ) != 7 ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &menu_font.window_color, tmp_string_buffer + 1 );

#if 0
    printf("    trans %u %u %u\n",
           menu_font.window_color[0], menu_font.window_color[1], menu_font.window_color[2] );
#endif
    return RET_CONTINUE;
}

int ScriptParser::menuselectcolorCommand()
{
    int i;
    char *p_string_buffer = string_buffer + string_buffer_offset + 15; // strlen("menuselectcolor") = 15

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &menu_font.on_color, tmp_string_buffer + 1 );
    for ( i=0 ; i<3 ; i++ ) system_font.on_color[i] = menu_font.on_color[i];

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &menu_font.off_color, tmp_string_buffer + 1 );
    for ( i=0 ; i<3 ; i++ ) system_font.off_color[i] = menu_font.off_color[i];
    
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &menu_font.nofile_color, tmp_string_buffer + 1 );
    for ( i=0 ; i<3 ; i++ ) system_font.nofile_color[i] = menu_font.nofile_color[i];
    
    return RET_CONTINUE;
}

int ScriptParser::lookbackcolorCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 13; // strlen("lookbackcolor") = 13

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset );
    readColor( &lookback_color, tmp_string_buffer + 1 );

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
        SKIP_SPACE( p_string_buffer );

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
            left_value = readInt( &p_string_buffer );
            //printf("%s(%d) ", tmp_string_buffer, left_value );

            SKIP_SPACE( p_string_buffer );
            token_start = p_string_buffer;
            while ( *p_string_buffer == '>' || *p_string_buffer == '<' ||
                    *p_string_buffer == '=' || *p_string_buffer == '!' ) p_string_buffer++;
            memcpy( eval_str, token_start, p_string_buffer-token_start );
            eval_str[ p_string_buffer-token_start ] = '\0';
            //printf("%s ", eval_str );

            SKIP_SPACE( p_string_buffer );

            right_value = readInt( &p_string_buffer );
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
        SKIP_SPACE( p_string_buffer );

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
    int no, val;
    char val_str[20], *tmp_buffer;
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("itoa") = 4
    
    SKIP_SPACE( p_string_buffer );

    if ( p_string_buffer[0] != '$' ) errorAndExit( string_buffer + string_buffer_offset );

    p_string_buffer++;
    no = readInt( &p_string_buffer );

    val = readInt( &p_string_buffer );

    tmp_buffer = str_variables[ no ];
    sprintf( val_str, "%d", val );
    str_variables[ no ] = new char[ strlen( val_str ) + 1 ];
    memcpy( str_variables[ no ], val_str, strlen( val_str ) + 1 );
    delete[] tmp_buffer;
    
    return RET_CONTINUE;
}

int ScriptParser::intlimitCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("intlimit") = 8
    int no = readInt( &p_string_buffer );

    num_limit_flag[ no ] = true;
    num_limit_lower[ no ] = readInt( &p_string_buffer );
    num_limit_upper[ no ] = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::incCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("inc") = 3
    char *p_buf = p_string_buffer;

    int val = readInt( &p_string_buffer );
    setInt( p_buf, val + 1 );

    return RET_CONTINUE;
}

int ScriptParser::humanzCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("humanz") = 6
    z_order = readInt( &p_string_buffer );
    
    return RET_CONTINUE;
}

int ScriptParser::gotoCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("goto") = 4
    readStr( &p_string_buffer, tmp_string_buffer );

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
    char *p_buf2;
    ForInfo *info = new ForInfo();
    info->next = NULL;

    SKIP_SPACE( p_string_buffer );
    char *p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    //char *p_dst_buf = tmp_string_buffer;
    //info->variable_no = readInt( &p_dst_buf );
    
    if ( *p_string_buffer != '=' ) errorAndExit( string_buffer + string_buffer_offset );
    p_string_buffer++;

    if ( p_buf[0] == '%' ){
        info->var_type = VAR_INT;
        p_buf2 = p_buf + 1;
        info->var_no = readInt( &p_buf2 );
        info->p_var = num_variables + info->var_no;
    }
    else{
        info->var_type = VAR_ARRAY;
        p_buf2 = p_buf;
        info->p_var = decodeArray( &p_buf2 );
    }

    setInt( p_buf, readInt( &p_string_buffer ) );
    //setNumVariable( info->variable_no, 
    
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( strcmp( tmp_string_buffer, "to" ) ) errorAndExit( string_buffer + string_buffer_offset );
    
    info->to = readInt( &p_string_buffer );

    SKIP_SPACE( p_string_buffer );
    if ( !strncmp( p_string_buffer, "step", 4 ) ){
        readStr( &p_string_buffer, tmp_string_buffer );
        info->step = readInt( &p_string_buffer );
    }
    else{
        info->step = 1;
    }
    
    /* ---------------------------------------- */
    /* Step forward callee's label info */
    SKIP_SPACE( p_string_buffer );
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

    //printf("stack %d forCommand %d = %d to %d step %d\n", for_stack_depth,
    //info->var_no, *info->p_var, info->to, info->step );

    string_buffer_offset = p_string_buffer - string_buffer;
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
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("effectblank") = 11
    
    effect_blank = readInt( &p_string_buffer );

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
        if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
        //printf("effectCoomand\n");
        last_effect_link->next = new EffectLink();
        last_effect_link = last_effect_link->next;
        last_effect_link->next = NULL;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("effect") = 6
        elink = last_effect_link;
    }
    
    if ( !window_effect_flag ) elink->num = readInt( &p_string_buffer );
    elink->effect = readInt( &p_string_buffer );
    elink->duration = readInt( &p_string_buffer );
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("div") = 3
    char *p_buf = p_string_buffer;

    int val1 = readInt( &p_string_buffer );
    int val2 = readInt( &p_string_buffer );
    setInt( p_buf, val1 / val2 );

    return RET_CONTINUE;
}

int ScriptParser::dimCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( string_buffer + string_buffer_offset );
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("dim") = 3

    struct ArrayVariable array;
    int dim = 1;
    
    int no = decodeArraySub( &p_string_buffer, &array );

    array_variables[ no ].num_dim = array.num_dim;
    //printf("dimCommand no=%d dim=%d\n",no,array.num_dim);
    for ( int i=0 ; i<array.num_dim ; i++ ){
        //printf("%d ",array.dim[i]);
        array_variables[ no ].dim[i] = array.dim[i]+1;
        dim *= (array.dim[i]+1);
        array_variables[ no ].data = new int[ dim ];
        memset( array_variables[ no ].data, 0, sizeof(int) * dim );
    }
    
    return RET_CONTINUE;
}

int ScriptParser::defvoicevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("defvoicevol") = 11
    voice_volume = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::defsevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("defsevol") = 8
    se_volume = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::defmp3volCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("defmp3vol") = 9
    mp3_volume = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ScriptParser::decCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("dec") = 3
    char *p_buf = p_string_buffer;

    int val = readInt( &p_string_buffer );
    setInt( p_buf, val - 1 );

    return RET_CONTINUE;
}

int ScriptParser::dateCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("date") = 4
    char *p_buf;
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_year + 1900 );

    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_mon + 1 );

    p_buf = p_string_buffer;
    readInt( &p_string_buffer );
    setInt( p_buf, tm->tm_mday );

    return RET_CONTINUE;
}

int ScriptParser::cmpCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("cmp") = 3
    char *p_buf = p_string_buffer, *tmp_buffer;

    readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    tmp_buffer = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( tmp_buffer, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    readStr( &p_string_buffer, tmp_string_buffer );

    //printf("cmp between %s <-> %s\n", tmp_buffer, tmp_string_buffer);
    setInt( p_buf, strcmp( tmp_buffer, tmp_string_buffer ) );
    delete[] tmp_buffer;
    
    return RET_CONTINUE;
}

int ScriptParser::clickstrCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("clickstr") = 8

    readStr( &p_string_buffer, tmp_string_buffer );

    clickstr_num = strlen( tmp_string_buffer ) / 2;
    clickstr_list = new char[clickstr_num * 2];
    for ( int i=0 ; i<clickstr_num*2 ; i++ ) clickstr_list[i] = tmp_string_buffer[i];

    clickstr_line = readInt( &p_string_buffer );
           
    return RET_CONTINUE;
}

int ScriptParser::breakCommand()
{
    printf("breakCommand\n");
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("atoi") = 4
    char *p_buf = p_string_buffer;

    readInt( &p_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
        
    setInt( p_buf, atoi( tmp_string_buffer ) );
    
    return RET_CONTINUE;
}

int ScriptParser::arcCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("arc") = 3
    readStr( &p_string_buffer, tmp_string_buffer );

    int i = 0;
    while ( tmp_string_buffer[i] != '|' && tmp_string_buffer[i] != '\0' ) i++;
    tmp_string_buffer[i] = '\0';

    if ( strcmp( cBR->getArchiveName(), "sar" ) ){
        delete cBR;
        cBR = new SarReader();
    }
    if ( cBR->open( tmp_string_buffer ) ){
        printf(" *** failed to open archive %s ...  ***\n", tmp_string_buffer );
        //exit(-1);
    }
    return RET_CONTINUE;
}

int ScriptParser::addCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("add") = 3
    char *tmp_buffer;
    int no;
    
    SKIP_SPACE( p_string_buffer );

    if ( p_string_buffer[0] == '%' || p_string_buffer[0] == '?' ){
        char *p_buf = p_string_buffer;
        no = readInt( &p_string_buffer );

        setInt( p_buf, no + readInt( &p_string_buffer ) );
    }
    else if ( p_string_buffer[0] == '$'){
        p_string_buffer++;
        no = readInt( &p_string_buffer );
        readStr( &p_string_buffer, tmp_string_buffer );
        tmp_buffer = str_variables[ no ];
        str_variables[ no ] = new char[ strlen( tmp_buffer ) + strlen( tmp_string_buffer ) + 1 ];
        memcpy( str_variables[ no ], tmp_buffer, strlen( tmp_buffer ) );
        memcpy( str_variables[ no ] + strlen( tmp_buffer ), tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        //printf("addCommand %s = %s + %s\n", str_variables[ no ], tmp_buffer, tmp_string_buffer );
        delete[] tmp_buffer;
    }
    else errorAndExit( string_buffer + string_buffer_offset );

    return RET_CONTINUE;
}

