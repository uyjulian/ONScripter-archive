/* -*- C++ -*-
 * 
 *  ONScripterLabel_command.cpp - Command executer of ONScripter
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

#include "ONScripterLabel.h"

#define DEFAULT_WAVE_CHANNEL 1
#define ONSCRIPTER_VERSION 198

#define DEFAULT_CURSOR_WAIT    ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

#define CONTINUOUS_PLAY

int ONScripterLabel::waveCommand()
{
    char *p_string_buffer;

    if ( !strncmp( string_buffer + string_buffer_offset, "waveloop", 8 ) ){
        wave_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 8;
    }
    else{
        wave_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 4;
    }

    wavestopCommand();

    readStr( &p_string_buffer, tmp_string_buffer );
    playWave( tmp_string_buffer, wave_play_once_flag, DEFAULT_WAVE_CHANNEL );
        
    return RET_CONTINUE;
}

int ONScripterLabel::wavestopCommand()
{
    if ( wave_sample[DEFAULT_WAVE_CHANNEL] ){
        Mix_Pause( DEFAULT_WAVE_CHANNEL );
        Mix_FreeChunk( wave_sample[DEFAULT_WAVE_CHANNEL] );
        wave_sample[DEFAULT_WAVE_CHANNEL] = NULL;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::waittimerCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;

    int count = readInt( &p_string_buffer ) + internal_timer - SDL_GetTicks();
    startTimer( count );
    
    return RET_WAIT_NEXT;
}

int ONScripterLabel::waitCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4;

    startTimer( readInt( &p_string_buffer ) );

    return RET_WAIT_NEXT;
}

int ONScripterLabel::vspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3;

    int no = readInt( &p_string_buffer );
    int v  = readInt( &p_string_buffer );
    sprite_info[ no ].valid = (v==1)?true:false;
    
    return RET_CONTINUE;
}

int ONScripterLabel::voicevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;
    voice_volume = readInt( &p_string_buffer );

    if ( wave_sample[0] ) Mix_Volume( 0, se_volume * 128 / 100 );
    
    return RET_CONTINUE;
}

int ONScripterLabel::trapCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4;
    readStr( &p_string_buffer, tmp_string_buffer );

    if ( tmp_string_buffer[0] == '*' ){
        trap_flag = true;
        if ( trap_dist ) delete[] trap_dist;
        trap_dist = new char[ strlen( tmp_string_buffer ) ];
        memcpy( trap_dist, tmp_string_buffer + 1, strlen( tmp_string_buffer ) );
    }
    else if ( !strcmp( tmp_string_buffer, "off" ) ){
        trap_flag = false;
    }
    else{
        printf("[%s] is not supported\n", string_buffer + string_buffer_offset );
    }
              
    return RET_CONTINUE;
}

int ONScripterLabel::textspeedCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;

    sentence_font.wait_time = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::textonCommand()
{
    text_on_flag = true;
    if ( !(display_mode & TEXT_DISPLAY_MODE) ){
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        restoreTextBuffer();
        flush();
        display_mode = TEXT_DISPLAY_MODE;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::textoffCommand()
{
    text_on_flag = false;
    if ( display_mode & TEXT_DISPLAY_MODE ){
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        flush();
        display_mode = NORMAL_DISPLAY_MODE;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::textclearCommand()
{
    newPage( false );
    return RET_CONTINUE;
}

int ONScripterLabel::texecCommand()
{
    if ( clickstr_state == CLICK_NEWPAGE ){
        new_line_skip_flag = true;
        newPage( true );
    }
    else{
        sentence_font.xy[0] = 0;
        sentence_font.xy[1]++;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::tablegotoCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;
    int count = 0;
    
    int no = readInt( &p_string_buffer );

    while( end_with_comma_flag ){
        readStr( &p_string_buffer, tmp_string_buffer );
        if ( count++ == no ){
            current_link_label_info->label_info = lookupLabel( tmp_string_buffer + 1 );
            current_link_label_info->current_line = 0;
            current_link_label_info->offset = 0;
    
            return RET_JUMP;
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::systemcallCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10;

    readStr( &p_string_buffer, tmp_string_buffer );
    system_menu_mode = getSystemCallNo( tmp_string_buffer );
    event_mode = WAIT_SLEEP_MODE;

    enterSystemCall();
    
    startTimer( MINIMUM_TIMER_RESOLUTION );
    return RET_WAIT_NEXT;
}

int ONScripterLabel::stopCommand()
{
    wavestopCommand();
    stopBGM( false );
    
    return RET_CONTINUE;
}

int ONScripterLabel::spstrCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;

    readStr( &p_string_buffer, tmp_string_buffer );
    decodeExbtnControl( NULL, tmp_string_buffer, false, true );
    
    return RET_CONTINUE;
}

int ONScripterLabel::spbtnCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;

    int sprite_no = readInt( &p_string_buffer );
    int no        = readInt( &p_string_buffer );

    if ( sprite_info[ sprite_no ].num_of_cells == 0 ) return RET_CONTINUE;

    last_button_link->next = new ButtonLink();
    last_button_link = last_button_link->next;

    last_button_link->button_type = SPRITE_BUTTON;
    last_button_link->sprite_no   = sprite_no;
    last_button_link->no          = no;

    if ( sprite_info[ sprite_no ].image_surface || sprite_info[ sprite_no ].trans_mode == AnimationInfo::TRANS_STRING ){
        last_button_link->image_rect = last_button_link->select_rect = sprite_info[ last_button_link->sprite_no ].pos;
        sprite_info[ sprite_no ].valid = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::skipoffCommand() 
{ 
    skip_flag = false; 
 
    return RET_CONTINUE; 
} 

int ONScripterLabel::sevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;
    se_volume = readInt( &p_string_buffer );

    for ( int i=1 ; i<MIX_CHANNELS ; i++ )
        if ( wave_sample[i] ) Mix_Volume( i, se_volume * 128 / 100 );
        
    return RET_CONTINUE;
}

int ONScripterLabel::setwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;

    sentence_font.font_valid_flag = false;
    sentence_font.top_xy[0] = readInt( &p_string_buffer );
    sentence_font.top_xy[1] = readInt( &p_string_buffer );
    sentence_font.num_xy[0] = readInt( &p_string_buffer );
    sentence_font.num_xy[1] = readInt( &p_string_buffer );
    sentence_font.font_size_xy[0] = readInt( &p_string_buffer );
    sentence_font.font_size_xy[1] = readInt( &p_string_buffer );
    sentence_font.pitch_xy[0] = readInt( &p_string_buffer ) + sentence_font.font_size_xy[0];
    sentence_font.pitch_xy[1] = readInt( &p_string_buffer ) + sentence_font.font_size_xy[1];
    sentence_font.wait_time = readInt( &p_string_buffer );
    sentence_font.display_bold = readInt( &p_string_buffer )?true:false;
    sentence_font.display_shadow = readInt( &p_string_buffer )?true:false;

    if ( sentence_font.ttf_font ) TTF_CloseFont( (TTF_Font*)sentence_font.ttf_font );
    int font_size = (sentence_font.font_size_xy[0] < sentence_font.font_size_xy[1])?
        sentence_font.font_size_xy[0]:sentence_font.font_size_xy[1];
    sentence_font.ttf_font = (void*)TTF_OpenFont( font_file, font_size * screen_ratio1 / screen_ratio2 );

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] == '#' ){
        sentence_font.display_transparency = true;
        if ( strlen( tmp_string_buffer ) != 7 ) errorAndExit( string_buffer + string_buffer_offset );
        readColor( &sentence_font.window_color, tmp_string_buffer + 1 );

        sentence_font_info.pos.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.w = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.x + 1;
        sentence_font_info.pos.h = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.y + 1;
    }
    else{
        sentence_font.display_transparency = false;
        sentence_font_info.setImageName( tmp_string_buffer );
        parseTaggedString( &sentence_font_info );
        setupAnimationInfo( &sentence_font_info );
        sentence_font_info.pos.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    }

    lookbackflushCommand();
    clearCurrentTextBuffer();
    display_mode = NORMAL_DISPLAY_MODE;
    
    return RET_CONTINUE;
}

int ONScripterLabel::setcursorCommand()
{
    char *p_string_buffer;
    bool abs_flag;

    if ( !strncmp( string_buffer + string_buffer_offset, "abssetcursor", 12 ) ) {
        abs_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 12;
    }
    else{
        abs_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 9;
    }
    
    int no = readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    char *buf = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( buf, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    int x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    int y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;

    loadCursor( no, buf, x, y, abs_flag );
    delete[] buf;
    
    return RET_CONTINUE;
}

int ONScripterLabel::selectCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    int xy[2];
    char *p_script_buffer = current_link_label_info->current_script;
    readLine( &p_script_buffer );
    line_cache = -1;
    int select_mode;
    SelectLink *last_select_link;
    char *p_string_buffer, *p_buf;

    if ( !strncmp( string_buffer + string_buffer_offset, "selnum", 6 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 6;
        select_mode = SELECT_NUM_MODE;
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "selgosub", 8 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 8;
        select_mode = SELECT_GOSUB_MODE;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "select", 6 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 6;
        select_mode = SELECT_GOTO_MODE;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "csel", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4;
        select_mode = SELECT_CSEL_MODE;
    }

    bool comma_flag, first_token_flag = true;
    int count = 0;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return RET_WAIT;
        
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        int counter = 1;
        last_select_link = root_select_link.next;
        while ( last_select_link ){
            if ( current_button_state.button == counter++ ) break;
            last_select_link = last_select_link->next;
        }

        if ( select_mode  == SELECT_GOTO_MODE ){
            current_link_label_info->label_info = lookupLabel( last_select_link->label );
            current_link_label_info->current_line = 0;
            current_link_label_info->offset = 0;
            ret = RET_JUMP;
        }
        else if ( select_mode == SELECT_GOSUB_MODE ){
            current_link_label_info->current_line = select_label_info.current_line;
            string_buffer_offset = select_label_info.offset;

            gosubReal( last_select_link->label );
            ret = RET_JUMP;
        }
        else{
            setInt( p_buf, current_button_state.button - 1 );
            ret = RET_CONTINUE;
        }
        deleteSelectLink();

        newPage( true );

        return ret;
    }
    else{
        if ( select_mode == SELECT_CSEL_MODE ){
            shelter_soveon_flag = saveon_flag;
            saveoffCommand();
        }
        SelectLink *link;
        shortcut_mouse_line = -1;
        flush();
        skip_flag = false;
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];

        last_select_link = &root_select_link;
        select_label_info.current_line = current_link_label_info->current_line;
        while(1){
            comma_flag = readStr( &p_string_buffer, tmp_string_buffer );
            //printf("sel [%s] [%s]\n", p_string_buffer, tmp_string_buffer );
            if ( tmp_string_buffer[0] != '\0' || comma_flag ){
                first_token_flag = false;
                count++;
                if ( select_mode == SELECT_NUM_MODE || count % 2 ){
                    if ( select_mode != SELECT_NUM_MODE && !comma_flag ) errorAndExit( string_buffer + string_buffer_offset, "comma is needed here" );
                    link = new SelectLink();
                    setStr( &link->text, tmp_string_buffer );
                    //printf("Select text %s\n", link->text);
                }
                if ( select_mode == SELECT_NUM_MODE || !(count % 2) ){
                    setStr( &link->label, tmp_string_buffer+1 );
                    //printf("Select label %s\n", link->label );
                    last_select_link->next = link;
                    last_select_link = last_select_link->next;
                }
            }

            if ( p_string_buffer[0] == '\0' ){ // end of line
                text_line_flag = false;
                if ( first_token_flag ) comma_flag = true;

                do{
                    readLine( &p_script_buffer, true );
                    select_label_info.current_line++;
                }
                while ( string_buffer[ string_buffer_offset ] == ';' || string_buffer[ string_buffer_offset ] == '\0' );
                
                p_string_buffer = string_buffer + string_buffer_offset;
                if ( *p_string_buffer == ',' ){
                    if ( comma_flag ) errorAndExit( string_buffer + string_buffer_offset, "double comma" );
                    p_string_buffer++;
                }
                else if ( !comma_flag ) break;
            }
            else if (!comma_flag ) break;
            
            if ( first_token_flag ) first_token_flag = false;
        }
        select_label_info.offset = p_string_buffer - string_buffer;

        if ( select_mode != SELECT_CSEL_MODE ){
            last_select_link = root_select_link.next;
            int counter = 1;
            while( last_select_link ){
                if ( *last_select_link->text ){
                    last_button_link->next = getSelectableSentence( last_select_link->text, &sentence_font );
                    last_button_link = last_button_link->next;
                    last_button_link->no = counter;
                }
                counter++;
                last_select_link = last_select_link->next;
            }
            SDL_BlitSurface( text_surface, NULL, select_surface, NULL );
        }

        if ( select_mode == SELECT_GOTO_MODE || select_mode == SELECT_CSEL_MODE ){ /* Resume */
            p_script_buffer = current_link_label_info->current_script;
            readLine( &p_script_buffer );

            if ( select_mode == SELECT_CSEL_MODE ){
                current_link_label_info->label_info = lookupLabel( "customsel" );
                current_link_label_info->current_line = 0;
                current_link_label_info->offset = 0;

                return RET_JUMP;
            }
        }
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();

        return RET_WAIT;
    }
}

int ONScripterLabel::savetimeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;

    int no = readInt( &p_string_buffer ) - 1;
    
    searchSaveFiles();

    if ( !save_file_info[no].valid ){
        setInt( p_string_buffer, 0 );
        return RET_CONTINUE;
    }

    setInt( p_string_buffer, save_file_info[no].month );
    readInt( &p_string_buffer );
    setInt( p_string_buffer, save_file_info[no].day );
    readInt( &p_string_buffer );
    setInt( p_string_buffer, save_file_info[no].hour );
    readInt( &p_string_buffer );
    setInt( p_string_buffer, save_file_info[no].minute );
    readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::saveonCommand()
{
    saveon_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::saveoffCommand()
{
    if ( saveon_flag ){
        saveSaveFile( -1 );
    }
    
    saveon_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::savegameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;

    int no = readInt( &p_string_buffer );
    if ( no < 0 )
        errorAndExit( string_buffer + string_buffer_offset );
    else{
        shelter_event_mode = event_mode;
        saveSaveFile( no );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::rndCommand()
{
    char *p_string_buffer, *p_buf;
    int  upper, lower;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "rnd2", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4;
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
        lower = readInt( &p_string_buffer );
        upper = readInt( &p_string_buffer );
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 3;
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
        lower = 0;
        upper = readInt( &p_string_buffer ) - 1;
    }

    setInt( p_buf, lower + (int)( (double)(upper-lower+1)*rand()/(RAND_MAX+1.0)) );

    return RET_CONTINUE;
}

int ONScripterLabel::rmodeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;

    if ( readInt( &p_string_buffer ) == 1 ) rmode_flag = true;
    else                                    rmode_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::resettimerCommand()
{
    internal_timer = SDL_GetTicks();
    return RET_CONTINUE;
}

int ONScripterLabel::resetCommand()
{
    for ( int i=0 ; i<199 ; i++ ){
        num_variables[i] = 0;
        if ( str_variables[i] ) delete[] str_variables[i];
        str_variables[i] = NULL;
    }

    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;
    text_char_flag = false;
    skip_flag      = false;
    monocro_flag   = false;
    saveon_flag    = true;
    
    deleteLabelLink();
    current_link_label_info->label_info = lookupLabel( "start" );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;

    barclearCommand();
    prnumclearCommand();

    deleteButtonLink();
    deleteSelectLink();

    wavestopCommand();
    stopBGM( false );
    
    SDL_FillRect( background_surface, NULL, SDL_MapRGBA( background_surface->format, 0, 0, 0, 0 ) );
    SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );

    return RET_JUMP;
}

int ONScripterLabel::repaintCommand()
{
    refreshSurface( accumulation_surface, NULL, REFRESH_SHADOW_MODE );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
    restoreTextBuffer();
    flush();
    
    return RET_CONTINUE;
}

int ONScripterLabel::quakeCommand()
{
    char *p_string_buffer;
    int quake_type;

    if ( !strncmp( string_buffer + string_buffer_offset, "quakey", 6 ) ){
        quake_type = 0;
        p_string_buffer = string_buffer + string_buffer_offset + 6;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "quakex", 6 ) ){
        quake_type = 1;
        p_string_buffer = string_buffer + string_buffer_offset + 6;
    }
    else{
        quake_type = 2;
        p_string_buffer = string_buffer + string_buffer_offset + 5;
    }

    quake_effect.num      = readInt( &p_string_buffer );
    quake_effect.duration = readInt( &p_string_buffer );
    if ( quake_effect.duration < quake_effect.num * 4 ) quake_effect.duration = quake_effect.num * 4;
    
    if ( event_mode & EFFECT_EVENT_MODE ){
        if ( effect_counter == 0 ){
            SDL_BlitSurface( text_surface, NULL, effect_src_surface, NULL );
            SDL_BlitSurface( text_surface, NULL, effect_dst_surface, NULL );
        }
        quake_effect.effect = CUSTOM_EFFECT_NO + quake_type;
        return doEffect( QUAKE_EFFECT, NULL, DIRECT_EFFECT_IMAGE );
    }
    else{
        char *buf = new char[512];
        if ( quake_type == 0 )      sprintf( buf, "quakey %d, %d", quake_effect.num, quake_effect.duration );
        else if ( quake_type == 1 ) sprintf( buf, "quakex %d, %d", quake_effect.num, quake_effect.duration );
        else if ( quake_type == 2 ) sprintf( buf, "quake %d, %d",  quake_effect.num, quake_effect.duration );
        setEffect( 2, buf ); // 2 is dummy value
        return RET_WAIT_NEXT;
    }
}

int ONScripterLabel::puttextCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    readStr( &p_string_buffer, tmp_string_buffer );

    drawString( tmp_string_buffer, sentence_font.color, &sentence_font, false, text_surface, NULL, true );
    flush();

    return RET_CONTINUE;
}

int ONScripterLabel::prnumclearCommand()
{
    for ( int i=0 ; i<MAX_PARAM_NUM ; i++ ) {
        if ( prnum_info[i] ) {
            delete prnum_info[i];
            prnum_info[i] = NULL;
        }
    }
    return RET_CONTINUE;
}

int ONScripterLabel::prnumCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;
    char num_buf[12], buf[7];
    int ptr = 0;

    int no = readInt( &p_string_buffer );
    if ( prnum_info[no] ) delete prnum_info[no];
    prnum_info[no] = new AnimationInfo();
    prnum_info[no]->trans_mode = AnimationInfo::TRANS_STRING;
    prnum_info[no]->num_of_cells = 1;
    prnum_info[no]->current_cell = 0;
    prnum_info[no]->color_list = new uchar3[ prnum_info[no]->num_of_cells ];
    
    int param = readInt( &p_string_buffer );
    prnum_info[no]->pos.x = readInt( &p_string_buffer );
    prnum_info[no]->pos.y = readInt( &p_string_buffer );
    prnum_info[no]->font_size_xy[0] = readInt( &p_string_buffer );
    prnum_info[no]->font_size_xy[1] = readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset, "Color is not specified." );
    readColor( &prnum_info[no]->color_list[0], tmp_string_buffer + 1 );

    sprintf( num_buf, "%3d", param );
    if ( param<0 ){
        if ( param>-10 ) {
            buf[ptr++] = "Å@"[0];
            buf[ptr++] = "Å@"[1];
        }
        buf[ptr++] = "Å|"[0];
        buf[ptr++] = "Å|"[1];
        sprintf( num_buf, "%d", -param );
    }
    for ( int i=0 ; i<(int)strlen( num_buf ) ; i++ ){
        if ( num_buf[i] == ' ' ) {
            buf[ptr++] = "Å@"[0];
            buf[ptr++] = "Å@"[1];
            continue;
        }
        getSJISFromInteger( &buf[ptr], num_buf[i] - '0', false );
        ptr += 2;
        if ( ptr >= 6 ) break; // up to 3 columns (NScripter's restriction)
    }
    setStr( &prnum_info[no]->file_name, buf );

    setupAnimationInfo( prnum_info[no] );

    return RET_CONTINUE;
}

int ONScripterLabel::printCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;

    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &print_effect );
        if ( num > 1 ) return doEffect( PRINT_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        else           return doEffect( print_effect.effect, NULL, TACHI_EFFECT_IMAGE );
    }
    else{
        char *buf = new char[512];
        sprintf( buf, "print " );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::playstopCommand()
{
    stopBGM( false );
    return RET_CONTINUE;
}

int ONScripterLabel::playCommand()
{
    char *p_string_buffer;

    if ( !strncmp( string_buffer + string_buffer_offset, "playonce", 8 ) ){
        music_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 8;
    }
    else{
        music_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 4;
    }

    readStr( &p_string_buffer, tmp_string_buffer );

    if ( tmp_string_buffer[0] == '*' ){
        int new_cd_track = atoi( tmp_string_buffer + 1 );
#ifdef CONTINUOUS_PLAY        
        if ( current_cd_track != new_cd_track ) {
#endif        
            stopBGM( false );
            current_cd_track = new_cd_track;

            if ( cdaudio_flag ){
                if ( cdrom_info ) playCDAudio( current_cd_track );
            }
            else{
                playMP3( current_cd_track );
            }
#ifdef CONTINUOUS_PLAY        
        }
#endif
    }
    else{ // play MIDI
        stopBGM( false );
        
        setStr( &music_file_name, tmp_string_buffer );
        playMIDIFile();
    }

    return RET_CONTINUE;
}

int ONScripterLabel::mspCommand()
{
    int no;
    char *p_string_buffer;

    p_string_buffer = string_buffer + string_buffer_offset + 3;

    no = readInt( &p_string_buffer );
    sprite_info[ no ].pos.x += readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    sprite_info[ no ].pos.y += readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    sprite_info[ no ].trans += readInt( &p_string_buffer );
    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;

    return RET_CONTINUE;
}

int ONScripterLabel::mp3volCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;
    mp3_volume = readInt( &p_string_buffer );

    if ( mp3_sample ) SMPEG_setvolume( mp3_sample, mp3_volume );

    return RET_CONTINUE;
}

int ONScripterLabel::mp3Command()
{
    char *p_string_buffer;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "mp3save", 7 ) ){
        music_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 7;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "mp3loop", 7 ) ){
        music_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 7;
    }
    else{
        music_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 3;
    }

    stopBGM( false );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    setStr( &music_file_name, tmp_string_buffer );

    playMP3( 0 );
        
    return RET_CONTINUE;
}

int ONScripterLabel::monocroCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7;

    readStr( &p_string_buffer, tmp_string_buffer );

    if ( !strcmp( tmp_string_buffer, "off" ) ){
        monocro_flag_new = false;
    }
    else if ( tmp_string_buffer[0] != '#' ){
        errorAndExit( string_buffer + string_buffer_offset );
    }
    else{
        monocro_flag_new = true;
        readColor( &monocro_color_new, tmp_string_buffer + 1 );
    }
    need_refresh_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::menu_windowCommand()
{
    if ( fullscreen_mode ){
        if ( SDL_WM_ToggleFullScreen( screen_surface ) ) fullscreen_mode = false;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::menu_fullCommand()
{
    if ( !fullscreen_mode ){
        if ( SDL_WM_ToggleFullScreen( screen_surface ) ) fullscreen_mode = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::lspCommand()
{
    bool v;
    int no;
    char *p_string_buffer;

    if ( !strncmp( string_buffer + string_buffer_offset, "lsph", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4;
        v = false;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 3;
        v = true;
    }

    no = readInt( &p_string_buffer );
    sprite_info[ no ].valid = v;

    readStr( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].setImageName( tmp_string_buffer );

    sprite_info[ no ].pos.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    sprite_info[ no ].pos.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;

    if ( end_with_comma_flag )
        sprite_info[ no ].trans = readInt( &p_string_buffer );
    else
        sprite_info[ no ].trans = 255;

    parseTaggedString( &sprite_info[ no ] );
    setupAnimationInfo( &sprite_info[ no ] );

    return RET_CONTINUE;
}

int ONScripterLabel::lookbackflushCommand()
{
    int i;
    
    current_text_buffer = current_text_buffer->next;
    for ( i=0 ; i<MAX_TEXT_BUFFER-1 ; i++ ){
        current_text_buffer->xy[1] = -1;
        current_text_buffer = current_text_buffer->next;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::locateCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;

    sentence_font.xy[0] = readInt( &p_string_buffer );
    sentence_font.xy[1] = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::loadgameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;
    
    int no = readInt( &p_string_buffer );

    if ( no < 0 )
        errorAndExit( string_buffer + string_buffer_offset );

    if ( loadSaveFile( no ) ) return RET_CONTINUE;
    else {
        skip_flag = false;
        deleteButtonLink();
        deleteSelectLink();
        key_pressed_flag = false;
        saveon_flag = true;
        if ( event_mode & WAIT_INPUT_MODE ) return RET_WAIT;
        return RET_JUMP;
    }
}

int ONScripterLabel::ldCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2;
    
    readStr( &p_string_buffer, tmp_string_buffer );
    char loc = tmp_string_buffer[0];

    readStr( &p_string_buffer, tmp_string_buffer );
    
    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        else           return doEffect( tmp_effect.effect, NULL, TACHI_EFFECT_IMAGE );
    }
    else{
        int no;

        if      ( loc == 'l' ) no = 0;
        else if ( loc == 'c' ) no = 1;
        else if ( loc == 'r' ) no = 2;
        
        tachi_info[ no ].setImageName( tmp_string_buffer );
        parseTaggedString( &tachi_info[ no ] );
        setupAnimationInfo( &tachi_info[ no ] );
        if ( tachi_info[ no ].image_surface )
            tachi_info[ no ].valid = true;
        
        tachi_info[ no ].pos.x = screen_width * (no+1) / 4 - tachi_info[ no ].pos.w / 2;
        tachi_info[ no ].pos.y = underline_value - tachi_info[ no ].image_surface->h + 1;

        char *buf = new char[512];
        sprintf( buf, "ld %c, \"%s\",", loc, tmp_string_buffer );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::jumpfCommand()
{
    jumpf_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::jumpbCommand()
{
    current_link_label_info->label_info = last_tilde.label_info;
    current_link_label_info->current_line = last_tilde.current_line;
    current_link_label_info->offset = last_tilde.offset;

    return RET_JUMP;
}

int ONScripterLabel::ispageCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;

    if ( clickstr_state == CLICK_NEWPAGE ){
        setInt( p_string_buffer, 1 );
    }
    else{
        setInt( p_string_buffer, 0 );
    }
    
    return RET_CONTINUE;
}

int ONScripterLabel::isdownCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;

    if ( current_button_state.down_flag ){
        setInt( p_string_buffer, 1 );
    }
    else{
        setInt( p_string_buffer, 0 );
    }
    
    return RET_CONTINUE;
}

int ONScripterLabel::getversionCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10;
    
    setInt( p_string_buffer, ONSCRIPTER_VERSION );

    return RET_CONTINUE;
}

int ONScripterLabel::gettimerCommand()
{
    char *p_string_buffer; 
 
    if ( !strncmp( string_buffer + string_buffer_offset, "gettimer", 8 ) ){ 
        p_string_buffer = string_buffer + string_buffer_offset + 8;
        setInt( p_string_buffer, SDL_GetTicks() - internal_timer );
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "getbtntimer", 11 ) ){ 
        p_string_buffer = string_buffer + string_buffer_offset + 11;
        setInt( p_string_buffer, btnwait_time );
    }
 
    return RET_CONTINUE; 
}

int ONScripterLabel::getregCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;
    char buf[256], buf2[256], *p_buf;
    FILE *fp;
    bool found_flag = false;
    char *path = NULL;
    
    SKIP_SPACE( p_string_buffer );
    
    if ( p_string_buffer[0] != '$') errorAndExit( string_buffer + string_buffer_offset, "no string variable" );
    p_string_buffer++;
    int no = readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    setStr( &path , tmp_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );

    printf("  reading Registry file for [%s] %s\n", path, tmp_string_buffer );
        
    if ( ( fp = fopen( registry_file, "r" ) ) == NULL ){
        fprintf( stderr, "Cannot open file [%s]\n", registry_file );
        return RET_CONTINUE;
    }
    while( fgets( buf, 256, fp) && !found_flag ){
        if ( buf[0] == '[' ){
            unsigned int c=0;
            while ( buf[c] != ']' && buf[c] != '\0' ) c++;
            if ( !strncmp( buf + 1, path, (c-1>strlen(path))?(c-1):strlen(path) ) ){
                while( fgets( buf, 256, fp) ){
                    p_buf = buf+1;
                    readStr( &p_buf, buf2 );
                    if ( strncmp( buf2,
                                  tmp_string_buffer,
                                  (strlen(buf2)>strlen(tmp_string_buffer))?strlen(buf2):strlen(tmp_string_buffer) ) ) continue;
                    
                    readStr( &p_buf, buf2 );
                    if ( buf2[0] != '=' ) continue;

                    readStr( &p_buf, buf2 );
                    setStr( &str_variables[ no ], buf2 );
                    printf("  $%d = %s\n", no, str_variables[ no ] );
                    found_flag = true;
                    break;
                }
            }
        }
    }

    if ( !found_flag ) fprintf( stderr, "  The key is not found.\n" );
    fclose(fp);

    return RET_CONTINUE;
}

int ONScripterLabel::getmouseposCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 11;
    char *p_buf = p_string_buffer;

    readInt( &p_string_buffer );
    setInt( p_buf, current_button_state.x * screen_ratio2 / screen_ratio1 );
    
    p_buf = p_string_buffer;
    setInt( p_buf, current_button_state.y * screen_ratio2 / screen_ratio1 );
    
    return RET_CONTINUE;
}

int ONScripterLabel::getcursorposCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 12;
    char *p_buf = p_string_buffer;

    readInt( &p_string_buffer );
    setInt( p_buf, sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] );
    
    p_buf = p_string_buffer;
    setInt( p_buf, sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] );
    
    return RET_CONTINUE;
}

int ONScripterLabel::getcselnumCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10;
    int count = 0;

    SelectLink *link = root_select_link.next;
    while ( link ) {
        count++;
        link = link->next;
    }
    setInt( p_string_buffer, count );
    //printf("getcselnum num=%d\n", count);

    return RET_CONTINUE;
}

int ONScripterLabel::gameCommand()
{
    current_link_label_info->label_info = lookupLabel( "start" );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;
    current_mode = NORMAL_MODE;

    sentence_font.wait_time = default_text_speed[text_speed_no];

    /* ---------------------------------------- */
    /* Load default cursor */
    loadCursor( CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0 );
    loadCursor( CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0 );
    
    /* ---------------------------------------- */
    /* Lookback related variables */
    for ( int i=0 ; i<4 ; i++ ){
        setStr( &lookback_info[i].image_name, lookback_image_name[i] );
        parseTaggedString( &lookback_info[i] );
        setupAnimationInfo( &lookback_info[i] );
    }
    
    return RET_JUMP;
}

int ONScripterLabel::exbtnCommand()
{
    int sprite_no = -1, no;
    char *p_string_buffer;
    ButtonLink *button;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "exbtn_d", 7 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 7;
        button = &exbtn_d_button_link;
        if ( button->exbtn_ctl ) delete[] button->exbtn_ctl;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 5;
        sprite_no = readInt( &p_string_buffer );
        no = readInt( &p_string_buffer );

        if ( sprite_info[ sprite_no ].num_of_cells == 0 ) return RET_CONTINUE;
        
        button = new ButtonLink();
        last_button_link->next = button;
        last_button_link = last_button_link->next;
    }
    //printf("exbtnCommand %s\n",string_buffer + string_buffer_offset);
    readStr( &p_string_buffer, tmp_string_buffer );

    //if ( !sprite_info[ sprite_no ].valid ) return RET_CONTINUE;

    button->button_type = EX_SPRITE_BUTTON;
    button->sprite_no   = sprite_no;
    button->no          = no;
    button->exbtn_ctl   = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( button->exbtn_ctl, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    
    if ( sprite_no >= 0 &&
         ( sprite_info[ sprite_no ].image_surface || sprite_info[ sprite_no ].trans_mode == AnimationInfo::TRANS_STRING ) ){
        button->image_rect = button->select_rect = sprite_info[ button->sprite_no ].pos;
        sprite_info[ sprite_no ].valid = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::erasetextwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 15;

    erase_text_window_mode = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::endCommand()
{
    saveGlovalData();
    saveFileLog();
    saveLabelLog();
    if ( cdrom_info ){
        SDL_CDStop( cdrom_info );
        SDL_CDClose( cdrom_info );
    }
    if ( midi_info ){
        Mix_HaltMusic();
        Mix_FreeMusic( midi_info );
    }
    SDL_Quit();
    exit(0);
}

int ONScripterLabel::dwavestopCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;
    int ch = readInt( &p_string_buffer );

    if ( wave_sample[ch] ){
        Mix_Pause( ch );
        Mix_FreeChunk( wave_sample[ch] );
        wave_sample[ch] = NULL;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::dwaveCommand()
{
    char *p_string_buffer;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "dwaveloop", 9 ) ){
        wave_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 9;
    }
    else{
        wave_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 5;
    }

    wavestopCommand();

    int ch = readInt( &p_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    playWave( tmp_string_buffer, wave_play_once_flag, ch );
        
    return RET_CONTINUE;
}

int ONScripterLabel::delayCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5;

    if ( event_mode & (WAIT_SLEEP_MODE | WAIT_INPUT_MODE ) ){
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        event_mode = WAIT_SLEEP_MODE | WAIT_INPUT_MODE;
        key_pressed_flag = false;
        startTimer( readInt( &p_string_buffer ) );
        return RET_WAIT;
    }
}

int ONScripterLabel::cspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3;
    int no = readInt( &p_string_buffer );

    if ( no == -1 )
        for ( int i=0 ; i<MAX_SPRITE_NUM ; i++ ){
            sprite_info[i].remove();
        }
    else{
        sprite_info[no].remove();
    }

    return RET_CONTINUE;
}

int ONScripterLabel::cselgotoCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8;

    int csel_no = readInt( &p_string_buffer );

    int counter = 0;
    SelectLink *link = root_select_link.next;
    while( link ){
        if ( csel_no == counter++ ) break;
        link = link->next;
    }
    if ( !link ) errorAndExit( string_buffer + string_buffer_offset, "no select link" );

    current_link_label_info->label_info   = lookupLabel( link->label );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset       = 0;
    saveon_flag = shelter_soveon_flag;

    deleteSelectLink();
    newPage( true );
    
    return RET_JUMP;
}

int ONScripterLabel::cselbtnCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7;

    int csel_no   = readInt( &p_string_buffer );
    int button_no = readInt( &p_string_buffer );

    FontInfo csel_info;
    csel_info = sentence_font;
    csel_info.top_xy[0] = readInt( &p_string_buffer );
    csel_info.top_xy[1] = readInt( &p_string_buffer );
    csel_info.xy[0] = csel_info.xy[1] = 0;

    int counter = 0;
    SelectLink *link = root_select_link.next;
    while ( link ){
        if ( csel_no == counter++ ) break;
        link = link->next;
    }
    if ( link == NULL || link->text == NULL || *link->text == '\0' )
        errorAndExit( string_buffer + string_buffer_offset, "no select text" );

    last_button_link->next = getSelectableSentence( link->text, &csel_info );
    last_button_link = last_button_link->next;
    last_button_link->button_type = CUSTOM_SELECT_BUTTON;
    last_button_link->no          = button_no;
    last_button_link->sprite_no   = csel_no;

    sentence_font.font_valid_flag = csel_info.font_valid_flag;
    sentence_font.ttf_font = csel_info.ttf_font;

    SDL_BlitSurface( text_surface, &last_button_link->select_rect, select_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::clickCommand()
{
    if ( event_mode & WAIT_INPUT_MODE ){
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        skip_flag = false;
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        return RET_WAIT;
    }
}

int ONScripterLabel::clCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2;
    
    readStr( &p_string_buffer, tmp_string_buffer );
    char loc = tmp_string_buffer[0];
    
    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        else           return doEffect( tmp_effect.effect, NULL, TACHI_EFFECT_IMAGE );
    }
    else{
        if ( loc == 'l' || loc == 'a' ){
            tachi_info[0].remove();
        }
        if ( loc == 'c' || loc == 'a' ){
            tachi_info[1].remove();
        }
        if ( loc == 'r' || loc == 'a' ){
            tachi_info[2].remove();
        }

        char *buf = new char[512];
        sprintf( buf, "cl %c,", loc );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::cellCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4;
    
    int sprite_no = readInt( &p_string_buffer );
    int no        = readInt( &p_string_buffer );

    if ( sprite_info[ sprite_no ].num_of_cells > 0 )
        sprite_info[ sprite_no ].current_cell = no;
        
    return RET_CONTINUE;
}

int ONScripterLabel::captionCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7;

    readStr( &p_string_buffer, tmp_string_buffer );

#if defined(LINUX) /* convert sjis to euc */
    int i = 0;
    while ( tmp_string_buffer[i] ) {
        if ( (unsigned char)tmp_string_buffer[i] > 0x80 ) {
            unsigned char c1, c2;
            c1 = tmp_string_buffer[i];
            c2 = tmp_string_buffer[i+1];

            c1 -= (c1 <= 0x9f) ? 0x71 : 0xb1;
            c1 = c1 * 2 + 1;
            if (c2 >= 0x9e) {
                c2 -= 0x7e;
                c1++;
            }
            else if (c2 >= 0x80) {
                c2 -= 0x20;
            }
            else {
                c2 -= 0x1f;
            }

            tmp_string_buffer[i]   = c1 | 0x80;
            tmp_string_buffer[i+1] = c2 | 0x80;
            i++;
        }
        i++;
    }
#endif

    setStr( &wm_title_string, tmp_string_buffer );
    setStr( &wm_icon_string,  tmp_string_buffer );

    SDL_WM_SetCaption( wm_title_string, wm_icon_string );

    return RET_CONTINUE;
}


int ONScripterLabel::btndownCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7;
    btndown_flag = (readInt( &p_string_buffer )==1)?true:false;

    return RET_CONTINUE;
}

int ONScripterLabel::btndefCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6;
    readStr( &p_string_buffer, tmp_string_buffer );

    if ( strcmp( tmp_string_buffer, "clear" ) ){
        btndef_info.remove();
        if ( tmp_string_buffer[0] != '\0' ){
            btndef_info.setImageName( tmp_string_buffer );
            parseTaggedString( &btndef_info );
            setupAnimationInfo( &btndef_info );
            SDL_SetAlpha( btndef_info.image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
        }
    }
    
    deleteButtonLink();
    
    return RET_CONTINUE;
}

int ONScripterLabel::btnCommand()
{
    SDL_Rect src_rect;
    
    last_button_link->next = new ButtonLink();
    last_button_link = last_button_link->next;
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 3;
    last_button_link->button_type  = NORMAL_BUTTON;
    last_button_link->no           = readInt( &p_string_buffer );
    last_button_link->image_rect.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    last_button_link->image_rect.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    last_button_link->image_rect.w = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    last_button_link->image_rect.h = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    last_button_link->select_rect = last_button_link->image_rect;

    src_rect.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.w = last_button_link->image_rect.w;
    src_rect.h = last_button_link->image_rect.h;

    last_button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, last_button_link->image_rect.w, last_button_link->image_rect.h, 32, rmask, gmask, bmask, amask );

    SDL_BlitSurface( btndef_info.image_surface, &src_rect, last_button_link->image_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::btnwaitCommand()
{
    char *p_string_buffer;
    bool del_flag, textbtn_flag = false, selectbtn_flag = false;

    if ( !strncmp( string_buffer + string_buffer_offset, "btnwait2", 8 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 8;
        del_flag = false;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "btnwait", 7 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 7;
        del_flag = true;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "textbtnwait", 11 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 11;
        del_flag = false;
        textbtn_flag = true;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "selectbtnwait", 13 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 13;
        del_flag = false;
        selectbtn_flag = true;
    }

    if ( event_mode & WAIT_BUTTON_MODE ){
        btnwait_time = SDL_GetTicks() - internal_button_timer;
        btntime_value = 0;

        if ( textbtn_flag && skip_flag ) current_button_state.button = 0;
        setInt( p_string_buffer, current_button_state.button );

        if ( del_flag ){
            if ( current_button_state.button > 0 ) deleteButtonLink();
            if ( exbtn_d_button_link.exbtn_ctl ){
                delete[] exbtn_d_button_link.exbtn_ctl;
                exbtn_d_button_link.exbtn_ctl = NULL;
            }
        }

        event_mode = IDLE_EVENT_MODE;
        btndown_flag = false;

        return RET_CONTINUE;
    }
    else{
        shortcut_mouse_line = 0;
        skip_flag = false;
        event_mode = WAIT_BUTTON_MODE;
        if ( textbtn_flag ) event_mode |= WAIT_TEXTBTN_MODE;

        ButtonLink *p_button_link = root_button_link.next;
        while( p_button_link ){
            if ( current_button_link.button_type == SPRITE_BUTTON || current_button_link.button_type == EX_SPRITE_BUTTON )
                sprite_info[ current_button_link.sprite_no ].current_cell = 0;
            p_button_link = p_button_link->next;
        }

        refreshSurface( accumulation_surface,
                        NULL,
                        ( erase_text_window_mode == 0 && text_on_flag)?REFRESH_SHADOW_MODE:REFRESH_NORMAL_MODE );
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );

        if ( erase_text_window_mode == 0 && text_on_flag ){
            restoreTextBuffer();
            display_mode = TEXT_DISPLAY_MODE;
        }
        else{
            display_mode = NORMAL_DISPLAY_MODE;
        }
        
        /* ---------------------------------------- */
        /* Resotre csel button */
        FontInfo f_info = sentence_font;
        f_info.xy[0] = 0;
        f_info.xy[1] = 0;
        
        p_button_link = root_button_link.next;
        while ( p_button_link ){
            if ( p_button_link->button_type == CUSTOM_SELECT_BUTTON ){
            
                f_info.xy[0] = f_info.xy[1] = 0;
                f_info.top_xy[0] = p_button_link->image_rect.x * screen_ratio2 / screen_ratio1;
                f_info.top_xy[1] = p_button_link->image_rect.y * screen_ratio2 / screen_ratio1;

                int counter = 0;
                SelectLink *s_link = root_select_link.next;
                while ( s_link ){
                    if ( p_button_link->sprite_no == counter++ ) break;
                    s_link = s_link->next;
                }
            
                drawString( s_link->text, f_info.off_color, &f_info, false, text_surface );
            
            }
            p_button_link = p_button_link->next;
        }
    
        sentence_font.font_valid_flag = f_info.font_valid_flag;
        sentence_font.ttf_font = f_info.ttf_font;
        
        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

        flush();

        refreshMouseOverButton();

        if ( btntime_value > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( btntime_value );
            if ( usewheel_flag ) current_button_state.button = -5;
            else                 current_button_state.button = -2;
        }
        internal_button_timer = SDL_GetTicks();

        if ( textbtn_flag ){
            event_mode |= WAIT_ANIMATION_MODE;
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
        
        return RET_WAIT;
    }
}

int ONScripterLabel::btntimeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7;

    btntime_value = readInt( &p_string_buffer );
    
    return RET_CONTINUE;
}

int ONScripterLabel::brCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    sentence_font.xy[0] = 0;
    sentence_font.xy[1]++;
    text_char_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::bltCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3;
    SDL_Rect src_rect, dst_rect, clip, clipped;

    dst_rect.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    dst_rect.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    dst_rect.w = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    dst_rect.h = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.w = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    src_rect.h = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;

    if ( src_rect.w == dst_rect.w && src_rect.h == dst_rect.h ){

        clip.x = clip.y = 0;
        clip.w = screen_width;
        clip.h = screen_height;
        doClipping( &dst_rect, &clip, &clipped );
        src_rect.x += clipped.x;
        src_rect.y += clipped.y;
        src_rect.w -= clipped.x;
        src_rect.h -= clipped.y;
        
        SDL_BlitSurface( btndef_info.image_surface, &src_rect, screen_surface, &dst_rect );
        SDL_UpdateRect( screen_surface, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h );
    }
    else{
        resizeSurface( btndef_info.image_surface, &src_rect, text_surface, &dst_rect );
        flush( &dst_rect );
    }
    
    return RET_CONTINUE;
}

int ONScripterLabel::bgCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2;
    
    readStr( &p_string_buffer, tmp_string_buffer );

    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, &bg_info, bg_effect_image );
        else           return doEffect( tmp_effect.effect, &bg_info, bg_effect_image );
    }
    else{
        for ( int i=0 ; i<3 ; i++ ){
            tachi_info[i].remove();
        }
        bg_info.remove();

        bg_effect_image = COLOR_EFFECT_IMAGE;

        if ( !strcmp( (const char*)tmp_string_buffer, "white" ) ){
            bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0xff;
        }
        else if ( !strcmp( (const char*)tmp_string_buffer, "black" ) ){
            bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0x00;
        }
        else if ( tmp_string_buffer[0] == '#' ){
            readColor( &bg_info.color, tmp_string_buffer + 1 );
        }
        else{
            setStr( &bg_info.image_name, tmp_string_buffer );
            parseTaggedString( &bg_info );
            setupAnimationInfo( &bg_info );
            bg_effect_image = BG_EFFECT_IMAGE;
            if ( bg_info.image_surface ){
                SDL_Rect src_rect, dst_rect;
                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.w = bg_info.image_surface->w;
                src_rect.h = bg_info.image_surface->h;
                dst_rect.x = (screen_width - bg_info.image_surface->w) / 2;
                dst_rect.y = (screen_height - bg_info.image_surface->h) / 2;

                SDL_BlitSurface( bg_info.image_surface, &src_rect, background_surface, &dst_rect );
            }
        }

        if ( bg_effect_image == COLOR_EFFECT_IMAGE ){
            SDL_FillRect( background_surface, NULL, SDL_MapRGB( effect_dst_surface->format, bg_info.color[0], bg_info.color[1], bg_info.color[2]) );
        }

        char *buf = new char[ 512 ];
        sprintf( buf, "bg \"%s\",",tmp_string_buffer );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::barclearCommand()
{
    for ( int i=0 ; i<MAX_PARAM_NUM ; i++ ) {
        if ( bar_info[i] ) {
            delete bar_info[i];
            bar_info[i] = NULL;
        }
    }
    return RET_CONTINUE;
}

int ONScripterLabel::barCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3;

    int no = readInt( &p_string_buffer );
    if ( bar_info[no] ) delete bar_info[no];
    bar_info[no] = new AnimationInfo();
    bar_info[no]->trans_mode = AnimationInfo::TRANS_COPY;
    bar_info[no]->num_of_cells = 1;
    bar_info[no]->current_cell = 0;
    bar_info[no]->alpha_offset = 0;

    int param           = readInt( &p_string_buffer );
    bar_info[no]->pos.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
                          
    bar_info[no]->pos.w = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.h = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    int max             = readInt( &p_string_buffer );
    if ( max == 0 ) errorAndExit( string_buffer + string_buffer_offset, "Invalid argument (max = 0)." );
    bar_info[no]->pos.w = bar_info[no]->pos.w * param / max;

    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '#' ) errorAndExit( string_buffer + string_buffer_offset, "Color is not specified." );
    readColor( &bar_info[no]->color, tmp_string_buffer + 1 );
    
    bar_info[no]->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, bar_info[no]->pos.w, bar_info[no]->pos.h, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( bar_info[no]->image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    SDL_FillRect( bar_info[no]->image_surface, NULL, SDL_MapRGB( bar_info[no]->image_surface->format, bar_info[no]->color[0], bar_info[no]->color[1], bar_info[no]->color[2] ) );

    return RET_CONTINUE;
}

int ONScripterLabel::autoclickCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9;
    autoclick_timer = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::amspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4;

    int no = readInt( &p_string_buffer );
    sprite_info[ no ].pos.x = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;
    sprite_info[ no ].pos.y = readInt( &p_string_buffer ) * screen_ratio1 / screen_ratio2;

    if ( end_with_comma_flag )
        sprite_info[ no ].trans = readInt( &p_string_buffer );

    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;
    
    return RET_CONTINUE;
}

int ONScripterLabel::allspresumeCommand()
{
    all_sprite_hide_flag = false;
    refreshSurface( accumulation_surface,
                                NULL,
                                ( erase_text_window_mode == 0 && text_on_flag)?REFRESH_SHADOW_MODE:REFRESH_NORMAL_MODE );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
    flush();

    return RET_CONTINUE;
}

int ONScripterLabel::allsphideCommand()
{
    all_sprite_hide_flag = true;
    refreshSurface( accumulation_surface,
                                NULL,
                                ( erase_text_window_mode == 0 && text_on_flag)?REFRESH_SHADOW_MODE:REFRESH_NORMAL_MODE );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
    flush();
    
    return RET_CONTINUE;
}
