/* -*- C++ -*-
 *
 *  ONScripterLabel_file.cpp - FILE I/O of ONScripter
 *
 *  Copyright (c) 2001-2003 Ogapee. All rights reserved.
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

#if defined(LINUX) || defined(MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#elif defined(WIN32)
#include <windows.h>
#elif defined(MACOS9)
#include <DateTimeUtils.h>
#include <Files.h>
extern "C" void c2pstrcpy(Str255 dst, const char *src);	//#include <TextUtils.h>
#else
#endif

#define SAVEFILE_MAGIC_NUMBER "ONS"
#define SAVEFILE_VERSION_MAJOR 2
#define SAVEFILE_VERSION_MINOR 1

#define READ_LENGTH 4096

void ONScripterLabel::searchSaveFile( SaveFileInfo &save_file_info, int no )
{
    char file_name[256];

    getSJISFromInteger( save_file_info.sjis_no, no );
#if defined(LINUX) || defined(MACOSX)
    sprintf( file_name, "%ssave%d.dat", archive_path, no );
    struct stat buf;
    struct tm *tm;
    if ( stat( file_name, &buf ) != 0 ){
        save_file_info.valid = false;
        return;
    }
    tm = localtime( &buf.st_mtime );
        
    save_file_info.month  = tm->tm_mon + 1;
    save_file_info.day    = tm->tm_mday;
    save_file_info.hour   = tm->tm_hour;
    save_file_info.minute = tm->tm_min;
#elif defined(WIN32)
    sprintf( file_name, "%ssave%d.dat", archive_path, no );
    HANDLE  handle;
    FILETIME    tm, ltm;
    SYSTEMTIME  stm;

    handle = CreateFile( file_name, GENERIC_READ, 0, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( handle == INVALID_HANDLE_VALUE ){
        save_file_info.valid = false;
        return;
    }
            
    GetFileTime( handle, NULL, NULL, &tm );
    FileTimeToLocalFileTime( &tm, &ltm );
    FileTimeToSystemTime( &ltm, &stm );
    CloseHandle( handle );

    save_file_info.month  = stm.wMonth;
    save_file_info.day    = stm.wDay;
    save_file_info.hour   = stm.wHour;
    save_file_info.minute = stm.wMinute;
#elif defined(MACOS9)
	sprintf( file_name, "%ssave%d.dat", archive_path, no );
	CInfoPBRec  pb;
	Str255      p_file_name;
	FSSpec      file_spec;
	DateTimeRec tm;
	c2pstrcpy( p_file_name, file_name );
	if ( FSMakeFSSpec(0, 0, p_file_name, &file_spec) != noErr ){
		save_file_info.valid = false;
		return;
	}
	pb.hFileInfo.ioNamePtr = file_spec.name;
	pb.hFileInfo.ioVRefNum = file_spec.vRefNum;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = file_spec.parID;
	if (PBGetCatInfoSync(&pb) != noErr) {
		save_file_info.valid = false;
		return;
	}
	SecondsToDate( pb.hFileInfo.ioFlMdDat, &tm );
	save_file_info.month  = tm.month;
	save_file_info.day    = tm.day;
	save_file_info.hour   = tm.hour;
	save_file_info.minute = tm.minute;
#else
    sprintf( file_name, "save%d.dat", no );
    FILE *fp;
    if ( (fp = fopen( file_name, "rb" )) == NULL ){
        save_file_info.valid = false;
        return;
    }
    fclose( fp );

    save_file_info.month  = 1;
    save_file_info.day    = 1;
    save_file_info.hour   = 0;
    save_file_info.minute = 0;
#endif
    save_file_info.valid = true;
    getSJISFromInteger( save_file_info.sjis_month,  save_file_info.month );
    getSJISFromInteger( save_file_info.sjis_day,    save_file_info.day );
    getSJISFromInteger( save_file_info.sjis_hour,   save_file_info.hour );
    getSJISFromInteger( save_file_info.sjis_minute, save_file_info.minute );
}

int ONScripterLabel::loadSaveFile( int no )
{
    FILE *fp;
    char file_name[256], *str = NULL;
    int  i, j, k, address;
    int  file_version;
    
    sprintf( file_name, "save%d.dat", no );
    if ( ( fp = fopen( file_name, "rb" ) ) == NULL ){
        fprintf( stderr, "can't open save file %s\n", file_name );
        return -1;
    }

    /* ---------------------------------------- */
    /* Load magic number */
    for ( i=0 ; i<(int)strlen( SAVEFILE_MAGIC_NUMBER ) ; i++ )
        if ( fgetc( fp ) != SAVEFILE_MAGIC_NUMBER[i] ) break;
    if ( i != (int)strlen( SAVEFILE_MAGIC_NUMBER ) ){ // if not ONS save file
        fseek( fp, 0, SEEK_SET );
        // check for ONS version 0
        bool ons_ver0_flag = false;
        loadInt( fp, &j );
        if ( j != 1 ) ons_ver0_flag = true;
        loadInt( fp, &j );
        loadInt( fp, &j );
        loadInt( fp, &j );
        if ( j != 0 ) ons_ver0_flag = true;
        loadInt( fp, &j );
        loadInt( fp, &j );
        if ( j != 0xff ) ons_ver0_flag = true;
        loadInt( fp, &j );
        if ( j != 0xff ) ons_ver0_flag = true;
        loadInt( fp, &j );
        if ( j != 0xff ) ons_ver0_flag = true;
        
        fseek( fp, 0, SEEK_SET );
        if ( !ons_ver0_flag ){
            printf("Save file version is unknown\n" );
            return loadSaveFile2( fp, SAVEFILE_VERSION_MAJOR*100 + SAVEFILE_VERSION_MINOR );
        }
        file_version = 0;
    }
    else{
        file_version = (fgetc( fp ) * 100) + fgetc( fp );
    }
    printf("Save file version is %d.%d\n", file_version/100, file_version%100 );
    if ( file_version > SAVEFILE_VERSION_MAJOR*100 + SAVEFILE_VERSION_MINOR ){
        fprintf( stderr, "Save file is newer than %d.%d, please use the latest ONScripter.\n", SAVEFILE_VERSION_MAJOR, SAVEFILE_VERSION_MINOR );
        return -1;
    }

    if ( file_version >= 200 )
        return loadSaveFile2( fp, file_version );
    
    deleteLabelLink();

    /* ---------------------------------------- */
    /* Load text history */
    if ( file_version >= 107 )
        loadInt( fp, &i );
    else
        i = 0;
    sentence_font.setTateyokoMode( i );
    int text_history_num;
    loadInt( fp, &text_history_num );
    struct TextBuffer *tb = current_text_buffer;
    for ( i=0 ; i<text_history_num ; i++ ){
        loadInt( fp, &tb->num_xy[0] );
        loadInt( fp, &tb->num_xy[1] );
        int xy[2];
        loadInt( fp, &xy[0] );
        loadInt( fp, &xy[1] );
        if ( tb->buffer2 ) delete[] tb->buffer2;
        tb->buffer2_count = 0;
        tb->buffer2 = new char[ (tb->num_xy[0]*2+1) * tb->num_xy[1] + 1 ];

        char ch1, ch2;
        for ( j=0, k=0 ; j<tb->num_xy[0] * tb->num_xy[1] ; j++ ){
            ch1 = fgetc( fp );
            ch2 = fgetc( fp );
            if ( ch1 == ((char*)"�@")[0] &&
                 ch2 == ((char*)"�@")[1] ){
                k += 2;
            }
            else{
                if ( ch1 ){
                    tb->buffer2[tb->buffer2_count++] = ch1;
                    k++;
                }
                if ( ch1 & 0x80 || ch2 ){
                    tb->buffer2[tb->buffer2_count++] = ch2;
                    k++;
                }
            }
            if ( k >= tb->num_xy[0] * 2 ){
                tb->buffer2[tb->buffer2_count++] = 0x0a;
                k = 0;
            }
        }
        tb = tb->next;
        if ( i==0 ){
            for ( j=0 ; j<max_text_buffer - text_history_num ; j++ )
                tb = tb->next;
            start_text_buffer = tb;
        }
    }

    /* ---------------------------------------- */
    /* Load sentence font */
    loadInt( fp, &j );
    //sentence_font.is_valid = (j==1)?true:false;
    loadInt( fp, &sentence_font.font_size_xy[0] );
    if ( file_version >= 100 ){
        loadInt( fp, &sentence_font.font_size_xy[1] );
    }
    else{
        sentence_font.font_size_xy[1] = sentence_font.font_size_xy[0];
    }
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
    sentence_font.is_bold = (j==1)?true:false;
    loadInt( fp, &j );
    sentence_font.is_shadow = (j==1)?true:false;
    loadInt( fp, &j );
    sentence_font.is_transparent = (j==1)?true:false;

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

    loadInt( fp, &j ); sentence_font_info.pos.x = j * screen_ratio1 / screen_ratio2; 
    loadInt( fp, &j ); sentence_font_info.pos.y = j * screen_ratio1 / screen_ratio2;
    loadInt( fp, &j ); sentence_font_info.pos.w = j * screen_ratio1 / screen_ratio2;
    loadInt( fp, &j ); sentence_font_info.pos.h = j * screen_ratio1 / screen_ratio2;

    if ( !sentence_font.is_transparent ){
        parseTaggedString( &sentence_font_info );
        setupAnimationInfo( &sentence_font_info );
    }

    sentence_font.ttf_font = NULL;

    loadInt( fp, &clickstr_state );
    loadInt( fp, &j );
    new_line_skip_flag = (j==1)?true:false;
    if ( file_version >= 103 ){
        loadInt( fp, &erase_text_window_mode );
    }
    
    /* ---------------------------------------- */
    /* Load link label info */

    while( 1 ){
        loadStr( fp, &str );
        current_link_label_info->label_info = script_h.lookupLabel( str );
        
        loadInt( fp, &current_link_label_info->current_line );
        current_link_label_info->current_line++;
        char *buf = current_link_label_info->label_info.label_header;
        while( buf < current_link_label_info->label_info.start_address ){
            if ( *buf == 0x0a )
                current_link_label_info->current_line--;
            buf++;
        }

        int offset;
        loadInt( fp, &offset );
            
        script_h.setCurrent( current_link_label_info->label_info.start_address, false );
        script_h.skipLine( current_link_label_info->current_line );
        current_link_label_info->current_script = script_h.getCurrent() + offset;

        if ( file_version <= 104 )
        {
            if ( file_version >= 102 )
                loadInt( fp, &j );

            loadInt( fp, &address );
            current_link_label_info->string_buffer_offset = 0;
        }
        else{
            loadInt( fp, &current_link_label_info->string_buffer_offset );
        }
        
        if ( fgetc( fp ) == 0 ) break;

        if ( file_version<= 105 )
            current_link_label_info->string_buffer_offset = 0;
        
        current_link_label_info->next = new LinkLabelInfo();
        current_link_label_info->next->previous = current_link_label_info;
        current_link_label_info = current_link_label_info->next;
        current_link_label_info->next = NULL;
    }
    script_h.setCurrent( current_link_label_info->current_script );
    string_buffer_offset = current_link_label_info->string_buffer_offset;
    
    int tmp_event_mode = fgetc( fp );

    /* ---------------------------------------- */
    /* Load variables */
    loadVariables( fp, 0, 200 );

    /* ---------------------------------------- */
    /* Load monocro flag */
    monocro_flag = (fgetc( fp )==1)?true:false;
    if ( file_version >= 101 ){
        monocro_flag_new = (fgetc( fp )==1)?true:false;
    }
    for ( i=0 ; i<3 ; i++ ) monocro_color[i] = fgetc( fp );

    if ( file_version >= 101 ){
        for ( i=0 ; i<3 ; i++ ) monocro_color_new[i] = fgetc( fp );
        need_refresh_flag = (fgetc( fp )==1)?true:false;
    }
    else{
        need_refresh_flag = false;
    }
    
    for ( i=0 ; i<256 ; i++ ){
        monocro_color_lut[i][0] = (monocro_color[0] * i) >> 8;
        monocro_color_lut[i][1] = (monocro_color[1] * i) >> 8;
        monocro_color_lut[i][2] = (monocro_color[2] * i) >> 8;
    }
    
    /* Load nega flag */
    if ( file_version >= 104 ){
        nega_mode = (unsigned char)fgetc( fp );
    }

    /* ---------------------------------------- */
    /* Load current images */
    bg_info.remove();
    bg_info.color[0] = (unsigned char)fgetc( fp );
    bg_info.color[1] = (unsigned char)fgetc( fp );
    bg_info.color[2] = (unsigned char)fgetc( fp );
    bg_info.num_of_cells = 1;
    loadStr( fp, &bg_info.file_name );
    setupAnimationInfo( &bg_info );
    bg_effect_image = (EFFECT_IMAGE)fgetc( fp );

    for ( i=0 ; i<3 ; i++ ){
        tachi_info[i].remove();
    }

    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        sprite_info[i].remove();
    }

    /* ---------------------------------------- */
    /* Load Tachi image and Sprite */
    for ( i=0 ; i<3 ; i++ ){
        loadStr( fp, &tachi_info[i].image_name );
        if ( tachi_info[i].image_name ){
            parseTaggedString( &tachi_info[i] );
            setupAnimationInfo( &tachi_info[ i ] );
            tachi_info[ i ].pos.x = screen_width * (i+1) / 4 - tachi_info[ i ].pos.w / 2;
            tachi_info[ i ].pos.y = underline_value - tachi_info[ i ].image_surface->h + 1;
        }
    }

    /* ---------------------------------------- */
    /* Load current sprites */
    for ( i=0 ; i<256 ; i++ ){
        loadInt( fp, &j );
        sprite_info[i].visible = (j==1)?true:false;
        loadInt( fp, &j ); sprite_info[i].pos.x = j * screen_ratio1 / screen_ratio2;
        loadInt( fp, &j ); sprite_info[i].pos.y = j * screen_ratio1 / screen_ratio2;
        loadInt( fp, &sprite_info[i].trans );
        loadStr( fp, &sprite_info[i].image_name );
        if ( sprite_info[i].image_name ){
            parseTaggedString( &sprite_info[i] );
            setupAnimationInfo( &sprite_info[i] );
        }
    }

    /* ---------------------------------------- */
    /* Load current playing CD track */
    stopBGM( false );
    current_cd_track = (Sint8)fgetc( fp );
    bool play_once_flag = (fgetc( fp )==1)?true:false;
    if ( current_cd_track == -2 ){
        loadStr( fp, &midi_file_name );
        midi_play_loop_flag = !play_once_flag;
        setStr( &music_file_name, NULL );
        music_play_loop_flag = false;
    }
    else{
        loadStr( fp, &music_file_name );
        if ( music_file_name ){
            music_play_loop_flag = !play_once_flag;
            cd_play_loop_flag = false;
        }
        else{
            music_play_loop_flag = false;
            cd_play_loop_flag = !play_once_flag;
        }
        setStr( &midi_file_name, NULL );
        midi_play_loop_flag = false;
    }
    
    if ( current_cd_track >= 0 ){
        if ( cdaudio_flag ){
            if ( cdrom_info ) playCDAudio( current_cd_track );
        }
        else{
            playMP3( current_cd_track );
        }
    }
    else if ( midi_file_name && midi_play_loop_flag ){
        playMIDIFile();
    }
    else if ( music_file_name && music_play_loop_flag ){
        if ( playWave( music_file_name, music_play_loop_flag, ONS_MIX_CHANNELS-1 ) )
#if defined(EXTERNAL_MUSIC_PLAYER)
            playMusicFile();
#else
            playMP3( 0 );
#endif
        else{
            setStr( &wave_file_name, music_file_name );
            wave_play_loop_flag = music_play_loop_flag;
            setStr( &music_file_name, NULL );
            music_play_loop_flag = false;
        }
    }

    /* ---------------------------------------- */
    /* Load rmode flag */
    rmode_flag = (fgetc( fp )==1)?true:false;
    
    /* ---------------------------------------- */
    /* Load text on flag */
    text_on_flag = (fgetc( fp )==1)?true:false;

    fclose( fp );

    refreshSurface( accumulation_surface, NULL, REFRESH_NORMAL_MODE );
    refreshSurface( text_surface, NULL, REFRESH_SHADOW_MODE );
    flush();
    display_mode = next_display_mode = TEXT_DISPLAY_MODE;

    event_mode = tmp_event_mode;
    if ( event_mode & WAIT_BUTTON_MODE ) event_mode = WAIT_SLEEP_MODE; // Re-execute the selectCommand, etc.

    if ( event_mode & WAIT_SLEEP_MODE )
        event_mode &= ~WAIT_SLEEP_MODE;
    else
        event_mode |= WAIT_TIMER_MODE;
    
    return 0;
}

void ONScripterLabel::saveMagicNumber( FILE *fp )
{
    for ( unsigned int i=0 ; i<strlen( SAVEFILE_MAGIC_NUMBER ) ; i++ )
        fputc( SAVEFILE_MAGIC_NUMBER[i], fp );
    fputc( SAVEFILE_VERSION_MAJOR, fp );
    fputc( SAVEFILE_VERSION_MINOR, fp );
}

void ONScripterLabel::saveSaveFileFromTmpFile( FILE *fp )
{
    int c;
    long len;
    char *buf = new char[ READ_LENGTH ];
        
    fseek( tmp_save_fp, 0, SEEK_END );
    len = ftell( tmp_save_fp );
    fseek( tmp_save_fp, 0, SEEK_SET );
    while( len > 0 ){
        if ( len > READ_LENGTH ) c = READ_LENGTH;
        else                     c = len;
        len -= c;
        fread( buf, 1, c, tmp_save_fp );
        fwrite( buf, 1, c, fp );
    }

    delete[] buf;
}

int ONScripterLabel::saveSaveFile( int no )
{
    if ( no >= 0 )
    {
        FILE *fp;
        char file_name[256];

        saveAll();

        sprintf( file_name, "save%d.dat", no );
        if ( ( fp = fopen( file_name, "wb" ) ) == NULL ){
            fprintf( stderr, "can't open save file %s for writing\n", file_name );
            return -1;
        }

        if ( !saveon_flag || !internal_saveon_flag ){
            saveMagicNumber( fp );
            saveSaveFileFromTmpFile( fp );
            fclose( fp );
        }
        else{
            saveMagicNumber( fp );
            saveSaveFile2( fp );
            fclose(fp);
        }

        sprintf( file_name, RELATIVEPATH "sav%csave%d.dat", DELIMITER, no );
        if ( ( fp = fopen( file_name, "wb" ) ) == NULL ){
            fprintf( stderr, "can't open save file %s for writing (not error)\n", file_name );
            return 0;
        }

        if ( !saveon_flag || !internal_saveon_flag ){
            saveSaveFileFromTmpFile( fp );
            fclose( fp );
        }
        else{
            saveSaveFile2( fp );
            fclose(fp);
        }
    }
    else{
        if ( tmp_save_fp ) fclose( tmp_save_fp );
        if ( (tmp_save_fp = tmpfile()) == NULL ){
            fprintf( stderr, "can't open tmp_file\n");
            return -1;
        }
        saveSaveFile2( tmp_save_fp );
    }

    return 0;
}
