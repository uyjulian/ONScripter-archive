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
    if ( count > 0 ){
        startTimer( count );
        return RET_WAIT_NEXT;
    }
    else
        return RET_CONTINUE;
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
    int v = readInt( &p_string_buffer );
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
              
    printf("trapCommand %s\n", tmp_string_buffer );
    
    return RET_CONTINUE;
}

int ONScripterLabel::textclearCommand()
{
     clearCurrentTextBuffer();

    /* ---------------------------------------- */
    /* Clear the screen */
    shadowTextDisplay();
    flush();

    return RET_CONTINUE;
}

int ONScripterLabel::systemcallCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("systemcall") = 10

    readStr( &p_string_buffer, tmp_string_buffer );
    system_menu_mode = getSystemCallNo( tmp_string_buffer );
    event_mode = WAIT_SLEEP_MODE;
    
    startTimer( MINIMUM_TIMER_RESOLUTION );
    return RET_WAIT_NEXT;
}

int ONScripterLabel::stopCommand()
{
    wavestopCommand();
    playstopCommand();
    if ( mp3_file_name ){
        delete[] mp3_file_name;
        mp3_file_name = NULL;
    }
    
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
    int sprite_no, no;
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("spbtn") = 5
    printf("spbtnCommand %s\n",string_buffer + string_buffer_offset);
    sprite_no = readInt( &p_string_buffer );
    no = readInt( &p_string_buffer );

    //if ( !sprite_info[ sprite_no ].valid ) return RET_CONTINUE;
    
    last_button_link->next = new ButtonLink();
    last_button_link = last_button_link->next;

    last_button_link->button_type = SPRITE_BUTTON;
    last_button_link->sprite_no = sprite_no;
    last_button_link->no = no;
    
    if ( sprite_info[ sprite_no ].image_surface )
        last_button_link->image_rect = last_button_link->select_rect = sprite_info[ last_button_link->sprite_no ].pos;

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
    //if ( sentence_font.wait_time == 0 ) sentence_font.wait_time = 10;
    sentence_font.display_bold = readInt( &p_string_buffer )?true:false;
    sentence_font.display_shadow = readInt( &p_string_buffer )?true:false;

    if ( sentence_font.ttf_font ) TTF_CloseFont( (TTF_Font*)sentence_font.ttf_font );
    sentence_font.ttf_font = (void*)TTF_OpenFont( FONT_NAME, sentence_font.font_size );
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
        sentence_font.window_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
        sentence_font.window_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
        sentence_font.window_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );

        sentence_font.window_rect[0] = readInt( &p_string_buffer );
        sentence_font.window_rect[1] = readInt( &p_string_buffer );
        sentence_font.window_rect[2] = readInt( &p_string_buffer );
        sentence_font.window_rect[3] = readInt( &p_string_buffer );
#if 0
        printf("    trans %u %u %u rect %d %d %d %d\n",
               sentence_font.window_color[0], sentence_font.window_color[1], sentence_font.window_color[2],
               sentence_font.window_rect[0], sentence_font.window_rect[1], sentence_font.window_rect[2], sentence_font.window_rect[3] );
#endif        
    }
    else{
        sentence_font.display_transparency = false;
        if ( sentence_font.window_image ) delete[] sentence_font.window_image;
        sentence_font.window_image = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( sentence_font.window_image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        parseTaggedString( sentence_font.window_image, &sentence_font_tag );
        sentence_font.window_rect[0] = readInt( &p_string_buffer );
        sentence_font.window_rect[1] = readInt( &p_string_buffer );
#if 0        
        printf("    image %s rect %d %d \n",
               sentence_font.window_image,
               sentence_font.window_rect[0], sentence_font.window_rect[1] );
#endif        
    }

    lookbackflushCommand();
    clearCurrentTextBuffer();
    SDL_BlitSurface( accumulation_surface, NULL, select_surface, NULL );
    display_mode = NORMAL_DISPLAY_MODE;
    
    return RET_CONTINUE;
}

int ONScripterLabel::setcursorCommand()
{
    int x, y, no;
    char *buf;
    bool abs_flag;
    char *p_string_buffer;
    if ( !strncmp( string_buffer + string_buffer_offset, "abssetcursor", 12 ) ) {
        abs_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 12; // strlen("abssetcursor") = 12
    }
    else{
        abs_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("setcursor") = 9
    }
    
    no = readInt( &p_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    buf = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( buf, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    x = readInt( &p_string_buffer );
    y = readInt( &p_string_buffer );

    loadCursor( no, buf, x, y, abs_flag );
    delete[] buf;
    
    return RET_CONTINUE;
}

int ONScripterLabel::selectCommand()
{
    int ret = enterTextDisplayMode();
    int xy[2];
    if ( ret != RET_NOMATCH ) return ret;

    //printf("selectCommand %d\n", event_mode);
    char *p_script_buffer = current_link_label_info->current_script;
    readLine( &p_script_buffer );
    int select_mode;
    char *p_string_buffer, *p_buf;

    if ( !strncmp( string_buffer + string_buffer_offset, "selnum", 6 ) ){
        select_mode = 2;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("selnum") = 6
        while( *p_string_buffer == ' ' || *p_string_buffer == '\t' ) p_string_buffer++;
        p_buf = p_string_buffer;
        readInt( &p_string_buffer );
    }
    else if ( !strncmp( string_buffer + string_buffer_offset, "selgosub", 8 ) ){
        select_mode = 1;
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("selgosub") = 8
    }
    else{
        select_mode = 0;
        p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("select") = 6
    }
    SelectLink *tmp_select_link = NULL;
    bool comma_flag, first_token_flag = true;
    int count = 0;

    //printf("p_string_buffer [%s], string_buffer [%s], string_buffer_offset %d\n",
    //p_string_buffer, string_buffer, string_buffer_offset );
    if ( event_mode & (WAIT_MOUSE_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return RET_WAIT;
        
        event_mode = IDLE_EVENT_MODE;
        int counter = 1;

        deleteButtonLink();

        last_select_link = root_select_link.next;
        while ( last_select_link ){
            if ( current_button_state.button == counter++ ){
                //printf("button %d label %s is selected \n",current_button_state.button, last_select_link->label);
                break;
            }
            last_select_link = last_select_link->next;
        }

        if ( select_mode  == 0 ){ // go
            current_link_label_info->label_info = lookupLabel( last_select_link->label );
            current_link_label_info->current_line = 0;
            current_link_label_info->offset = 0;
            ret = RET_JUMP;
        }
        else if ( select_mode == 1 ){
            current_link_label_info->current_line = select_label_info.current_line;
            current_link_label_info->offset = select_label_info.offset;
            gosubReal( last_select_link->label );
            ret = RET_JUMP;
        }
        else{
            setInt( p_buf, current_button_state.button - 1 );
            ret = RET_CONTINUE;
        }
        deleteSelectLink();

        refreshAccumulationSurface( accumulation_surface );
        //SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
        enterNewPage();

        return ret;
    }
    else{
        shortcut_mouse_line = -1;
        flush();
        skip_flag = false;
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];
        
        select_label_info.current_line = current_link_label_info->current_line;
        while(1){
            comma_flag = readStr( &p_string_buffer, tmp_string_buffer );
            //printf("read tmp_string_buffer [%s] %d\n", tmp_string_buffer, comma_flag );
            //printf("read p_string [%s]\n", p_string_buffer );
            if ( tmp_string_buffer[0] != '\0' ){
                //printf("tmp_string_buffer %s\n", tmp_string_buffer );
                if ( first_token_flag ){ // First token is at the second line
                    first_token_flag = false;
                }
                count++;
                if ( select_mode == 2 || count % 2 ){
                    tmp_select_link = new SelectLink();
                    tmp_select_link->next = NULL;
                    tmp_select_link->text = new char[ strlen(tmp_string_buffer) + 1 ];
                    memcpy( tmp_select_link->text, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
                }
                if ( select_mode == 2 || !(count % 2) ){
                    tmp_select_link->label = new char[ strlen(tmp_string_buffer+1) + 1 ];
                    memcpy( tmp_select_link->label, tmp_string_buffer+1, strlen(tmp_string_buffer+1) + 1 );
                    last_select_link->next = tmp_select_link;
                    last_select_link = last_select_link->next;
                }
            }
            
            if ( p_string_buffer[0] == '\0' ){ // end of line
                //printf(" end of line %d\n", first_token_flag);
                if ( first_token_flag ){ // First token is at the second line
                    comma_flag = true;
                    first_token_flag = false;
                }
                do{
                    readLine( &p_script_buffer );
                }
                while ( string_buffer[ string_buffer_offset ] == ';' || string_buffer[ string_buffer_offset ] == '\0' );
                select_label_info.current_line++;
                //printf("after readline %s\n",string_buffer);
                
                while( string_buffer[ string_buffer_offset ] == ' ' ||
                       string_buffer[ string_buffer_offset ] == '\t' ) string_buffer_offset++;
                p_string_buffer = string_buffer + string_buffer_offset;
                assert ( !comma_flag || string_buffer[ string_buffer_offset ] != ',' );
                if ( !comma_flag && string_buffer[ string_buffer_offset ] != ',' ) break;

                if ( string_buffer[ string_buffer_offset ] == ',' ) string_buffer_offset++;
                continue;
            }
            
            if ( first_token_flag ) first_token_flag = false;

        }
        select_label_info.offset = string_buffer_offset;
        //printf("select end\n");

        tmp_select_link = root_select_link.next;
        int counter = 1;
        while( tmp_select_link ){
            last_button_link->next = getSelectableSentence( tmp_select_link->text, &sentence_font );
            last_button_link = last_button_link->next;
            last_button_link->no = counter++;

            //printf(" link text %s label %s\n", tmp_select_link->text, tmp_select_link->label );
            tmp_select_link = tmp_select_link->next;
        }
        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

        if ( select_mode == 0 ){
            /* Resume */
            p_script_buffer = current_link_label_info->current_script;
            readLine( &p_script_buffer );
        }
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];

        event_mode = WAIT_MOUSE_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();

        return RET_WAIT;
    }
}

int ONScripterLabel::savegameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("savegame") = 8

    int no = readInt( &p_string_buffer );
    saveSaveFile( no );

    return RET_CONTINUE;
}

int ONScripterLabel::rndCommand()
{
    char *p_string_buffer, *p_buf;
    int upper, lower;
    
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
    int i=0;

    for ( i=0 ; i<199 ; i++ ){
        num_variables[i] = 0;
        if ( str_variables[i] ) delete[] str_variables[i];
        str_variables[i] = NULL;
    }

    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;
    text_char_flag = false;
    skip_flag = false;
    monocro_flag = false;

    deleteLabelLink();
    current_link_label_info->label_info = lookupLabel( "start" );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;

    deleteButtonLink();
    deleteSelectLink();

    wavestopCommand();
    playstopCommand();
    if ( mp3_file_name ){
        delete[] mp3_file_name;
        mp3_file_name = NULL;
    }
    
    SDL_FillRect( background_surface, NULL, SDL_MapRGBA( background_surface->format, 0, 0, 0, 0 ) );
    SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
    SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );

    return RET_JUMP;
}

int ONScripterLabel::puttextCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    char out_text[3];
    int c=0;
    char ch, *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("puttext") = 8

    readStr( &p_string_buffer, tmp_string_buffer );
    printf("puttext %s\n",tmp_string_buffer );
    
    while( tmp_string_buffer[ c ] ){
        ch = tmp_string_buffer[ c ];
        if ( ch & 0x80 ){
            out_text[0] = tmp_string_buffer[ c++ ];
            out_text[1] = tmp_string_buffer[ c++ ];
            out_text[2] = '\0';
        }
        else{
            out_text[0] = tmp_string_buffer[ c++ ];
            out_text[1] = '\0';
        }
        drawChar( out_text, &sentence_font, false );
    }
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
        if ( print_effect.effect == 0 ) return setEffect( RET_CONTINUE, buf );
        else                            return setEffect( RET_WAIT_NEXT, buf );
    }
}

int ONScripterLabel::playstopCommand()
{
    if ( mp3_sample ){
        SMPEG_stop( mp3_sample );
        SMPEG_delete( mp3_sample );
        //Mix_FadeOutMusic( 1000 );
        Mix_HookMusic( NULL, NULL );
#if 0        
        if ( mp3_sample->flags & SOUND_SAMPLEFLAG_ERROR ){
            fprintf(stderr, "Error in decoding sound file!\n"
                    "  reason: [%s].\n", Sound_GetError());
        }

        Sound_FreeSample( mp3_sample );
#endif        
        mp3_sample = NULL;
        if ( mp3_buffer ){
            delete[] mp3_buffer;
            mp3_buffer = NULL;
        }
    }

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
        playstopCommand();
        if ( mp3_file_name ){
            delete[] mp3_file_name;
            mp3_file_name = NULL;
        }
        current_cd_track = atoi( tmp_string_buffer + 1 );
        playMP3( current_cd_track );
        
        return RET_CONTINUE;
    }
    else{ // play MIDI
        return RET_CONTINUE;
    }
    
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

    readStr( &p_string_buffer, tmp_string_buffer );

    playstopCommand();

    if ( mp3_file_name ) delete[] mp3_file_name;
    mp3_file_name = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( mp3_file_name, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    playMP3( 0 );
        
    return RET_CONTINUE;
}

int ONScripterLabel::monocroCommand()
{
    int i;
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
        monocro_color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
        monocro_color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
        monocro_color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
        for ( i=0 ; i<256 ; i++ ){
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
    if ( sprite_info[ no ].image_name ) delete[] sprite_info[ no ].image_name;
    sprite_info[ no ].image_name = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( sprite_info[ no ].image_name, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );

    parseTaggedString( sprite_info[ no ].image_name, &sprite_info[ no ].tag );
    setupAnimationInfo( &sprite_info[ no ] );

    sprite_info[ no ].pos.x = readInt( &p_string_buffer );
    sprite_info[ no ].pos.y = readInt( &p_string_buffer );

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
    printf("loadGmae %d\n", no);
    if ( loadSaveFile( no ) ) return RET_CONTINUE;
    else {
        skip_flag = false;
        deleteButtonLink();
        deleteSelectLink();
        key_pressed_flag = false;
        printf("loadGmae %d %d\n",event_mode,skip_flag);
        if ( event_mode & (WAIT_MOUSE_MODE | WAIT_KEY_MODE) ) return RET_WAIT;
        startTimer( MINIMUM_TIMER_RESOLUTION );
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
        
        if ( tachi_info[ no ].image_name ) delete[] tachi_info[ no ].image_name;
        tachi_info[ no ].image_name = new char[ strlen(tmp_string_buffer) + 1 ];
        memcpy( tachi_info[ no ].image_name, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
        parseTaggedString( tachi_info[ no ].image_name, &tagged_info );
    
        if ( tachi_info[ no ].image_surface ) SDL_FreeSurface( tachi_info[ no ].image_surface );
        tachi_info[ no ].image_surface = loadPixmap( &tagged_info );

        char *buf = new char[512];
        sprintf( buf, "ld %c, \"%s\",", loc, tmp_string_buffer );
        makeEffectStr( &p_string_buffer, buf );
        if ( tmp_effect.effect == 0 ) return setEffect( RET_CONTINUE, buf );
        else                          return setEffect( RET_WAIT_NEXT, buf );
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

int ONScripterLabel::getversionCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 10; // strlen("getversion") = 10
    
    setInt( p_string_buffer, ONSCRITER_VERSION );

    return RET_CONTINUE;
}

int ONScripterLabel::gettimerCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("gettimer") = 8 
 
    setInt( p_string_buffer, SDL_GetTicks() - internal_timer ); 
 
    return RET_CONTINUE; 
}

int ONScripterLabel::gameCommand()
{
    current_link_label_info->label_info = lookupLabel( "start" );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;
    current_mode = NORMAL_MODE;

    /* ---------------------------------------- */
    /* Lookback related variables */
    int i;
    for ( i=0 ; i<4 ; i++ ){
        parseTaggedString( lookback_image_name[i], &lookback_image_tag[i] );
        lookback_image_surface[i] = loadPixmap( &lookback_image_tag[i] );
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
        button = new ButtonLink();
        last_button_link->next = button;
        last_button_link = last_button_link->next;
    }
    printf("exbtnCommand %s\n",string_buffer + string_buffer_offset);
    readStr( &p_string_buffer, tmp_string_buffer );

    //if ( !sprite_info[ sprite_no ].valid ) return RET_CONTINUE;

    button->button_type = EX_SPRITE_BUTTON;
    button->sprite_no = sprite_no;
    button->no = no;
    button->exbtn_ctl = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( button->exbtn_ctl, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
    
    if ( sprite_no >= 0 && sprite_info[ sprite_no ].image_surface )
        button->image_rect = button->select_rect = sprite_info[ button->sprite_no ].pos;

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
    int ch;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "dwaveloop", 9 ) ){
        wave_play_once_flag = true;
        p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("dwaveloop") = 9
    }
    else{
        wave_play_once_flag = false;
        p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("dwave") = 5
    }

    wavestopCommand();

    ch = readInt( &p_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    playWave( tmp_string_buffer, wave_play_once_flag, ch );
        
    return RET_CONTINUE;
}

int ONScripterLabel::delayCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("delay") = 5

    int t = readInt( &p_string_buffer );

    if ( event_mode & (WAIT_SLEEP_MODE | WAIT_MOUSE_MODE | WAIT_KEY_MODE) ){
        event_mode = IDLE_EVENT_MODE;
    }
    else{
        event_mode = WAIT_SLEEP_MODE | WAIT_MOUSE_MODE | WAIT_KEY_MODE;
        key_pressed_flag = false;
        startTimer( t );
        return RET_WAIT;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::cspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("csp") = 3
    int no = readInt( &p_string_buffer );

    if ( no == -1 )
        for ( int i=0 ; i<MAX_SPRITE_NUM ; i++ ) sprite_info[i].valid = false;
    else{
        sprite_info[no].valid = false;
        if ( sprite_info[no].image_name ){
            delete[] sprite_info[no].image_name;
            sprite_info[no].image_name = NULL;
        }
        if ( sprite_info[no].image_surface ){
            SDL_FreeSurface( sprite_info[no].image_surface );
            sprite_info[no].image_surface = NULL;
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::clickCommand()
{
    if ( event_mode & (WAIT_MOUSE_MODE | WAIT_BUTTON_MODE) ){
        return RET_CONTINUE;
    }
    else{
        event_mode = WAIT_MOUSE_MODE | WAIT_KEY_MODE;
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
        if ( tachi_info[0].image_name ) delete[] tachi_info[0].image_name;
        tachi_info[0].image_name = NULL;
        if ( tachi_info[0].image_surface ) SDL_FreeSurface( tachi_info[0].image_surface );
        tachi_info[0].image_surface = NULL;
    }
    if ( loc == 'c' || loc == 'a' ){
        if ( tachi_info[1].image_name ) delete[] tachi_info[1].image_name;
        tachi_info[1].image_name = NULL;
        if ( tachi_info[1].image_surface ) SDL_FreeSurface( tachi_info[1].image_surface );
        tachi_info[1].image_surface = NULL;
    }
    if ( loc == 'r' || loc == 'a' ){
        if ( tachi_info[2].image_name ) delete[] tachi_info[2].image_name;
        tachi_info[2].image_name = NULL;
        if ( tachi_info[2].image_surface ) SDL_FreeSurface( tachi_info[2].image_surface );
        tachi_info[2].image_surface = NULL;
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
        if ( tmp_effect.effect == 0 ) return setEffect( RET_CONTINUE, buf );
        else                          return setEffect( RET_WAIT_NEXT, buf );
    }
}

int ONScripterLabel::cellCommand()
{
    int sprite_no, no;
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("cell") = 4
    printf("cellCommand %s\n",string_buffer + string_buffer_offset);
    sprite_no = readInt( &p_string_buffer );
    no = readInt( &p_string_buffer );

    if ( sprite_info[ sprite_no ].tag.num_of_cells > 0 )
        sprite_info[ sprite_no ].tag.current_cell = no;
        
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
    int x3, y3;
    
    last_button_link->next = new ButtonLink();
    last_button_link = last_button_link->next;
    
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("btn") = 3
    last_button_link->button_type = NORMAL_BUTTON;
    last_button_link->no = readInt( &p_string_buffer );
    last_button_link->image_rect.x = readInt( &p_string_buffer );
    last_button_link->image_rect.y = readInt( &p_string_buffer );
    last_button_link->image_rect.w = readInt( &p_string_buffer );
    last_button_link->image_rect.h = readInt( &p_string_buffer );
    last_button_link->select_rect = last_button_link->image_rect;
    x3 = readInt( &p_string_buffer );
    y3 = readInt( &p_string_buffer );
#if 0
    printf("ONScripterLabel::btnCommand %d,%d,%d,%d,%d,%d,%d\n",
           last_button_link->no,
           last_button_link->image_rect.x,
           last_button_link->image_rect.y,
           last_button_link->image_rect.w,
           last_button_link->image_rect.h,
           x3,
           y3 );
#endif

    last_button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, last_button_link->image_rect.w, last_button_link->image_rect.h, 32, rmask, gmask, bmask, amask );

    SDL_Rect src_rect;
    src_rect.x = x3;
    src_rect.y = y3;
    src_rect.w = last_button_link->image_rect.w;
    src_rect.h = last_button_link->image_rect.h;
    SDL_BlitSurface( btndef_surface, &src_rect, last_button_link->image_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::btnwaitCommand()
{
    char *p_string_buffer;
    bool del_flag;

    if ( !strncmp( string_buffer + string_buffer_offset, "btnwait2", 8 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("btnwait2") = 8
        del_flag = false;
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("btnwait") = 7
        del_flag = true;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE ){
        setInt( p_string_buffer, current_button_state.button );

        if ( del_flag && current_button_state.button > 0 ) deleteButtonLink();
        if ( exbtn_d_button_link.exbtn_ctl ) delete[] exbtn_d_button_link.exbtn_ctl;
        exbtn_d_button_link.exbtn_ctl = NULL;

        /* ---------------------------------------- */
        /* fill the button image */
        if ( current_over_button != 0 ){
            // Almost the same code as in the mouseOverCheck(), possible cause of bug !!
            if ( current_over_button_link.button_type == NORMAL_BUTTON ){
                SDL_BlitSurface( select_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );
            }
            else if ( current_over_button_link.button_type == SPRITE_BUTTON ){
                refreshAccumulationSurface( accumulation_surface );
                SDL_BlitSurface( accumulation_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );
            }
            flush( &current_over_button_link.image_rect );
            current_over_button = 0;
        }
            
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        skip_flag = false;
        event_mode = WAIT_BUTTON_MODE;
        refreshAccumulationSurface( accumulation_surface );
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
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
    SDL_Rect src_rect, dst_rect;
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("blt") = 3

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
        bg_effect_image = COLOR_EFFECT_IMAGE;
        
        if ( !strcmp( (const char*)tmp_string_buffer, "white" ) ){
            bg_image_tag.color[0] = bg_image_tag.color[1] = bg_image_tag.color[2] = 0xff;
        }
        else if ( !strcmp( (const char*)tmp_string_buffer, "black" ) ){
            bg_image_tag.color[0] = bg_image_tag.color[1] = bg_image_tag.color[2] = 0x00;
        }
        else if ( tmp_string_buffer[0] == '#' ){
            bg_image_tag.color[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
            bg_image_tag.color[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
            bg_image_tag.color[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );
        }
        else{
            parseTaggedString( tmp_string_buffer, &bg_image_tag );
            bg_effect_image = BG_EFFECT_IMAGE;
        }
        
        int num = readEffect( &p_string_buffer, &tmp_effect );
        if ( num > 1 ) return doEffect( TMP_EFFECT, &bg_image_tag, bg_effect_image );
        else           return doEffect( tmp_effect.effect, &bg_image_tag, bg_effect_image );
    }
    else{
        /* ---------------------------------------- */
        /* Delete all tachi iamges if exist */
        for ( int i=0 ; i<3 ; i++ ){
            if ( tachi_info[i].image_name ){
                delete[] tachi_info[i].image_name;
                tachi_info[i].image_name = NULL;
            }
            if ( tachi_info[i].image_surface ) SDL_FreeSurface( tachi_info[i].image_surface );
            tachi_info[i].image_surface = NULL;
        }

        char *buf = new char[ 512 ];
        sprintf( buf, "bg \"%s\",",tmp_string_buffer );
        makeEffectStr( &p_string_buffer, buf );
        if ( tmp_effect.effect == 0 ) return setEffect( RET_CONTINUE, buf );
        else                          return setEffect( RET_WAIT_NEXT, buf );
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
