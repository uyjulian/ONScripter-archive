/* -*- C++ -*-
 *
 *  ONScripterLabel_rmenu.cpp - Right click menu handler of ONScripter
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

#if defined(LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#elif defined(WIN32)
#include <windows.h>
#elif
#endif

void ONScripterLabel::searchSaveFiles()
{
    unsigned int i;
    char file_name[256];
    
    for ( i=0 ; i<num_save_file ; i++ ){
        sprintf( file_name, "save%d.dat", i+1 );

#if defined(LINUX)
        struct stat buf;
        struct tm *tm;
        if ( stat( file_name, &buf ) != 0 ){
            save_file_info[i].valid = false;
            continue;
        }
        tm = localtime( &buf.st_mtime );
        
        save_file_info[i].valid = true;
        getSJISFromInteger( save_file_info[i].month,  tm->tm_mon + 1 );
        getSJISFromInteger( save_file_info[i].day,    tm->tm_mday );
        getSJISFromInteger( save_file_info[i].hour,   tm->tm_hour );
        getSJISFromInteger( save_file_info[i].minute, tm->tm_min );
#elif defined(WIN32)
        HANDLE  handle;
        FILETIME    tm, ltm;
        SYSTEMTIME  stm;

        handle = CreateFile( file_name, GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( handle == INVALID_HANDLE_VALUE ){
            save_file_info[i].valid = false;
            continue;
        }
            
        GetFileTime( handle, NULL, NULL, &tm );
        FileTimeToLocalFileTime( &tm, &ltm );
        FileTimeToSystemTime( &ltm, &stm );
        CloseHandle( handle );

        save_file_info[i].valid = true;
        getSJISFromInteger( save_file_info[i].month,  stm.wMonth );
        getSJISFromInteger( save_file_info[i].day,    stm.wDay );
        getSJISFromInteger( save_file_info[i].hour,   stm.wHour );
        getSJISFromInteger( save_file_info[i].minute, stm.wMinute );
#else
        FILE *fp;
        if ( (fp = fopen( file_name, "rb" )) == NULL ){
            save_file_info[i].valid = false;
            continue;
        }
        fclose( fp );

        save_file_info[i].valid = true;
        getSJISFromInteger( save_file_info[i].month,  0);
        getSJISFromInteger( save_file_info[i].day,    0);
        getSJISFromInteger( save_file_info[i].hour,   0);
        getSJISFromInteger( save_file_info[i].minute, 0);
#endif
    }
}

int ONScripterLabel::loadSaveFile( int no )
{
    printf("loadSaveFile() %d\n", no);
    FILE *fp;
    char file_name[256], *str = NULL;
    int  i, j, address;
    
    sprintf( file_name, "save%d.dat", no );
    if ( ( fp = fopen( file_name, "rb" ) ) == NULL ){
        fprintf( stderr, "can't open save file %s\n", file_name );
        return -1;
    }

    deleteLabelLink();

    /* ---------------------------------------- */
    /* Load text history */
    loadInt( fp, &text_history_num );
    struct TextBuffer *tb = &text_buffer[0];
    for ( i=0 ; i<text_history_num ; i++ ){
        loadInt( fp, &tb->num_xy[0] );
        loadInt( fp, &tb->num_xy[1] );
        loadInt( fp, &tb->xy[0] );
        loadInt( fp, &tb->xy[1] );
        if ( tb->buffer ) delete[] tb->buffer;
        tb->buffer = new char[ tb->num_xy[0] * tb->num_xy[1] * 2 ];
        for ( j=0 ; j<tb->num_xy[0] * tb->num_xy[1] * 2 ; j++ )
            tb->buffer[j] = fgetc( fp );
        tb = tb->next;
    }
    current_text_buffer = &text_buffer[0];

    /* ---------------------------------------- */
    /* Load sentence font */
    loadInt( fp, &j );
    sentence_font.font_valid_flag = (j==1)?true:false;
    loadInt( fp, &sentence_font.font_size );
    loadInt( fp, &sentence_font.top_xy[0] );
    loadInt( fp, &sentence_font.top_xy[1] );
    loadInt( fp, &sentence_font.num_xy[0] );
    loadInt( fp, &sentence_font.num_xy[1] );
    loadInt( fp, &sentence_font.xy[0] );
    loadInt( fp, &sentence_font.xy[1] );
    loadInt( fp, &sentence_font.pitch_xy[0] );
    loadInt( fp, &sentence_font.pitch_xy[1] );
    loadInt( fp, &sentence_font.wait_time );
    loadInt( fp, &j );
    sentence_font.display_bold = (j==1)?true:false;
    loadInt( fp, &j );
    sentence_font.display_shadow = (j==1)?true:false;
    loadInt( fp, &j );
    sentence_font.display_transparency = (j==1)?true:false;

    /* Dummy, must be removed later !! */
    for ( i=0 ; i<8 ; i++ ){
        loadInt( fp, &j );
        //sentence_font.window_color[i] = j;
    }
    /* Should be char, not integer !! */
    for ( i=0 ; i<3 ; i++ ){
        loadInt( fp, &j );
        sentence_font.window_color[i] = j;
    }
    loadStr( fp, &sentence_font_info.image_name );

    loadInt( fp, &j ); sentence_font_info.pos.x = j; 
    loadInt( fp, &j ); sentence_font_info.pos.y = j;
    loadInt( fp, &j ); sentence_font_info.pos.w = j - sentence_font_info.pos.x + 1;
    loadInt( fp, &j ); sentence_font_info.pos.h = j - sentence_font_info.pos.y + 1;

    if ( !sentence_font.display_transparency ){
        parseTaggedString( sentence_font_info.image_name, &sentence_font_info.tag );
        setupAnimationInfo( &sentence_font_info );
    }
    
    if ( sentence_font.ttf_font ) TTF_CloseFont( (TTF_Font*)sentence_font.ttf_font );
    sentence_font.ttf_font = (void*)TTF_OpenFont( FONT_NAME, sentence_font.font_size );

    loadInt( fp, &clickstr_state );
    loadInt( fp, &j );
    new_line_skip_flag = (j==1)?true:false;
    
    /* ---------------------------------------- */
    /* Load link label info */
    label_stack_depth = 0;

    while( 1 ){
        loadStr( fp, &str );
        current_link_label_info->label_info = lookupLabel( str );
        
        loadInt( fp, &current_link_label_info->current_line );
        loadInt( fp, &current_link_label_info->offset );
        loadInt( fp, &address );
        current_link_label_info->current_script =
            current_link_label_info->label_info.start_address + address;

        if ( fgetc( fp ) == 0 ) break;

        current_link_label_info->next = new LinkLabelInfo();
        current_link_label_info->next->previous = current_link_label_info;
        current_link_label_info = current_link_label_info->next;
        current_link_label_info->next = NULL;
        label_stack_depth++;
    }

    event_mode = fgetc( fp );
    if ( event_mode & WAIT_BUTTON_MODE ) event_mode = WAIT_SLEEP_MODE; // Re-execute the selectCommand, etc.
    
    /* ---------------------------------------- */
    /* Load variables */
    loadVariables( fp, 0, 200 );

    /* ---------------------------------------- */
    /* Load monocro flag */
    monocro_flag = (fgetc( fp )==1)?true:false;
    for ( i=0 ; i<3 ; i++ ) monocro_color[i] = fgetc( fp );
    for ( i=0 ; i<256 ; i++ ){
        monocro_color_lut[i][0] = (monocro_color[0] * i) >> 8;
        monocro_color_lut[i][1] = (monocro_color[1] * i) >> 8;
        monocro_color_lut[i][2] = (monocro_color[2] * i) >> 8;
    }
    
    /* ---------------------------------------- */
    /* Load current images */
    bg_image_tag.color[0] = (unsigned char)fgetc( fp );
    bg_image_tag.color[1] = (unsigned char)fgetc( fp );
    bg_image_tag.color[2] = (unsigned char)fgetc( fp );
    if ( bg_image_tag.color_list ){
        delete[] bg_image_tag.color_list;
        bg_image_tag.color_list = NULL;
    }
    bg_image_tag.num_of_cells = 1;
    loadStr( fp, &bg_image_tag.file_name );
    bg_effect_image = (EFFECT_IMAGE)fgetc( fp );

    for ( i=0 ; i<3 ; i++ ){
        tachi_info[i].deleteImageName();
        tachi_info[i].deleteImageSurface();
    }

    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        sprite_info[i].valid = false;
        sprite_info[i].deleteImageName();
    }

    effect_counter = 0;
    doEffect( 1, &bg_image_tag, bg_effect_image );
    
    /* ---------------------------------------- */
    /* Load Tachi image and Sprite */
    for ( i=0 ; i<3 ; i++ ){
        loadStr( fp, &tachi_info[i].image_name );
        if ( tachi_info[i].image_name ){
            parseTaggedString( tachi_info[i].image_name, &tachi_info[i].tag );
            setupAnimationInfo( &tachi_info[ i ] );
            tachi_info[ i ].pos.x = screen_width * (i+1) / 4 - tachi_info[ i ].pos.w / 2;
            tachi_info[ i ].pos.y = underline_value - tachi_info[ i ].image_surface->h + 1;
        }
    }

    /* ---------------------------------------- */
    /* Load current sprites */
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        loadInt( fp, &j );
        sprite_info[i].valid = (j==1)?true:false;
        loadInt( fp, &j ); sprite_info[i].pos.x = j;
        loadInt( fp, &j ); sprite_info[i].pos.y = j;
        loadInt( fp, &sprite_info[i].trans );
        loadStr( fp, &sprite_info[i].image_name );
        if ( sprite_info[i].image_name ){
            parseTaggedString( sprite_info[i].image_name, &sprite_info[i].tag );
            setupAnimationInfo( &sprite_info[i] );
        }
    }

    effect_counter = 0;
    doEffect( 1, NULL, TACHI_EFFECT_IMAGE );

    shadowTextDisplay();
    restoreTextBuffer();
    flush();
    display_mode = TEXT_DISPLAY_MODE;
    
    /* ---------------------------------------- */
    /* Load current playing CD track */
    stopBGM( false );
    current_cd_track = fgetc( fp );
    mp3_play_once_flag = (fgetc( fp )==1)?true:false;
    if ( current_cd_track == 255 ) current_cd_track = -1;
    loadStr( fp, &mp3_file_name );

    if ( current_cd_track >= 0 || mp3_file_name ){
        if ( cdaudio_flag ){
            if ( cdrom_info && current_cd_track >= 0 ) playCDAudio( current_cd_track );
        }
        else{
            playMP3( current_cd_track );
        }
    }

    /* ---------------------------------------- */
    /* Load rmode flag */
    rmode_flag = (fgetc( fp )==1)?true:false;
    
    fclose( fp );

    return 0;
}

int ONScripterLabel::saveSaveFile( int no )
{
    printf("saveSaveFile() %d\n", no);
    FILE *fp;
    int i, j;
    char file_name[256];
    
    sprintf( file_name, "save%d.dat", no );
    if ( ( fp = fopen( file_name, "wb" ) ) == NULL ){
        fprintf( stderr, "can't open save file %s\n", file_name );
        return -1;
    }

    /* ---------------------------------------- */
    /* Save text history */
    saveInt( fp, text_history_num );
    struct TextBuffer *tb = current_text_buffer;
    for ( i=0 ; i<text_history_num ; i++ ){
        saveInt( fp, tb->num_xy[0] );
        saveInt( fp, tb->num_xy[1] );
        saveInt( fp, tb->xy[0] );
        saveInt( fp, tb->xy[1] );
        for ( j=0 ; j<tb->num_xy[0] * tb->num_xy[1] * 2 ; j++ )
            fputc( tb->buffer[j], fp );
        tb = tb->next;
    }

    /* ---------------------------------------- */
    /* Save sentence font */
    saveInt( fp, (sentence_font.font_valid_flag?1:0) );
    saveInt( fp, sentence_font.font_size );
    saveInt( fp, sentence_font.top_xy[0] );
    saveInt( fp, sentence_font.top_xy[1] );
    saveInt( fp, sentence_font.num_xy[0] );
    saveInt( fp, sentence_font.num_xy[1] );
    saveInt( fp, sentence_font.xy[0] );
    saveInt( fp, sentence_font.xy[1] );
    saveInt( fp, sentence_font.pitch_xy[0] );
    saveInt( fp, sentence_font.pitch_xy[1] );
    saveInt( fp, sentence_font.wait_time );
    saveInt( fp, (sentence_font.display_bold?1:0) );
    saveInt( fp, (sentence_font.display_shadow?1:0) );
    saveInt( fp, (sentence_font.display_transparency?1:0) );
    /* Dummy, must be removed later !! */
    for ( i=0 ; i<8 ; i++ )
        saveInt( fp, 0 );
    /* Should be char, not integer !! */
    for ( i=0 ; i<3 ; i++ )
        saveInt( fp, sentence_font.window_color[i] );
    saveStr( fp, sentence_font_info.image_name );
    saveInt( fp, sentence_font_info.pos.x );
    saveInt( fp, sentence_font_info.pos.y );
    saveInt( fp, sentence_font_info.pos.w - sentence_font_info.pos.x + 1);
    saveInt( fp, sentence_font_info.pos.h - sentence_font_info.pos.y + 1);

    saveInt( fp, clickstr_state );
    saveInt( fp, new_line_skip_flag?1:0 );
    
    /* ---------------------------------------- */
    /* Save link label info */
    LinkLabelInfo *info = &root_link_label_info;

    while( info ){
        fprintf( fp, "%s", info->label_info.name );
        fputc( 0, fp );
        saveInt( fp, info->current_line );
        saveInt( fp, info->offset );
        saveInt( fp, info->current_script - info->label_info.start_address );
        if ( info->next ) fputc( 1, fp );
        info = info->next;
    }
    fputc( 0, fp );

    fputc( shelter_event_mode, fp );
    
    /* ---------------------------------------- */
    /* Save variables */
    saveVariables( fp, 0, 200 );
    
    /* ---------------------------------------- */
    /* Save monocro flag */
    monocro_flag?fputc(1,fp):fputc(0,fp);
    for ( i=0 ; i<3 ; i++ ) fputc( monocro_color[i], fp );
    
    /* ---------------------------------------- */
    /* Save current images */
    fputc( bg_image_tag.color[0], fp );
    fputc( bg_image_tag.color[1], fp );
    fputc( bg_image_tag.color[2], fp );
    saveStr( fp, bg_image_tag.file_name );
    fputc( bg_effect_image, fp );

    saveStr( fp, tachi_info[0].image_name );
    saveStr( fp, tachi_info[1].image_name );
    saveStr( fp, tachi_info[2].image_name );

    /* ---------------------------------------- */
    /* Save current sprites */
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        saveInt( fp, sprite_info[i].valid?1:0 );
        saveInt( fp, sprite_info[i].pos.x );
        saveInt( fp, sprite_info[i].pos.y );
        saveInt( fp, sprite_info[i].trans );
        saveStr( fp, sprite_info[i].image_name );
    }

    /* ---------------------------------------- */
    /* Save current playing CD track */
    if ( mp3_sample ) fputc( current_cd_track, fp );
    else              fputc( -1, fp );
    mp3_play_once_flag?fputc(1,fp):fputc(0,fp);
    saveStr( fp, mp3_file_name );
    
    /* ---------------------------------------- */
    /* Save rmode flag */
    rmode_flag?fputc(1,fp):fputc(0,fp);

    fclose( fp );

    return 0;
}

void ONScripterLabel::leaveSystemCall( bool restore_flag )
{
    int i;

    if ( restore_flag ){
        SDL_BlitSurface( shelter_select_surface, NULL, select_surface, NULL );
        SDL_BlitSurface( shelter_text_surface, NULL, text_surface, NULL );

        flush();
        root_button_link.next = shelter_button_link;
        event_mode = shelter_event_mode;
        if ( event_mode & WAIT_BUTTON_MODE ) SDL_WarpMouse( shelter_mouse_state.x, shelter_mouse_state.y );
        current_text_buffer = shelter_text_buffer;
    }

    for ( i=0 ; i<3 ; i++ ) sentence_font.color[i] = shelter_sentence_color[i];
    system_menu_mode = SYSTEM_NULL;
    system_menu_enter_flag = false;
    key_pressed_flag = false;

    //printf("leaveSystemCall %d %d\n",event_mode, clickstr_state);

    if ( event_mode & WAIT_SLEEP_MODE ){
        event_mode &= ~WAIT_SLEEP_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }
    else{
        startCursor( clickstr_state );
        if ( event_mode & WAIT_ANIMATION_MODE ) startTimer( MINIMUM_TIMER_RESOLUTION );
    }
}

void ONScripterLabel::executeSystemCall()
{
    //printf("*****  executeSystemCall %d %d*****\n", system_menu_enter_flag, volatile_button_state.button );
    int i;
    
    if ( !system_menu_enter_flag ){
        shelter_button_link = root_button_link.next;
        last_button_link = &root_button_link;
        last_button_link->next = NULL;
        SDL_BlitSurface( select_surface, NULL, shelter_select_surface, NULL );
        SDL_BlitSurface( text_surface, NULL, shelter_text_surface, NULL );
        shelter_event_mode = event_mode;
        shelter_mouse_state.x = last_mouse_state.x;
        shelter_mouse_state.y = last_mouse_state.y;
        shelter_text_buffer = current_text_buffer;
        for ( i=0 ; i<3 ; i++ ) shelter_sentence_color[i] = sentence_font.color[i];
        event_mode = IDLE_EVENT_MODE;
        system_menu_enter_flag = true;
    }
    
    switch( system_menu_mode ){
      case SYSTEM_SKIP:
        executeSystemSkip();
        break;
      case SYSTEM_RESET:
        executeSystemReset();
        break;
      case SYSTEM_SAVE:
        executeSystemSave();
        break;
      case SYSTEM_LOAD:
        executeSystemLoad();
        break;
      case SYSTEM_LOOKBACK:
        executeSystemLookback();
        break;
      case SYSTEM_WINDOWERASE:
        executeWindowErase();
        break;
      case SYSTEM_MENU:
        executeSystemMenu();
        break;
      default:
        leaveSystemCall();
    }
}

void ONScripterLabel::executeSystemMenu()
{
    MenuLink *link;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button == -1 ){
            leaveSystemCall();
            return;
        }
    
        int counter = 1;
        link = root_menu_link.next;
        while ( link ){
            if ( current_button_state.button == counter++ ){
                system_menu_mode = link->system_call_no;
            }
            link = link->next;
        }

        startTimer( MINIMUM_TIMER_RESOLUTION );
    }
    else{
        SDL_BlitSurface( shelter_select_surface, NULL, select_surface, NULL );
        shadowTextDisplay();

        menu_font.num_xy[0] = menu_link_width;
        menu_font.num_xy[1] = menu_link_num;
        menu_font.top_xy[0] = (screen_width - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;

        menu_font.xy[0] = (menu_font.num_xy[0] - menu_link_width) / 2;
        menu_font.xy[1] = (menu_font.num_xy[1] - menu_link_num) / 2;


        link = root_menu_link.next;
        int counter = 1;
        while( link ){
            last_button_link->next = getSelectableSentence( link->label, &menu_font, false );
            last_button_link = last_button_link->next;
            last_button_link->no = counter++;

            //printf(" link label %s\n", link->label );
            link = link->next;
            flush();
        }
        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_MENU;
    }
}

void ONScripterLabel::executeSystemSkip()
{
    //printf("ONScripterLabel::executeSystemSkip() %d %d\n", shelter_event_mode, skip_flag );
    skip_flag = true;
    if ( !(shelter_event_mode & WAIT_BUTTON_MODE) )
        shelter_event_mode |= WAIT_SLEEP_MODE;
    leaveSystemCall();
}

void ONScripterLabel::executeSystemReset()
{
    //printf("ONScripterLabel::executeSystemReset() %d\n", event_mode );
    resetCommand();
    event_mode = WAIT_SLEEP_MODE;
    leaveSystemCall( false );
}

void ONScripterLabel::executeWindowErase()
{
    //printf("ONScripterLabel::executeWindowErase() %d\n", event_mode);

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){
        event_mode = IDLE_EVENT_MODE;

        leaveSystemCall();
    }
    else{
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        flush();

        event_mode = WAIT_INPUT_MODE;
        system_menu_mode = SYSTEM_WINDOWERASE;
    }
}

void ONScripterLabel::executeSystemLoad()
{
    unsigned int i;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        if ( loadSaveFile( current_button_state.button ) ){
            event_mode  = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
            refreshMouseOverButton();
            return;
        }
        
        deleteButtonLink();
        deleteSelectLink();

        leaveSystemCall( false );
    }
    else{
        searchSaveFiles();
    
        SDL_BlitSurface( shelter_select_surface, NULL, select_surface, NULL );
        shadowTextDisplay();

        system_font.xy[0] = (system_font.num_xy[0] - strlen( load_menu_name ) / 2) / 2;
        system_font.xy[1] = 0;
        drawString( load_menu_name, system_font.color, &system_font, true, text_surface );
        system_font.xy[1] += 2;
        
        int counter = 1;
        bool nofile_flag;
        char *buffer = new char[ strlen( save_item_name ) + 30 + 1 ];
        for ( i=0 ; i<num_save_file ; i++ ){
            system_font.xy[0] = (system_font.num_xy[0] - (strlen( save_item_name ) / 2 + 15) ) / 2;

            if ( save_file_info[i].valid ){
                sprintf( buffer, "%s%s@%sŒŽ%s“ú%sŽž%s•ª",
                         save_item_name,
                         save_file_info[i].no,
                         save_file_info[i].month,
                         save_file_info[i].day,
                         save_file_info[i].hour,
                         save_file_info[i].minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, "%s%s@||||||||||||",
                         save_item_name,
                         save_file_info[i].no );
                nofile_flag = true;
            }
            last_button_link->next = getSelectableSentence( buffer, &system_font, false, nofile_flag );
            last_button_link = last_button_link->next;
            last_button_link->no = counter++;
            flush();
        }
        delete[] buffer;
        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_LOAD;
    }
}

void ONScripterLabel::executeSystemSave()
{
    unsigned int i;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        saveSaveFile( current_button_state.button );
        leaveSystemCall();
    }
    else{
        searchSaveFiles();

        SDL_BlitSurface( shelter_select_surface, NULL, select_surface, NULL );
        shadowTextDisplay();

        system_font.xy[0] = (system_font.num_xy[0] - strlen( save_menu_name ) / 2 ) / 2;
        system_font.xy[1] = 0;
        drawString( save_menu_name, system_font.color, &system_font, true, text_surface );
        system_font.xy[1] += 2;
        
        int counter = 1;
        bool nofile_flag;
        char *buffer = new char[ strlen( save_item_name ) + 30 + 1 ];
        for ( i=0 ; i<num_save_file ; i++ ){
            system_font.xy[0] = (system_font.num_xy[0] - (strlen( save_item_name ) / 2 + 15) ) / 2;

            if ( save_file_info[i].valid ){
                sprintf( buffer, "%s%s@%sŒŽ%s“ú%sŽž%s•ª",
                         save_item_name,
                         save_file_info[i].no,
                         save_file_info[i].month,
                         save_file_info[i].day,
                         save_file_info[i].hour,
                         save_file_info[i].minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, "%s%s@||||||||||||",
                         save_item_name,
                         save_file_info[i].no );
                nofile_flag = true;
            }
            last_button_link->next = getSelectableSentence( buffer, &system_font, false, nofile_flag );
            last_button_link = last_button_link->next;
            last_button_link->no = counter++;
            flush();
        }
        delete[] buffer;

        SDL_BlitSurface( text_surface, NULL, select_surface, NULL );
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_SAVE;
    }
}

void ONScripterLabel::setupLookbackButton()
{
    /* ---------------------------------------- */
    /* Previous button check */
    if ( (current_text_buffer->previous->xy[1] != -1 ) &&
         current_text_buffer->previous != shelter_text_buffer ){
        last_button_link->next = new ButtonLink();
        last_button_link = last_button_link->next;
    
        last_button_link->button_type = NORMAL_BUTTON;
        last_button_link->no = 1;
        last_button_link->select_rect.x = sentence_font_info.pos.x;
        last_button_link->select_rect.y = sentence_font_info.pos.y;
        last_button_link->select_rect.w = sentence_font_info.pos.w;
        last_button_link->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_image_surface[0] ){
            last_button_link->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - lookback_image_surface[0]->w;
            last_button_link->image_rect.y = sentence_font_info.pos.y;
            last_button_link->image_rect.w = lookback_image_surface[0]->w;
            last_button_link->image_rect.h = lookback_image_surface[0]->h;
            
            last_button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, lookback_image_surface[0]->w, lookback_image_surface[0]->h, 32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( last_button_link->image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
            alphaBlend( last_button_link->image_surface, 0, 0,
                        text_surface, last_button_link->image_rect.x, last_button_link->image_rect.y, lookback_image_surface[0]->w, lookback_image_surface[0]->h,
                        lookback_image_surface[0], 0, 0,
                        0, 0, -lookback_image_tag[0].trans_mode );
        }

        if ( lookback_image_surface[1] )
            alphaBlend( select_surface, last_button_link->image_rect.x, last_button_link->image_rect.y,
                        text_surface, last_button_link->image_rect.x, last_button_link->image_rect.y, lookback_image_surface[1]->w, lookback_image_surface[1]->h,
                        lookback_image_surface[1], 0, 0,
                        0, 0, -lookback_image_tag[1].trans_mode );
        SDL_BlitSurface( select_surface, &last_button_link->image_rect, text_surface, &last_button_link->image_rect );
    }

    /* ---------------------------------------- */
    /* Next button check */
    if ( current_text_buffer->next != shelter_text_buffer ){
        last_button_link->next = new ButtonLink();
        last_button_link = last_button_link->next;
    
        last_button_link->button_type = NORMAL_BUTTON;
        last_button_link->no = 2;
        last_button_link->select_rect.x = sentence_font_info.pos.x;
        last_button_link->select_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h*2/3;
        last_button_link->select_rect.w = sentence_font_info.pos.w;
        last_button_link->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_image_surface[2] ){
            last_button_link->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - lookback_image_surface[2]->w;
            last_button_link->image_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h - lookback_image_surface[2]->h;
            last_button_link->image_rect.w = lookback_image_surface[2]->w;
            last_button_link->image_rect.h = lookback_image_surface[2]->h;
            
            last_button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, lookback_image_surface[2]->w, lookback_image_surface[2]->h, 32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( last_button_link->image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
            alphaBlend( last_button_link->image_surface, 0, 0,
                        text_surface, last_button_link->image_rect.x, last_button_link->image_rect.y, lookback_image_surface[2]->w, lookback_image_surface[2]->h,
                        lookback_image_surface[2], 0, 0,
                        0, 0, -lookback_image_tag[2].trans_mode );
        }

        if ( lookback_image_surface[3] )
            alphaBlend( select_surface, last_button_link->image_rect.x, last_button_link->image_rect.y,
                        text_surface, last_button_link->image_rect.x, last_button_link->image_rect.y, lookback_image_surface[3]->w, lookback_image_surface[3]->h,
                        lookback_image_surface[3], 0, 0,
                        0, 0, -lookback_image_tag[3].trans_mode );
        SDL_BlitSurface( select_surface, &last_button_link->image_rect, text_surface, &last_button_link->image_rect );
    }
    refreshMouseOverButton();
}

void ONScripterLabel::executeSystemLookback()
{
    int i;
    
    SDL_BlitSurface( shelter_select_surface, NULL, select_surface, NULL );
    shadowTextDisplay();
    SDL_BlitSurface( text_surface, NULL, select_surface, NULL );

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){
        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;
        
        deleteButtonLink();
        
        if ( current_button_state.button == 1 ){
            current_text_buffer = current_text_buffer->previous;
        }
        else{
            current_text_buffer = current_text_buffer->next;
        }
        restoreTextBuffer();
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        setupLookbackButton();
        flush();
    }
    else{
        for ( i=0 ; i<3 ; i++ ) sentence_font.color[i] = lookback_color[i];
        shelter_text_buffer = current_text_buffer;
        if ( current_text_buffer->previous->xy[1] == -1 ){
            leaveSystemCall();
            return;
        }
        current_text_buffer = current_text_buffer->previous;

        restoreTextBuffer();
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        setupLookbackButton();
        flush();

        system_menu_mode = SYSTEM_LOOKBACK;
    }
}

