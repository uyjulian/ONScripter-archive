/* -*- C++ -*-
 *
 *  ScriptParser_command.cpp - Define command executer of ONScripter
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

#include "ScriptParser.h"

int ScriptParser::windowbackCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( script_h.getStringBuffer(), "not in the define section" );
    windowback_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::versionstrCommand()
{
    delete[] version_str;

    script_h.readStr();
    const char *save_buf = script_h.saveStringBuffer();

    const char *buf = script_h.readStr();
    version_str = new char[ strlen( save_buf ) + strlen( buf ) + strlen("\n") * 2 + 1 ];
    sprintf( version_str, "%s\n%s\n", save_buf, buf );

    printf("versionstr %s", version_str );
    
    return RET_CONTINUE;
}

int ScriptParser::usewheelCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "usewheel: not in the define section" );

    usewheel_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::useescspcCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "useescspc: not in the define section" );

    if ( !force_button_shortcut_flag )
        useescspc_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::underlineCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "underline: not in the define section" );

    underline_value = script_h.readInt() * screen_ratio1 / screen_ratio2;

    return RET_CONTINUE;
}

int ScriptParser::transmodeCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "transmode: not in the define section" );

    const char *buf = script_h.readLabel();
    if      ( !strcmp( buf, "leftup" ) )   trans_mode = AnimationInfo::TRANS_TOPLEFT;
    else if ( !strcmp( buf, "copy" ) )     trans_mode = AnimationInfo::TRANS_COPY;
    else if ( !strcmp( buf, "alpha" ) )    trans_mode = AnimationInfo::TRANS_ALPHA;
    else if ( !strcmp( buf, "righttup" ) ) trans_mode = AnimationInfo::TRANS_TOPRIGHT;

    return RET_CONTINUE;
}

int ScriptParser::timeCommand()
{
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    script_h.readVariable();
    script_h.setInt( &script_h.current_variable, tm->tm_hour );
    
    script_h.readVariable();
    script_h.setInt( &script_h.current_variable, tm->tm_min );
    
    script_h.readVariable();
    script_h.setInt( &script_h.current_variable, tm->tm_sec );

    return RET_CONTINUE;
}

int ScriptParser::textgosubCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "textgosub: not in the define section" );

    setStr( &textgosub_label, script_h.readLabel()+1 );
    script_h.enableTextgosub(true);
    
    return RET_CONTINUE;
}

int ScriptParser::subCommand()
{
    int val1 = script_h.readInt();
    script_h.pushVariable();

    int val2 = script_h.readInt();
    script_h.setInt( &script_h.pushed_variable, val1 - val2 );

    return RET_CONTINUE;
}

int ScriptParser::straliasCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "stralias: not in the define section" );
    
    script_h.readLabel();
    const char *save_buf = script_h.saveStringBuffer();
    const char *buf = script_h.readStr();
    
    script_h.addStrAlias( save_buf, buf );
    
    return RET_CONTINUE;
}

int ScriptParser::soundpressplginCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "soundpressplgin: not in the define section" );

    const char *buf = script_h.readStr();
    while( *buf != '|' ) buf++;
    buf++;

    // We assume NZB compression here.
    script_h.cBR->registerCompressionType( buf, BaseReader::NBZ_COMPRESSION );

    return RET_CONTINUE;
}

int ScriptParser::skipCommand()
{
    int line = current_link_label_info->label_info.start_line + current_link_label_info->current_line + script_h.readInt();

    char *buf = script_h.getAddressByLine( line );
    current_link_label_info->label_info = script_h.getLabelByAddress( buf );
    current_link_label_info->current_line = script_h.getLineByAddress( buf );
    
    script_h.setCurrent( buf );

    return RET_CONTINUE;
}

int ScriptParser::shadedistanceCommand()
{
    if (current_mode != DEFINE_MODE) errorAndExit( "shadedistance: not in the define section" );
    shade_distance[0] = script_h.readInt();
    shade_distance[1] = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::selectvoiceCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "selectvoice: not in the define section" );

    for ( int i=0 ; i<SELECTVOICE_NUM ; i++ ){
        const char *buf = script_h.readStr();
        setStr( &selectvoice_file_name[i], buf );
    }

    return RET_CONTINUE;
}

int ScriptParser::selectcolorCommand()
{
    const char *buf = script_h.readStr();
    readColor( &sentence_font.on_color, buf );

    buf = script_h.readStr();
    readColor( &sentence_font.off_color, buf );
    
    return RET_CONTINUE;
}

int ScriptParser::savenumberCommand()
{
    num_save_file = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::savenameCommand()
{
    const char *buf = script_h.readStr();
    setStr( &save_menu_name, buf );

    buf = script_h.readStr();
    setStr( &load_menu_name, buf );

    buf = script_h.readStr();
    setStr( &save_item_name, buf );

    return RET_CONTINUE;
}

int ScriptParser::rubyonCommand()
{
    rubyon_flag = true;

    char *buf = script_h.getNext();
    if ( buf[0] == 0x0a || buf[0] == ':' || buf[0] == ';' ){
        ruby_struct.font_size_xy[0] = -1;
        ruby_struct.font_size_xy[1] = -1;
        setStr( &ruby_struct.font_name, NULL );
    }
    else{
        ruby_struct.font_size_xy[0] = script_h.readInt();
        ruby_struct.font_size_xy[1] = script_h.readInt();

        if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
            setStr( &ruby_struct.font_name, script_h.readStr() );
        }
        else{
            setStr( &ruby_struct.font_name, NULL );
        }
    }

    return RET_CONTINUE;
}

int ScriptParser::rubyoffCommand()
{
    rubyon_flag = false;

    return RET_CONTINUE;
}

int ScriptParser::roffCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "roff: not in the define section" );
    rmode_flag = false;

    return RET_CONTINUE;
}

int ScriptParser::rmenuCommand()
{
    /* ---------------------------------------- */
    /* Delete old MenuLink */
    MenuLink *link = root_menu_link.next;
    while( link ){
        MenuLink *menu = link->next;
        if ( link->label ) delete[] link->label;
        delete link;
        link = menu;
    }

    link = &root_menu_link;
    menu_link_num   = 0;
    menu_link_width = 0;

    bool comma_flag = true;
    while ( comma_flag ){
        link->next = new MenuLink();

        const char *buf = script_h.readStr();
        setStr( &link->next->label, buf );
        if ( menu_link_width < strlen( buf )/2 + 1 )
            menu_link_width = strlen( buf )/2 + 1;

        link->next->system_call_no = getSystemCallNo( script_h.readLabel() );

        link = link->next;
        menu_link_num++;

        comma_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
    }
    
    return RET_CONTINUE;
}

int ScriptParser::returnCommand()
{
    if ( current_link_label_info->previous == NULL ) errorAndExit( "return: too many returns" );
    
    current_link_label_info = current_link_label_info->previous;
    script_h.setCurrent( current_link_label_info->next_script );

    delete current_link_label_info->next;
    current_link_label_info->next = NULL;
    
    return RET_CONTINUE;
}

int ScriptParser::pretextgosubCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "pretextgosub: not in the define section" );

    setStr( &pretextgosub_label, script_h.readLabel()+1 );
    
    return RET_CONTINUE;
}

int ScriptParser::numaliasCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "numalias: numalias: not in the define section" );

    script_h.readLabel();
    const char *save_buf = script_h.saveStringBuffer();

    int no = script_h.readInt();
    script_h.addNumAlias( save_buf, no );
    
    return RET_CONTINUE;
}

int ScriptParser::nsadirCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "nsadir: not in the define section" );

    const char *buf = script_h.readStr();
    
    if ( strlen(nsa_path) > 0 ){
        delete[] nsa_path;
    }
    nsa_path = new char[ strlen(buf) + 2 ];
    sprintf( nsa_path, RELATIVEPATH "%s%c", buf, DELIMITER );

    return RET_CONTINUE;
}

int ScriptParser::nsaCommand()
{
    int archive_type = NsaReader::ARCHIVE_TYPE_NSA;
    
    if ( script_h.isName("ns2") ){
        archive_type = NsaReader::ARCHIVE_TYPE_NS2;
    }
    else if ( script_h.isName("ns3") ){
        archive_type = NsaReader::ARCHIVE_TYPE_NS3;
    }
    
    delete script_h.cBR;
    script_h.cBR = new NsaReader( archive_path, key_table );
    if ( script_h.cBR->open( nsa_path, archive_type ) ){
        fprintf( stderr, " *** failed to open Nsa archive, continuing ...  ***\n");
    }

    return RET_CONTINUE;
}

int ScriptParser::nextCommand()
{
    int val;
    
    if ( !break_flag ){
        val   = script_h.getIntVariable( &current_for_link->var );
        script_h.setInt( &current_for_link->var, val + current_for_link->step );
    }

    val = script_h.getIntVariable( &current_for_link->var );
    
    if ( break_flag ||
         current_for_link->step >= 0 && val > current_for_link->to ||
         current_for_link->step < 0  && val < current_for_link->to ){
        break_flag = false;
        for_stack_depth--;
        current_for_link = current_for_link->previous;

        delete current_for_link->next;
        current_for_link->next = NULL;
    }
    else{
        current_link_label_info->label_info   = current_for_link->label_info;
        current_link_label_info->current_line = current_for_link->current_line;
        script_h.setCurrent( current_for_link->next_script );
    }
    
    return RET_CONTINUE;
}

int ScriptParser::mulCommand()
{
    int val1 = script_h.readInt();
    script_h.pushVariable();
    
    int val2 = script_h.readInt();
    script_h.setInt( &script_h.pushed_variable, val1*val2 );

    return RET_CONTINUE;
}

int ScriptParser::movCommand()
{
    int count = 1;
    
    if ( script_h.isName( "mov10" ) ){
        count = 10;
    }
    else if ( script_h.isName( "movl" ) ){
        count = -1; // infinite
    }
    else if ( script_h.getStringBuffer()[3] >= '3' && script_h.getStringBuffer()[3] <= '9' ){
        count = script_h.getStringBuffer()[3] - '0';
    }

    script_h.readVariable();

    if ( script_h.current_variable.type == ScriptHandler::VAR_INT ||
         script_h.current_variable.type == ScriptHandler::VAR_ARRAY ){
        script_h.pushVariable();
        bool loop_flag = (script_h.getEndStatus() & ScriptHandler::END_COMMA);
        int i=0;
        while ( (count==-1 || i<count) && loop_flag ){
            int no = script_h.readInt();
            loop_flag = (script_h.getEndStatus() & ScriptHandler::END_COMMA);
            script_h.setInt( &script_h.pushed_variable, no, i++ );
        }
    }
    else if ( script_h.current_variable.type == ScriptHandler::VAR_STR ){
        script_h.pushVariable();
        const char *buf = script_h.readStr();
        setStr( &script_h.str_variables[ script_h.pushed_variable.var_no ], buf );
    }
    else errorAndExit( "mov: no variable" );
    
    return RET_CONTINUE;
}

int ScriptParser::mode_sayaCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "mode_saya: not in the define section" );
    mode_saya_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::mode_extCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "mode_ext: not in the define section" );
    mode_ext_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::modCommand()
{
    int val1 = script_h.readInt();
    script_h.pushVariable();
    
    int val2 = script_h.readInt();
    script_h.setInt( &script_h.pushed_variable, val1%val2 );

    return RET_CONTINUE;
}

int ScriptParser::midCommand()
{
    script_h.readStr();
    if ( script_h.current_variable.type != ScriptHandler::VAR_STR )
        errorAndExit( "mid: no string variable" );
    int no = script_h.current_variable.var_no;
    
    script_h.readStr();
    const char *save_buf = script_h.saveStringBuffer();
    unsigned int start = script_h.readInt();
    unsigned int len   = script_h.readInt();

    if ( script_h.str_variables[no] ) delete[] script_h.str_variables[no];
    if ( start >= strlen(save_buf) ){
        script_h.str_variables[no] = NULL;
    }
    else{
        if ( start+len > strlen(save_buf ) )
            len = strlen(save_buf) - start;
        script_h.str_variables[no] = new char[len+1];
        memcpy( script_h.str_variables[no], save_buf+start, len );
        script_h.str_variables[no][len] = '\0';
    }

    return RET_CONTINUE;
}

int ScriptParser::menusetwindowCommand()
{
    menu_font.ttf_font        = NULL;
    menu_font.font_size_xy[0] = script_h.readInt();
    menu_font.font_size_xy[1] = script_h.readInt();
    menu_font.pitch_xy[0]     = script_h.readInt() + menu_font.font_size_xy[0];
    menu_font.pitch_xy[1]     = script_h.readInt() + menu_font.font_size_xy[1];
    menu_font.is_bold         = script_h.readInt()?true:false;
    menu_font.is_shadow       = script_h.readInt()?true:false;

    const char *buf = script_h.readStr();
    if ( strlen(buf) ){ // Comma may or may not be appeared in this case.
        readColor( &menu_font.window_color, buf );
    }
    else{
        menu_font.window_color[0] = menu_font.window_color[1] = menu_font.window_color[2] = 0x99;
    }

    return RET_CONTINUE;
}

int ScriptParser::menuselectvoiceCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "menuselectvoice: not in the define section" );

    for ( int i=0 ; i<MENUSELECTVOICE_NUM ; i++ ){
        const char *buf = script_h.readStr();
        setStr( &menuselectvoice_file_name[i], buf );
    }

    return RET_CONTINUE;
}

int ScriptParser::menuselectcolorCommand()
{
    const char *buf = script_h.readStr();
    readColor( &menu_font.on_color, buf );

    buf = script_h.readStr();
    readColor( &menu_font.off_color, buf );
    
    buf = script_h.readStr();
    readColor( &menu_font.nofile_color, buf );
    
    return RET_CONTINUE;
}

int ScriptParser::maxkaisoupageCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "maxkaisoupage: not in the define section" );
    max_text_buffer = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::lookbackcolorCommand()
{
    const char *buf = script_h.readStr();
    readColor( &lookback_color, buf );

    return RET_CONTINUE;
}

int ScriptParser::linepageCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "linepage: not in the define section" );
    script_h.setLinepage( true );

    return RET_CONTINUE;
}

int ScriptParser::lenCommand()
{
    script_h.readInt();
    script_h.pushVariable();
    
    const char *buf = script_h.readStr();

    script_h.setInt( &script_h.pushed_variable, strlen( buf ) );

    return RET_CONTINUE;
}

int ScriptParser::labellogCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "labellog: not in the define section" );

    labellog_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::kidokuskipCommand()
{
    kidokuskip_flag = true;
    kidokumode_flag = true;
    script_h.loadKidokuData();
    
    return RET_CONTINUE;
}

int ScriptParser::kidokumodeCommand()
{
    if ( script_h.readInt() == 1 )
        kidokumode_flag = true;
    else
        kidokumode_flag = false;

    return RET_CONTINUE;
}

int ScriptParser::itoaCommand()
{
    script_h.readVariable();
    if ( script_h.current_variable.type != ScriptHandler::VAR_STR )
        errorAndExit( "itoa: no string variable." );
    int no = script_h.current_variable.var_no;

    int val = script_h.readInt();

    char val_str[20];
    sprintf( val_str, "%d", val );
    setStr( &script_h.str_variables[no], val_str );
    
    return RET_CONTINUE;
}

int ScriptParser::intlimitCommand()
{
    int no = script_h.readInt();

    script_h.num_limit_flag[ no ]  = true;
    script_h.num_limit_lower[ no ] = script_h.readInt();
    script_h.num_limit_upper[ no ] = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::incCommand()
{
    int val = script_h.readInt();
    script_h.setInt( &script_h.current_variable, val+1 );

    return RET_CONTINUE;
}

int ScriptParser::ifCommand()
{
    //printf("ifCommand\n");
    bool if_flag, condition_flag = true, f = false;
    char *op_buf;
    const char *buf;

    if ( script_h.isName( "notif" ) )
        if_flag = false;
    else
        if_flag = true;

    while(1){
        if (script_h.compareString("fchk")){
            script_h.readLabel();
            buf = script_h.readStr();
            f = (script_h.findAndAddLog( ScriptHandler::FILE_LOG, buf, false ));
            //printf("fchk %s(%d,%d) ", tmp_string_buffer, (findAndAddFileLog( tmp_string_buffer, fasle )), condition_flag );
        }
        else if (script_h.compareString("lchk")){
            script_h.readLabel();
            buf = script_h.readStr();
            f = (script_h.findAndAddLog( ScriptHandler::LABEL_LOG, buf+1, false ));
            //printf("lchk %s(%d,%d) ", tmp_string_buffer, script_h.fineAndAddLabelLog( tmp_string_buffer+1, false ), condition_flag );
        }
        else{
            int no = script_h.readInt();
            if (script_h.current_variable.type & ScriptHandler::VAR_INT ||
                script_h.current_variable.type & ScriptHandler::VAR_ARRAY){
                int left_value = no;
                //printf("left (%d) ", left_value );

                op_buf = script_h.getNext();
                if ( (op_buf[0] == '>' && op_buf[1] == '=') ||
                     (op_buf[0] == '<' && op_buf[1] == '=') ||
                     (op_buf[0] == '=' && op_buf[1] == '=') ||
                     (op_buf[0] == '!' && op_buf[1] == '=') ||
                     (op_buf[0] == '<' && op_buf[1] == '>') )
                    script_h.setCurrent(op_buf+2);
                else if ( op_buf[0] == '<' ||
                          op_buf[0] == '>' ||
                          op_buf[0] == '=' )
                    script_h.setCurrent(op_buf+1);
                //printf("current %c%c ", op_buf[0], op_buf[1] );

                int right_value = script_h.readInt();
                //printf("right (%d) ", right_value );

                if      (op_buf[0] == '>' && op_buf[1] == '=') f = (left_value >= right_value);
                else if (op_buf[0] == '<' && op_buf[1] == '=') f = (left_value <= right_value);
                else if (op_buf[0] == '=' && op_buf[1] == '=') f = (left_value == right_value);
                else if (op_buf[0] == '!' && op_buf[1] == '=') f = (left_value != right_value);
                else if (op_buf[0] == '<' && op_buf[1] == '>') f = (left_value != right_value);
                else if (op_buf[0] == '<')                     f = (left_value <  right_value);
                else if (op_buf[0] == '>')                     f = (left_value >  right_value);
                else if (op_buf[0] == '=')                     f = (left_value == right_value);
            }
            else{
                script_h.setCurrent(script_h.getCurrent());
                buf = script_h.readStr();
                const char *save_buf = script_h.saveStringBuffer();

                op_buf = script_h.getNext();

                if ( (op_buf[0] == '>' && op_buf[1] == '=') ||
                     (op_buf[0] == '<' && op_buf[1] == '=') ||
                     (op_buf[0] == '=' && op_buf[1] == '=') ||
                     (op_buf[0] == '!' && op_buf[1] == '=') ||
                     (op_buf[0] == '<' && op_buf[1] == '>') )
                    script_h.setCurrent(op_buf+2);
                else if ( op_buf[0] == '<' ||
                          op_buf[0] == '>' ||
                          op_buf[0] == '=' )
                    script_h.setCurrent(op_buf+1);
            
                buf = script_h.readStr();

                int val = strcmp( save_buf, buf );
                if      (op_buf[0] == '>' && op_buf[1] == '=') f = (val >= 0);
                else if (op_buf[0] == '<' && op_buf[1] == '=') f = (val <= 0);
                else if (op_buf[0] == '=' && op_buf[1] == '=') f = (val == 0);
                else if (op_buf[0] == '!' && op_buf[1] == '=') f = (val != 0);
                else if (op_buf[0] == '<' && op_buf[1] == '>') f = (val != 0);
                else if (op_buf[0] == '<')                     f = (val <  0);
                else if (op_buf[0] == '>')                     f = (val >  0);
                else if (op_buf[0] == '=')                     f = (val == 0);
            }
        }
        condition_flag &= (if_flag)?(f):(!f);

        op_buf = script_h.getNext();
        if ( op_buf[0] == '&' ){
            while(*op_buf == '&') op_buf++;
            script_h.setCurrent(op_buf);
            continue;
        }
        break;
    };

    /* Execute command */
    if ( condition_flag ) return RET_CONTINUE;
    else                  return RET_SKIP_LINE;
}

int ScriptParser::humanzCommand()
{
    z_order = script_h.readInt();
    
    return RET_CONTINUE;
}

int ScriptParser::gotoCommand()
{
    setCurrentLinkLabel( script_h.readLabel()+1 );
    
    return RET_CONTINUE;
}

void ScriptParser::gosubReal( const char *label, char *next_script )
{
    current_link_label_info->next_script = next_script;

    LinkLabelInfo *info = new LinkLabelInfo();
    info->previous = current_link_label_info;
    current_link_label_info->next = new LinkLabelInfo();
    current_link_label_info->next->previous = current_link_label_info;
    current_link_label_info = current_link_label_info->next;

    setCurrentLinkLabel( label );
}

int ScriptParser::gosubCommand()
{
    const char *buf = script_h.readLabel();
    gosubReal( buf+1, script_h.getNext() );

    return RET_CONTINUE;
}

int ScriptParser::globalonCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "globalon: not in the define section" );
    globalon_flag = true;
    
    return RET_CONTINUE;
}

int ScriptParser::getparamCommand()
{
    int end_status;
    do{
        script_h.readVariable();
        script_h.pushVariable();
        
        if ( current_link_label_info->previous == NULL ) errorAndExit( "getpapam: not in a subroutine" );
        script_h.pushCurrent(current_link_label_info->previous->next_script);

        script_h.readVariable();
        end_status = script_h.getEndStatus();
        
        if ( script_h.pushed_variable.type & ScriptHandler::VAR_PTR ){
            script_h.readVariable();
            script_h.setInt( &script_h.pushed_variable, script_h.current_variable.var_no );
        }
        else if ( script_h.pushed_variable.type & ScriptHandler::VAR_INT ||
                  script_h.pushed_variable.type & ScriptHandler::VAR_ARRAY ){
            script_h.setInt( &script_h.pushed_variable, script_h.readInt() );
        }
        else if ( script_h.pushed_variable.type & ScriptHandler::VAR_STR ){
            const char *buf = script_h.readStr();
            setStr( &script_h.str_variables[ script_h.pushed_variable.var_no ], buf );
        }
        
        current_link_label_info->previous->next_script = script_h.getNext();
        script_h.popCurrent();
    }
    while(end_status & ScriptHandler::END_COMMA);

    return RET_CONTINUE;
}

int ScriptParser::forCommand()
{
    for_stack_depth++;
    ForInfo *info = new ForInfo();

    script_h.readVariable();
    info->var = script_h.current_variable;

    if ( !script_h.compareString("=") ) 
        errorAndExit( "for: no =" );

    script_h.setCurrent(script_h.getNext() + 1);
    script_h.setInt( &info->var, script_h.readInt() );
    
    if ( !script_h.compareString("to") )
        errorAndExit( "for: no to" );

    script_h.readLabel();
    
    info->to = script_h.readInt();

    if ( script_h.compareString("step") ){
        script_h.readLabel();
        info->step = script_h.readInt();
    }
    else{
        info->step = 1;
    }
    
    /* ---------------------------------------- */
    /* Step forward callee's label info */
    info->label_info = current_link_label_info->label_info;
    info->current_line = current_link_label_info->current_line;
    info->next_script = script_h.getNext();

    info->previous = current_for_link;
    current_for_link->next = info;
    current_for_link = current_for_link->next;

    //printf("stack %d forCommand %d = %d to %d step %d\n", for_stack_depth,
    //info->var.var_no, script_h.getIntVariable(&info->var), info->to, info->step );

    return RET_CONTINUE;
}

int ScriptParser::filelogCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "filelog: not in the define section" );

    filelog_flag = true;
    script_h.loadLog( ScriptHandler::FILE_LOG );
    
    return RET_CONTINUE;
}

int ScriptParser::effectcutCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "effectcut: not in the define section." );

    effect_cut_flag = true;

    return RET_CONTINUE;
}

int ScriptParser::effectblankCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "effectblank: not in the define section" );
    
    effect_blank = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::effectCommand()
{
    EffectLink *elink;

    if ( script_h.isName( "windoweffect") ){
        elink = &window_effect;
    }
    else{
        if ( current_mode != DEFINE_MODE ) errorAndExit( "effect: not in the define section" );

        elink = new EffectLink();
        elink->num = script_h.readInt();

        last_effect_link->next = elink;
        last_effect_link = last_effect_link->next;
    }
    
    readEffect( elink );

    return RET_CONTINUE;
}

int ScriptParser::divCommand()
{
    int val1 = script_h.readInt();
    script_h.pushVariable();

    int val2 = script_h.readInt();
    script_h.setInt( &script_h.pushed_variable, val1/val2 );

    return RET_CONTINUE;
}

int ScriptParser::dimCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "dim: not in the define section" );

    script_h.declareDim();
    
    return RET_CONTINUE;
}

int ScriptParser::defvoicevolCommand()
{
    voice_volume = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::defsubCommand()
{
    last_user_func->next = new UserFuncLUT();
    setStr( &last_user_func->next->command, script_h.readLabel() );
    last_user_func = last_user_func->next;
    
    return RET_CONTINUE;
}

int ScriptParser::defsevolCommand()
{
    se_volume = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::defmp3volCommand()
{
    mp3_volume = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::defaultspeedCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "defaultspeed: not in the define section" );

    for ( int i=0 ; i<3 ; i++ ) default_text_speed[i] = script_h.readInt();

    return RET_CONTINUE;
}

int ScriptParser::defaultfontCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "defaultfont: not in the define section" );

    setStr( &default_env_font, script_h.readStr() );

    return RET_CONTINUE;
}

int ScriptParser::decCommand()
{
    int val = script_h.readInt();
    script_h.setInt( &script_h.current_variable, val-1 );

    return RET_CONTINUE;
}

int ScriptParser::dateCommand()
{
    time_t t = time(NULL);
    struct tm *tm = localtime( &t );

    script_h.readInt();
    script_h.setInt( &script_h.current_variable, tm->tm_year % 100 );

    script_h.readInt();
    script_h.setInt( &script_h.current_variable, tm->tm_mon + 1 );

    script_h.readInt();
    script_h.setInt( &script_h.current_variable, tm->tm_mday );

    return RET_CONTINUE;
}

int ScriptParser::cmpCommand()
{
    script_h.readInt();
    script_h.pushVariable();
    
    script_h.readStr();
    char *save_buf = script_h.saveStringBuffer();

    const char *buf = script_h.readStr();

    int val = strcmp( save_buf, buf );
    if      ( val > 0 ) val = 1;
    else if ( val < 0 ) val = -1;
    script_h.setInt( &script_h.pushed_variable, val );

    return RET_CONTINUE;
}

int ScriptParser::clickvoiceCommand()
{
    if ( current_mode != DEFINE_MODE ) errorAndExit( "clickvoice: not in the define section" );

    for ( int i=0 ; i<CLICKVOICE_NUM ; i++ ){
        const char *buf = script_h.readStr();
        setStr( &clickvoice_file_name[i], buf );
    }

    return RET_CONTINUE;
}

int ScriptParser::clickstrCommand()
{
    script_h.readStr();
    const char *buf = script_h.saveStringBuffer();

    clickstr_line = script_h.readInt();

    script_h.setClickstr( buf );
           
    return RET_CONTINUE;
}

int ScriptParser::breakCommand()
{
    char *buf = script_h.getNext();
    if ( buf[0] == '*' ){
        setCurrentLinkLabel( script_h.readLabel()+1 );
    }
    else{
        break_flag = true;
    }
    
    return RET_CONTINUE;
}

int ScriptParser::atoiCommand()
{
    script_h.readInt();
    script_h.pushVariable();
    
    const char *buf = script_h.readStr();
        
    script_h.setInt( &script_h.pushed_variable, atoi(buf) );
    
    return RET_CONTINUE;
}

int ScriptParser::arcCommand()
{
    const char *buf = script_h.readStr();
    char *buf2 = new char[ strlen( buf ) + 1 ];
    strcpy( buf2, buf );

    int i = 0;
    while ( buf2[i] != '|' && buf2[i] != '\0' ) i++;
    buf2[i] = '\0';

    if ( strcmp( script_h.cBR->getArchiveName(), "direct" ) == 0 ){
        delete script_h.cBR;
        script_h.cBR = new SarReader( archive_path, key_table );
        if ( script_h.cBR->open( buf2 ) ){
            fprintf( stderr, " *** failed to open archive %s, continuing ...  ***\n", buf2 );
        }
    }
    else if ( strcmp( script_h.cBR->getArchiveName(), "sar" ) == 0 ){
        if ( script_h.cBR->open( buf2 ) ){
            fprintf( stderr, " *** failed to open archive %s, continuing ...  ***\n", buf2 );
        }
    }
    // skip "arc" commands after "ns?" command
    
    delete[] buf2;
    
    return RET_CONTINUE;
}

int ScriptParser::addCommand()
{
    script_h.readVariable();
    
    if ( script_h.current_variable.type == ScriptHandler::VAR_INT ||
         script_h.current_variable.type == ScriptHandler::VAR_ARRAY ){
        int val = script_h.getIntVariable( &script_h.current_variable );
        script_h.pushVariable();

        script_h.setInt( &script_h.pushed_variable, val+script_h.readInt() );
    }
    else if ( script_h.current_variable.type == ScriptHandler::VAR_STR ){
        int no = script_h.current_variable.var_no;

        const char *buf = script_h.readStr();
        char *tmp_buffer = script_h.str_variables[no];

        if ( tmp_buffer ){
            script_h.str_variables[ no ] = new char[ strlen( tmp_buffer ) + strlen( buf ) + 1 ];
            strcpy( script_h.str_variables[ no ], tmp_buffer );
            strcat( script_h.str_variables[ no ], buf );
            delete[] tmp_buffer;
        }
        else{
            script_h.str_variables[ no ] = new char[ strlen( buf ) + 1 ];
            strcpy( script_h.str_variables[ no ], buf );
        }
    }
    else errorAndExit( "add: no variable." );

    return RET_CONTINUE;
}
