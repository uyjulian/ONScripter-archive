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
#define ONSCRIPTER_VERSION 196

#define DEFAULT_CURSOR_WAIT    ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

#define CONTINUOUS_PLAY

int ONScripterLabel::waveCommand()
{
    char *p_string_buffer;

    if ( !strncmp( string_buffer + string_buffer_offset, "waveloop", 8 ) ){
        wave_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("waveloop") = 8
    }
    else{
        wave_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("wave") = 4
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("waittimer") = 9

    int count = readInt( &p_string_buffer ) + internal_timer - SDL_GetTicks();
    startTimer( count );
    
    return RET_WAIT_NEXT;
}

int ONScripterLabel::waitCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("wait") = 4

    startTimer( readInt( &p_string_buffer ) );

    return RET_WAIT_NEXT;
}

int ONScripterLabel::vspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("vsp") = 3

    int no = readInt( &p_string_buffer );
    int v  = readInt( &p_string_buffer );
    sprite_info[ no ].valid = (v==1)?true:false;
    
    return RET_CONTINUE;
}

int ONScripterLabel::voicevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("voicevol") = 8
    voice_volume = readInt( &p_string_buffer );

    if ( wave_sample[0] ) Mix_Volume( 0, se_volume * 128 / 100 );
    
    return RET_CONTINUE;
}

int ONScripterLabel::trapCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("trap") = 4
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
              
    printf("trapCommand %s\n", tmp_string_buffer );
    
    return RET_CONTINUE;
}

int ONScripterLabel::textspeedCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("textspeed") = 9

    sentence_font.wait_time = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::textonCommand()
{
    text_on_flag = true;
    if ( !(display_mode & TEXT_DISPLAY_MODE) ){
        shadowTextDisplay();
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
    if ( clickstr_state == CLICK_NEWPAGE ) newPage( false );

    return RET_CONTINUE;
}

int ONScripterLabel::systemcallCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("systemcall") = 10

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
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("spstr") = 5

    readStr( &p_string_buffer, tmp_string_buffer );
    decodeExbtnControl( NULL, tmp_string_buffer, false );
    
    return RET_CONTINUE;
}

int ONScripterLabel::spbtnCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("spbtn") = 5

    int sprite_no = readInt( &p_string_buffer );
    int no        = readInt( &p_string_buffer );

    if ( sprite_info[ sprite_no ].tag.num_of_cells == 0 ) return RET_CONTINUE;

    last_button_link->next = new ButtonLink();
    last_button_link = last_button_link->next;

    last_button_link->button_type = SPRITE_BUTTON;
    last_button_link->sprite_no   = sprite_no;
    last_button_link->no          = no;

    if ( sprite_info[ sprite_no ].image_surface || sprite_info[ sprite_no ].tag.trans_mode == TRANS_STRING ){
        last_button_link->image_rect = last_button_link->select_rect = sprite_info[ last_button_link->sprite_no ].pos;
        sprite_info[ sprite_no ].valid = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::sevolCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("sevol") = 5
    se_volume = readInt( &p_string_buffer );

    for ( int i=1 ; i<MIX_CHANNELS ; i++ )
        if ( wave_sample[i] ) Mix_Volume( i, se_volume * 128 / 100 );
        
    return RET_CONTINUE;
}

int ONScripterLabel::setwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("setwindow") = 9

    sentence_font.font_valid_flag = false;
    sentence_font.top_xy[0] = readInt( &p_string_buffer );
    sentence_font.top_xy[1] = readInt( &p_string_buffer );
    sentence_font.num_xy[0] = readInt( &p_string_buffer );
    sentence_font.num_xy[1] = readInt( &p_string_buffer );
    sentence_font.font_size = readInt( &p_string_buffer );
    readInt( &p_string_buffer ); // Ignore font size along Y axis
    sentence_font.pitch_xy[0] = readInt( &p_string_buffer ) + sentence_font.font_size;
    sentence_font.pitch_xy[1] = readInt( &p_string_buffer ) + sentence_font.font_size;
    sentence_font.wait_time = readInt( &p_string_buffer );
    sentence_font.display_bold = readInt( &p_string_buffer )?true:false;
    sentence_font.display_shadow = readInt( &p_string_buffer )?true:false;

    if ( sentence_font.ttf_font ) TTF_CloseFont( (TTF_Font*)sentence_font.ttf_font );
    sentence_font.ttf_font = (void*)TTF_OpenFont( font_file, sentence_font.font_size );
#if 0
    printf("ONScripterLabel::setwindowCommand (%d,%d) (%d,%d) font=%d (%d,%d) wait=%d bold=%d, shadow=%d\n",
           sentence_font.top_xy[0], sentence_font.top_xy[1], sentence_font.num_xy[0], sentence_font.num_xy[1],
           sentence_font.font_size, sentence_font.pitch_xy[0], sentence_font.pitch_xy[1], sentence_font.wait_time,
           sentence_font.display_bold, sentence_font.display_shadow );
#endif
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] == '#' ){
        sentence_font.display_transparency = true;
        if ( strlen( tmp_string_buffer ) != 7 ) errorAndExit( string_buffer + string_buffer_offset );
        readColor( &sentence_font.window_color, tmp_string_buffer + 1 );

        sentence_font_info.pos.x = readInt( &p_string_buffer );
        sentence_font_info.pos.y = readInt( &p_string_buffer );
        sentence_font_info.pos.w = readInt( &p_string_buffer ) - sentence_font_info.pos.x + 1;
        sentence_font_info.pos.h = readInt( &p_string_buffer ) - sentence_font_info.pos.y + 1;
#if 0
        printf("    trans %u %u %u rect %d %d %d %d\n",
               sentence_font.window_color[0], sentence_font.window_color[1], sentence_font.window_color[2],
               sentence_font_info.pos.x, sentence_font_info.pos.y, sentence_font_info.pos.w, sentence_font_info.pos.h );
#endif        
    }
    else{
        sentence_font.display_transparency = false;
        sentence_font_info.setImageName( tmp_string_buffer );
        parseTaggedString( sentence_font_info.image_name, &sentence_font_info.tag );
        setupAnimationInfo( &sentence_font_info );
        sentence_font_info.pos.x = readInt( &p_string_buffer );
        sentence_font_info.pos.y = readInt( &p_string_buffer );
#if 0        
        printf("    image %s rect %d %d \n",
               sentence_font.window_image,
               sentence_font.window_rect[0], sentence_font.window_rect[1] );
#endif        
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
        p_string_buffer = string_buffer + string_buffer_offset + 12; // strlen("abssetcursor") = 12
    }
    else{
        abs_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("setcursor") = 9
    }
    
    int no = readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    char *buf = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( buf, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    int x = readInt( &p_string_buffer );
    int y = readInt( &p_string_buffer );

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
    int select_mode;
    SelectLink *last_select_link;
    char *p_string_buffer, *p_buf;

    if ( !strncmp( string_buffer + string_buffer_offset, "selnum", 6 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("selnum") = 6
        select_mode = SELECT_NUM_MODE;
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "selgosub", 8 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("selgosub") = 8
        select_mode = SELECT_GOSUB_MODE;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("select") = 6
        select_mode = SELECT_GOTO_MODE;
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

        refreshAccumulationSurface( accumulation_surface );
        newPage( true );

        return ret;
    }
    else{
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

        if ( select_mode == SELECT_GOTO_MODE ){ /* Resume */
            p_script_buffer = current_link_label_info->current_script;
            readLine( &p_script_buffer );
        }
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();

        return RET_WAIT;
    }
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("savegame") = 8

    int no = readInt( &p_string_buffer );
    if ( no < 0 )
        errorAndExit( string_buffer + string_buffer_offset );
    else
        saveSaveFile( no );

    return RET_CONTINUE;
}

int ONScripterLabel::rndCommand()
{
    char *p_string_buffer, *p_buf;
    int  upper, lower;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "rnd2", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("rnd2") = 4
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
        lower = readInt( &p_string_buffer );
        upper = readInt( &p_string_buffer );
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("rnd") = 3
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("rmode") = 5

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

    deleteButtonLink();
    deleteSelectLink();

    wavestopCommand();
    stopBGM( false );
    
    SDL_FillRect( background_surface, NULL, SDL_MapRGBA( background_surface->format, 0, 0, 0, 0 ) );
    SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );

    return RET_JUMP;
}

int ONScripterLabel::quakeCommand()
{
    char *p_string_buffer;
    int quake_type;

    if ( !strncmp( string_buffer + string_buffer_offset, "quakey", 6 ) ){
        quake_type = 0;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("quakey") = 6
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "quakex", 6 ) ){
        quake_type = 1;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("quakex") = 6
    }
    else{
        quake_type = 2;
        p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("quake") = 5
    }

    quake_effect.num      = readInt( &p_string_buffer );
    quake_effect.duration = readInt( &p_string_buffer );

    if ( event_mode & EFFECT_EVENT_MODE ){
        if ( effect_counter == 0 )
            SDL_BlitSurface( text_surface, NULL, effect_dst_surface, NULL );
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("puttext") = 8
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    readStr( &p_string_buffer, tmp_string_buffer );

    drawString( tmp_string_buffer, sentence_font.color, &sentence_font, false, text_surface, NULL, true );
    flush();

    return RET_CONTINUE;
}

int ONScripterLabel::printCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("print") = 5

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
        mp3_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("playonce") = 8
    }
    else{
        mp3_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("play") = 4
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
    }

    return RET_CONTINUE;
}

int ONScripterLabel::mspCommand()
{
    int no;
    char *p_string_buffer;

    p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("msp") = 3;

    no = readInt( &p_string_buffer );
    sprite_info[ no ].pos.x += readInt( &p_string_buffer );
    sprite_info[ no ].pos.y += readInt( &p_string_buffer );
    sprite_info[ no ].trans += readInt( &p_string_buffer );
    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;

    return RET_CONTINUE;
}

int ONScripterLabel::mp3volCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("mp3vol") = 6
    mp3_volume = readInt( &p_string_buffer );

    if ( mp3_sample ) SMPEG_setvolume( mp3_sample, mp3_volume );

    return RET_CONTINUE;
}

int ONScripterLabel::mp3Command()
{
    char *p_string_buffer;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "mp3save", 7 ) ){
        mp3_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("mp3save") = 7
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "mp3loop", 7 ) ){
        mp3_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("mp3loop") = 7
    }
    else{
        mp3_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("mp3") = 3
    }

    stopBGM( false );
    
    readStr( &p_string_buffer, tmp_string_buffer );
    setStr( &mp3_file_name, tmp_string_buffer );

    playMP3( 0 );
        
    return RET_CONTINUE;
}

int ONScripterLabel::monocroCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("monocro") = 7

    readStr( &p_string_buffer, tmp_string_buffer );

    if ( !strcmp( tmp_string_buffer, "off" ) ){
        monocro_flag = false;
    }
    else if ( tmp_string_buffer[0] != '#' ){
        errorAndExit( string_buffer + string_buffer_offset );
    }
    else{
        monocro_flag = true;
        readColor( &monocro_color, tmp_string_buffer + 1 );
        for ( int i=0 ; i<256 ; i++ ){
            monocro_color_lut[i][0] = (monocro_color[0] * i) >> 8;
            monocro_color_lut[i][1] = (monocro_color[1] * i) >> 8;
            monocro_color_lut[i][2] = (monocro_color[2] * i) >> 8;
        }
    }
    return RET_CONTINUE;
}

int ONScripterLabel::lspCommand()
{
    bool v;
    int no;
    char *p_string_buffer;

    if ( !strncmp( string_buffer + string_buffer_offset, "lsph", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("lsph") = 4;
        v = false;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("lsp") = 3
        v = true;
    }

    no = readInt( &p_string_buffer );
    sprite_info[ no ].valid = v;

    readStr( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].setImageName( tmp_string_buffer );

    sprite_info[ no ].pos.x = readInt( &p_string_buffer );
    sprite_info[ no ].pos.y = readInt( &p_string_buffer );

    parseTaggedString( sprite_info[ no ].image_name, &sprite_info[ no ].tag );
    sprite_info[ no ].tag.pos.x = sprite_info[ no ].pos.x;
    sprite_info[ no ].tag.pos.y = sprite_info[ no ].pos.y;
    setupAnimationInfo( &sprite_info[ no ] );

    if ( *p_string_buffer != '\0' ){
        sprite_info[ no ].trans = readInt( &p_string_buffer );
    }
    else{
        sprite_info[ no ].trans = 255;
    }

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
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("locate") = 6

    sentence_font.xy[0] = readInt( &p_string_buffer );
    sentence_font.xy[1] = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::loadgameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("loadgame") = 8
    
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
        printf("loadGame %d %d\n",event_mode,skip_flag);
        if ( event_mode & WAIT_INPUT_MODE ) return RET_WAIT;
        return RET_JUMP;
    }
}

int ONScripterLabel::ldCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2; // strlen("ld") = 2
    
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
        TaggedInfo tagged_info;

        if ( loc == 'l' )      no = 0;
        else if ( loc == 'c' ) no = 1;
        else if ( loc == 'r' ) no = 2;
        
        tachi_info[ no ].setImageName( tmp_string_buffer );
        parseTaggedString( tachi_info[ no ].image_name, &tachi_info[ no ].tag );
        setupAnimationInfo( &tachi_info[ no ] );
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("ispage") = 6
    char *p_buf = p_string_buffer;

    if ( clickstr_state == CLICK_NEWPAGE ){
        setInt( p_buf, 1 );
    }
    else{
        setInt( p_buf, 0 );
    }
    
    return RET_CONTINUE;
    
}

int ONScripterLabel::getversionCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("getversion") = 10
    
    setInt( p_string_buffer, ONSCRIPTER_VERSION );

    return RET_CONTINUE;
}

int ONScripterLabel::gettimerCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("gettimer") = 8 
 
    setInt( p_string_buffer, SDL_GetTicks() - internal_timer ); 
 
    return RET_CONTINUE; 
}

int ONScripterLabel::getregCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("getreg") = 6
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

int ONScripterLabel::getcursorposCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 12; // strlen("getcursorpos") = 12
    char *p_buf = p_string_buffer;

    readInt( &p_string_buffer );
    setInt( p_buf, sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] );

    p_buf = p_string_buffer;
    setInt( p_buf, sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] );

    printf("getcursorpos %d %d\n",
           sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0],
           sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] );
    
    return RET_CONTINUE;
}

int ONScripterLabel::gameCommand()
{
    current_link_label_info->label_info = lookupLabel( "start" );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;
    current_mode = NORMAL_MODE;

    /* ---------------------------------------- */
    /* Load default cursor */
    loadCursor( CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0 );
    loadCursor( CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0 );
    
    /* ---------------------------------------- */
    /* Lookback related variables */
    for ( int i=0 ; i<4 ; i++ ){
        setStr( &lookback_info[i].image_name, lookback_image_name[i] );
        parseTaggedString( lookback_info[i].image_name, &lookback_info[i].tag );
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
        p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("exbtn_d") = 7
        button = &exbtn_d_button_link;
        if ( button->exbtn_ctl ) delete[] button->exbtn_ctl;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("exbtn") = 5
        sprite_no = readInt( &p_string_buffer );
        no = readInt( &p_string_buffer );

        if ( sprite_info[ sprite_no ].tag.num_of_cells == 0 ) return RET_CONTINUE;
        
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
         ( sprite_info[ sprite_no ].image_surface || sprite_info[ sprite_no ].tag.trans_mode == TRANS_STRING ) ){
        button->image_rect = button->select_rect = sprite_info[ button->sprite_no ].pos;
        sprite_info[ sprite_no ].valid = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::erasetextwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 15; // strlen("erasetextwindow") = 15

    erase_text_window_flag = (readInt( &p_string_buffer )==1)?true:false;
    printf("erase text window %s\n",erase_text_window_flag?"true":"false");
    
    return RET_CONTINUE;
}

int ONScripterLabel::endCommand()
{
    saveGlovalData();
    saveFileLog();
    saveLabelLog();
    SDL_Quit();
    exit(0);
}

int ONScripterLabel::dwavestopCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("dwave") = 9
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
        p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("dwaveloop") = 9
    }
    else{
        wave_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("dwave") = 5
    }

    wavestopCommand();

    int ch = readInt( &p_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    playWave( tmp_string_buffer, wave_play_once_flag, ch );
        
    return RET_CONTINUE;
}

int ONScripterLabel::delayCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("delay") = 5

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
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("csp") = 3
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

int ONScripterLabel::clickCommand()
{
    if ( event_mode & WAIT_INPUT_MODE ){
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 2; // strlen("cl") = 2
    
    readStr( &p_string_buffer, tmp_string_buffer );
    char loc = tmp_string_buffer[0];
    
    if ( loc == 'l' || loc == 'a' ){
        tachi_info[0].deleteImageName();
        tachi_info[0].deleteImageSurface();
    }
    if ( loc == 'c' || loc == 'a' ){
        tachi_info[1].deleteImageName();
        tachi_info[1].deleteImageSurface();
    }
    if ( loc == 'r' || loc == 'a' ){
        tachi_info[2].deleteImageName();
        tachi_info[2].deleteImageSurface();
    }

    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        else           return doEffect( tmp_effect.effect, NULL, TACHI_EFFECT_IMAGE );
    }
    else{
        char *buf = new char[512];
        sprintf( buf, "cl %c,", loc );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::cellCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("cell") = 4
    
    int sprite_no = readInt( &p_string_buffer );
    int no        = readInt( &p_string_buffer );

    if ( sprite_info[ sprite_no ].tag.num_of_cells > 0 )
        sprite_info[ sprite_no ].tag.current_cell = no;
        
    return RET_CONTINUE;
}

int ONScripterLabel::captionCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("caption") = 7

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

    printf("caption \"%s\"\n", wm_title_string );
    SDL_WM_SetCaption( wm_title_string, wm_icon_string );

    return RET_CONTINUE;
}


int ONScripterLabel::btndefCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("btndef") = 6
    readStr( &p_string_buffer, tmp_string_buffer );

    if ( strcmp( tmp_string_buffer, "clear" ) ){
        if ( btndef_surface ) SDL_FreeSurface( btndef_surface );
        btndef_surface = NULL;

        unsigned long length = cBR->getFileLength( tmp_string_buffer );
        if ( length ){
            unsigned char *buffer = new unsigned char[length];
            cBR->getFile( tmp_string_buffer, buffer );
            btndef_surface = IMG_Load_RW(SDL_RWFromMem( buffer, length ),1);
            delete[] buffer;
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
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("btn") = 3
    last_button_link->button_type  = NORMAL_BUTTON;
    last_button_link->no           = readInt( &p_string_buffer );
    last_button_link->image_rect.x = readInt( &p_string_buffer );
    last_button_link->image_rect.y = readInt( &p_string_buffer );
    last_button_link->image_rect.w = readInt( &p_string_buffer );
    last_button_link->image_rect.h = readInt( &p_string_buffer );
    last_button_link->select_rect = last_button_link->image_rect;

    src_rect.x = readInt( &p_string_buffer );
    src_rect.y = readInt( &p_string_buffer );
    src_rect.w = last_button_link->image_rect.w;
    src_rect.h = last_button_link->image_rect.h;
#if 0
    printf("ONScripterLabel::btnCommand %d,%d,%d,%d,%d,%d,%d\n",
           last_button_link->no,
           last_button_link->image_rect.x,
           last_button_link->image_rect.y,
           last_button_link->image_rect.w,
           last_button_link->image_rect.h,
           src_rect.x,
           src_rect.y );
#endif

    last_button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, last_button_link->image_rect.w, last_button_link->image_rect.h, 32, rmask, gmask, bmask, amask );

    SDL_BlitSurface( btndef_surface, &src_rect, last_button_link->image_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::btnwaitCommand()
{
    char *p_string_buffer;
    bool del_flag, textbtn_flag = false;

    if ( !strncmp( string_buffer + string_buffer_offset, "btnwait2", 8 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("btnwait2") = 8
        del_flag = false;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "btnwait", 7 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("btnwait") = 7
        del_flag = true;
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "textbtnwait", 11 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 11; // strlen("textbtnwait") = 11
        del_flag = false;
        textbtn_flag = true;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE ){
        setInt( p_string_buffer, current_button_state.button );

        if ( del_flag && current_button_state.button > 0 ) deleteButtonLink();
        if ( exbtn_d_button_link.exbtn_ctl ) delete[] exbtn_d_button_link.exbtn_ctl;
        exbtn_d_button_link.exbtn_ctl = NULL;

        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        skip_flag = false;
        event_mode = WAIT_BUTTON_MODE;
        if ( textbtn_flag ) event_mode |= WAIT_TEXTBTN_MODE;

        ButtonLink *p_button_link = root_button_link.next;
        while( p_button_link ){
            if ( current_button_link.button_type == SPRITE_BUTTON || current_button_link.button_type == EX_SPRITE_BUTTON )
                sprite_info[ current_button_link.sprite_no ].tag.current_cell = 0;
            p_button_link = p_button_link->next;
        }
        refreshAccumulationSurface( accumulation_surface );
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        if ( !erase_text_window_flag && text_on_flag ){
            shadowTextDisplay( NULL, text_surface );
            restoreTextBuffer();
            display_mode = TEXT_DISPLAY_MODE;
        }
        else{
            display_mode = NORMAL_DISPLAY_MODE;
        }
        
        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

        flush();

        refreshMouseOverButton();

        return RET_WAIT;
    }
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("blt") = 3
    SDL_Rect src_rect, dst_rect;

    dst_rect.x = readInt( &p_string_buffer );
    dst_rect.y = readInt( &p_string_buffer );
    dst_rect.w = readInt( &p_string_buffer );
    dst_rect.h = readInt( &p_string_buffer );
    src_rect.x = readInt( &p_string_buffer );
    src_rect.y = readInt( &p_string_buffer );
    src_rect.w = readInt( &p_string_buffer );
    src_rect.h = readInt( &p_string_buffer );
    
    SDL_BlitSurface( btndef_surface, &src_rect, screen_surface, &dst_rect );
    SDL_UpdateRect( screen_surface, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h );
    
    return RET_CONTINUE;
}

int ONScripterLabel::bgCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2; // strlen("bg") = 2
    
    readStr( &p_string_buffer, tmp_string_buffer );

    if ( event_mode & EFFECT_EVENT_MODE ){
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, &bg_info, bg_effect_image );
        else           return doEffect( tmp_effect.effect, &bg_info, bg_effect_image );
    }
    else{
        for ( int i=0 ; i<3 ; i++ ){
            tachi_info[i].deleteImageName();
            tachi_info[i].deleteImageSurface();
        }

        bg_effect_image = COLOR_EFFECT_IMAGE;

        if ( !strcmp( (const char*)tmp_string_buffer, "white" ) ){
            bg_info.tag.color[0] = bg_info.tag.color[1] = bg_info.tag.color[2] = 0xff;
        }
        else if ( !strcmp( (const char*)tmp_string_buffer, "black" ) ){
            bg_info.tag.color[0] = bg_info.tag.color[1] = bg_info.tag.color[2] = 0x00;
        }
        else if ( tmp_string_buffer[0] == '#' ){
            readColor( &bg_info.tag.color, tmp_string_buffer + 1 );
        }
        else{
            parseTaggedString( tmp_string_buffer, &bg_info.tag );
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
            SDL_FillRect( background_surface, NULL, SDL_MapRGB( effect_dst_surface->format, bg_info.tag.color[0], bg_info.tag.color[1], bg_info.tag.color[2]) );
        }

        char *buf = new char[ 512 ];
        sprintf( buf, "bg \"%s\",",tmp_string_buffer );
        makeEffectStr( &p_string_buffer, buf );
        return setEffect( tmp_effect.effect, buf );
    }
}

int ONScripterLabel::autoclickCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("autoclick") = 9
    autoclick_timer = readInt( &p_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::amspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("amsp") = 4;

    int no = readInt( &p_string_buffer );
    sprite_info[ no ].pos.x = readInt( &p_string_buffer );
    sprite_info[ no ].pos.y = readInt( &p_string_buffer );
    sprite_info[ no ].trans = readInt( &p_string_buffer );

    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;
    //printf("amsp [%d] %d %d %d\n",no, sprite_info[ no ].x, sprite_info[ no ].y, sprite_info[ no ].trans );
    
    return RET_CONTINUE;
}
