/* -*- C++ -*-
 * 
 *  ONScripterLabel.cpp - Execution block analyzer of ONScripter
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

extern void initSJIS2UTF16();

#define FONT_SIZE 26

#define DEFAULT_DECODEBUF 16384
#define DEFAULT_AUDIOBUF  4096

#define FONT_FILE "default.ttf"
#define REGISTRY_FILE "registry.txt"

#define DEFAULT_TEXT_SPEED1 40 // Low speed
#define DEFAULT_TEXT_SPEED2 20 // Middle speed
#define DEFAULT_TEXT_SPEED3 10 // High speed

extern void mp3callback( void *userdata, Uint8 *stream, int len );
extern Uint32 cdaudioCallback( Uint32 interval, void *param );
extern SDL_TimerID timer_cdaudio_id;

typedef int (ONScripterLabel::*FuncList)();
static struct FuncLUT{
    char command[40];
    FuncList method;
} func_lut[] = {
    {"wavestop",   &ONScripterLabel::wavestopCommand},
    {"waveloop",   &ONScripterLabel::waveCommand},
    {"wave",   &ONScripterLabel::waveCommand},
    {"waittimer",   &ONScripterLabel::waittimerCommand},
    {"wait",   &ONScripterLabel::waitCommand},
    {"vsp",   &ONScripterLabel::vspCommand},
    {"voicevol",   &ONScripterLabel::voicevolCommand},
    {"trap",   &ONScripterLabel::trapCommand},
    {"textspeed",   &ONScripterLabel::textspeedCommand},
    {"texton",   &ONScripterLabel::textonCommand},
    {"textoff",   &ONScripterLabel::textoffCommand},
    {"textclear",   &ONScripterLabel::textclearCommand},
    {"systemcall",   &ONScripterLabel::systemcallCommand},
    {"stop",   &ONScripterLabel::stopCommand},
    {"spstr",   &ONScripterLabel::spstrCommand},
    {"spbtn",   &ONScripterLabel::spbtnCommand},
    {"sevol",   &ONScripterLabel::sevolCommand},
    {"setwindow",   &ONScripterLabel::setwindowCommand},
    {"setcursor",   &ONScripterLabel::setcursorCommand},
    {"selnum",   &ONScripterLabel::selectCommand},
    {"selgosub",   &ONScripterLabel::selectCommand},
    {"select",   &ONScripterLabel::selectCommand},
    {"savegame",   &ONScripterLabel::savegameCommand},
    {"rnd",   &ONScripterLabel::rndCommand},
    {"rnd2",   &ONScripterLabel::rndCommand},
    {"rmode",   &ONScripterLabel::rmodeCommand},
    {"resettimer",   &ONScripterLabel::resettimerCommand},
    {"reset",   &ONScripterLabel::resetCommand},
    {"quakey",   &ONScripterLabel::quakeCommand},
    {"quakex",   &ONScripterLabel::quakeCommand},
    {"quake",   &ONScripterLabel::quakeCommand},
    {"puttext",   &ONScripterLabel::puttextCommand},
    {"print",   &ONScripterLabel::printCommand},
    {"playstop",   &ONScripterLabel::playstopCommand},
    {"playonce",   &ONScripterLabel::playCommand},
    {"play",   &ONScripterLabel::playCommand},
    {"msp", &ONScripterLabel::mspCommand},
    {"mp3vol", &ONScripterLabel::mp3volCommand},
    {"mp3save", &ONScripterLabel::mp3Command},
    {"mp3loop", &ONScripterLabel::mp3Command},
    {"mp3", &ONScripterLabel::mp3Command},
    {"monocro", &ONScripterLabel::monocroCommand},
    {"lsph", &ONScripterLabel::lspCommand},
    {"lsp", &ONScripterLabel::lspCommand},
    {"lookbackflush", &ONScripterLabel::lookbackflushCommand},
    {"locate", &ONScripterLabel::locateCommand},
    {"loadgame", &ONScripterLabel::loadgameCommand},
    {"ld", &ONScripterLabel::ldCommand},
    {"jumpf", &ONScripterLabel::jumpfCommand},
    {"jumpb", &ONScripterLabel::jumpbCommand},
    {"getversion", &ONScripterLabel::getversionCommand},
    {"gettimer", &ONScripterLabel::gettimerCommand},
    {"getreg", &ONScripterLabel::getregCommand},
    {"game", &ONScripterLabel::gameCommand},
    {"exbtn_d", &ONScripterLabel::exbtnCommand},
    {"exbtn", &ONScripterLabel::exbtnCommand},
    {"erasetextwindow", &ONScripterLabel::erasetextwindowCommand},
    {"end", &ONScripterLabel::endCommand},
    {"dwavestop", &ONScripterLabel::dwavestopCommand},
    {"dwaveloop", &ONScripterLabel::dwaveCommand},
    {"dwave", &ONScripterLabel::dwaveCommand},
    {"delay", &ONScripterLabel::delayCommand},
    {"csp", &ONScripterLabel::cspCommand},
    {"click", &ONScripterLabel::clickCommand},
    {"cl", &ONScripterLabel::clCommand},
    {"cell", &ONScripterLabel::cellCommand},
    {"caption", &ONScripterLabel::captionCommand},
    {"btnwait2", &ONScripterLabel::btnwaitCommand},
    {"btnwait", &ONScripterLabel::btnwaitCommand},
    {"btndef",  &ONScripterLabel::btndefCommand},
    {"btn",     &ONScripterLabel::btnCommand},
    {"br",      &ONScripterLabel::brCommand},
    {"blt",      &ONScripterLabel::bltCommand},
    {"bg",      &ONScripterLabel::bgCommand},
    {"autoclick",      &ONScripterLabel::autoclickCommand},
    {"amsp",      &ONScripterLabel::amspCommand},
    {"abssetcursor", &ONScripterLabel::setcursorCommand},
    {"", NULL}
};

int ONScripterLabel::SetVideoMode()
{
    /* ---------------------------------------- */
    /* Initialize SDL */
    if ( TTF_Init() < 0 ){
        fprintf( stderr, "can't initialize SDL TTF\n");
        SDL_Quit();
        exit(-1);
    }
	screen_surface = SDL_SetVideoMode( screen_width, screen_height, 32, DEFAULT_SURFACE_FLAG );
	if ( screen_surface == NULL ) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
					screen_width, screen_height, 32, SDL_GetError());
		return(-1);
	}

	//SDL_UpdateRect(screen_surface, 0, 0, 0, 0);

    initSJIS2UTF16();
    
    wm_title_string = new char[ strlen(DEFAULT_WM_TITLE) + 1 ];
    memcpy( wm_title_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_TITLE) + 1 );
    wm_icon_string = new char[ strlen(DEFAULT_WM_ICON) + 1 ];
    memcpy( wm_icon_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_ICON) + 1 );
    SDL_WM_SetCaption( wm_title_string, wm_icon_string );
    
	return(0);
}

ONScripterLabel::ONScripterLabel( bool cdaudio_flag, char *default_font, char *default_registry, bool edit_flag )
{
    int i;

    if ( open() ) exit(-1);

    printf("ONScripter\n");

	if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO|SDL_INIT_CDROM) < 0 ) {
		fprintf(stderr,
			"Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

    SetVideoMode();

    if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF ) < 0 ){
        fprintf(stderr, "Couldn't open audio device!\n"
                "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else{
        int freq;
        Uint16 format;
        int channels;

        Mix_QuerySpec( &freq, &format, &channels);
        printf("Opened audio at %d Hz %d bit %s\n", freq,
               (format&0xFF),
               (channels > 1) ? "stereo" : "mono");
        audio_format.format = format;
        audio_format.freq = freq;
        audio_format.channels = channels;

        audio_open_flag = true;
    }
    
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    background_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( background_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    accumulation_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( accumulation_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    select_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( select_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    text_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( text_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    effect_src_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( effect_src_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    effect_dst_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( effect_dst_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    shelter_select_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( shelter_select_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    shelter_text_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, screen_width, screen_height, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( shelter_text_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );

    debug_flag = false;
    internal_timer = SDL_GetTicks();
    autoclick_timer = 0;

    monocro_flag = false;
    trap_flag = false;
    trap_dist = NULL;
    
    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    skip_flag = false;
    draw_one_page_flag = false;
    key_pressed_flag = false;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    
    last_button_link = &root_button_link;
    btndef_surface = NULL;
    current_over_button = 0;

    this->edit_flag = edit_flag;

    /* ---------------------------------------- */
    /* Sound related variables */
    this->cdaudio_flag = cdaudio_flag;
    if ( cdaudio_flag ){
        if ( SDL_CDNumDrives() > 0 ) cdrom_info = SDL_CDOpen( 0 );
        if ( !cdrom_info ){
            fprintf(stderr, "Couldn't open default CD-ROM: %s\n", SDL_GetError());
        }
        else if ( cdrom_info && !CD_INDRIVE( SDL_CDStatus( cdrom_info ) ) ) {
            SDL_CDClose( cdrom_info );
            cdrom_info = NULL;
        }
    }
    else{
        cdrom_info = NULL;
    }
    
    mp3_sample = NULL;
    mp3_file_name = NULL;
    mp3_buffer = NULL;
    current_cd_track = -1;
    for ( i=0 ; i<MIX_CHANNELS ; i++ ) wave_sample[i] = NULL;
    
    /* ---------------------------------------- */
    /* Initialize registry */
    registry_file = NULL;
    if ( default_registry ) setStr( &registry_file, default_registry );
    else                    setStr( &registry_file, REGISTRY_FILE );

    /* ---------------------------------------- */
    /* Initialize font */
    font_file = NULL;
    if ( default_font ) setStr( &font_file, default_font );
    else                setStr( &font_file, FONT_FILE );
    
    text_char_flag = false;
    default_text_speed[0] = DEFAULT_TEXT_SPEED1;
    default_text_speed[1] = DEFAULT_TEXT_SPEED2;
    default_text_speed[2] = DEFAULT_TEXT_SPEED3;
    text_speed_no = 1;
    
    new_line_skip_flag = false;
    erase_text_window_flag = true;
    text_on_flag = true;
    sentence_font.ttf_font = (void*)TTF_OpenFont( font_file, FONT_SIZE );
    if ( !sentence_font.ttf_font ){
        fprintf( stderr, "can't open font file: %s\n", font_file );
        SDL_Quit();
        exit(-1);
    }
    sentence_font.font_valid_flag = true;
    sentence_font.color[0] = sentence_font.color[1] = sentence_font.color[2] = 0xff;
    sentence_font.font_size = FONT_SIZE;
    sentence_font.top_xy[0] = 8;
    sentence_font.top_xy[1] = 16;// + sentence_font.font_size;
    sentence_font.num_xy[0] = 23;
    sentence_font.num_xy[1] = 16;
    sentence_font.pitch_xy[0] = sentence_font.font_size;
    sentence_font.pitch_xy[1] = 2 + sentence_font.font_size;
    sentence_font.wait_time = default_text_speed[text_speed_no];
    sentence_font.display_bold = true;
    sentence_font.display_shadow = true;
    sentence_font.display_transparency = true;
    sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0x99;
    sentence_font.on_color[0] = sentence_font.on_color[1] = sentence_font.on_color[2] = 0xff;
    sentence_font.off_color[0] = sentence_font.off_color[1] = sentence_font.off_color[2] = 0x80;
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width;
    sentence_font_info.pos.h = screen_height;

    
    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;

    /* ---------------------------------------- */
    /* Effect related variables */
    effect_mask_surface = NULL;
    
    bg_effect_image = COLOR_EFFECT_IMAGE;

    clearCurrentTextBuffer();
}

ONScripterLabel::~ONScripterLabel( )
{
}

void ONScripterLabel::flush( SDL_Rect *rect )
{
    SDL_BlitSurface( text_surface, rect, screen_surface, rect );
    if ( rect )
        SDL_UpdateRect( screen_surface, rect->x, rect->y, rect->w, rect->h );
    else
        SDL_UpdateRect( screen_surface, 0, 0, 0, 0 );
}

void ONScripterLabel::flush( int x, int y, int w, int h )
{
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    flush( &rect );
}

// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
void ONScripterLabel::proceedAnimation( AnimationInfo *anim )
{
    if ( anim->tag.loop_mode != 3 && anim->tag.num_of_cells > 1 )
        anim->tag.current_cell += anim->tag.direction;

    if ( anim->tag.current_cell < 0 ){ // loop_mode must be 2
        anim->tag.current_cell = 1;
        anim->tag.direction = 1;
    }
    else if ( anim->tag.current_cell >= anim->tag.num_of_cells ){
        if ( anim->tag.loop_mode == 0 ){
            anim->tag.current_cell = 0;
        }
        else if ( anim->tag.loop_mode == 1 ){
            anim->tag.current_cell = anim->tag.num_of_cells - 1;
        }
        else{
            anim->tag.current_cell = anim->tag.num_of_cells - 2;
            anim->tag.direction = -1;
        }
    }
}

void ONScripterLabel::showAnimation( AnimationInfo *anim )
{
    if ( anim->tag.duration_list ){
        if ( anim->image_surface ){
            SDL_Rect src_rect, dst_rect;
            src_rect.x = anim->image_surface->w * anim->tag.current_cell / anim->tag.num_of_cells;
            src_rect.y = 0;
            src_rect.w = anim->pos.w;
            src_rect.h = anim->pos.h;
            if ( anim->abs_flag ) {
                dst_rect.x = anim->pos.x;
                dst_rect.y = anim->pos.y;
            }
            else{
                dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + anim->pos.x;
                dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + anim->pos.y;
            }
            alphaBlend( text_surface, dst_rect.x, dst_rect.y,
                        anim->preserve_surface, 0, 0, anim->preserve_surface->w, anim->preserve_surface->h,
                        anim->image_surface, src_rect.x, src_rect.y,
                        0, 0, -anim->tag.trans_mode );
            //SDL_BlitSurface( anim->image_surface, &src_rect, text_surface, &dst_rect );
            flush( dst_rect.x, dst_rect.y, src_rect.w, src_rect.h );
        }

        proceedAnimation( anim );
    }
}

void ONScripterLabel::mouseOverCheck( int x, int y )
{
    int c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;
    
    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    ButtonLink *p_button_link = root_button_link.next;
    while( p_button_link ){
        if ( x >= p_button_link->select_rect.x && x < p_button_link->select_rect.x + p_button_link->select_rect.w &&
             y >= p_button_link->select_rect.y && y < p_button_link->select_rect.y + p_button_link->select_rect.h ){
            button = p_button_link->no;
            break;
        }
        p_button_link = p_button_link->next;
        c++;
    }
    if ( current_over_button != button ){
        if ( event_mode & WAIT_BUTTON_MODE && !first_mouse_over_flag ){
            if ( current_button_link.button_type == NORMAL_BUTTON ){
                //SDL_BlitSurface( select_surface, &current_button_link.image_rect, text_surface, &current_button_link.image_rect );
            }
            else if ( current_button_link.button_type == SPRITE_BUTTON || current_button_link.button_type == EX_SPRITE_BUTTON ){
                sprite_info[ current_button_link.sprite_no ].tag.current_cell = 0;
                //refreshAccumulationSurface( select_surface, &current_button_link.image_rect );
                //SDL_BlitSurface( accumulation_surface, &current_button_link.image_rect, text_surface, &current_button_link.image_rect );
            }
            SDL_BlitSurface( select_surface, &current_button_link.image_rect, text_surface, &current_button_link.image_rect );
            flush( &current_button_link.image_rect );
        }
        first_mouse_over_flag = false;

        if ( p_button_link ){
            if ( event_mode & WAIT_BUTTON_MODE ){
                if ( p_button_link->button_type == NORMAL_BUTTON && p_button_link->image_surface ){
                    SDL_BlitSurface( p_button_link->image_surface, NULL, text_surface, &p_button_link->image_rect );
                }
                else if ( p_button_link->button_type == SPRITE_BUTTON || p_button_link->button_type == EX_SPRITE_BUTTON ){
                    sprite_info[ p_button_link->sprite_no ].tag.current_cell = 1;
                    refreshAccumulationSurface( text_surface, &p_button_link->image_rect );
                    if ( p_button_link->button_type == EX_SPRITE_BUTTON ){
                        drawExbtn( text_surface, p_button_link->exbtn_ctl );
                    }
                }
                if ( monocro_flag && !(event_mode & WAIT_INPUT_MODE) ) makeMonochromeSurface( text_surface, &p_button_link->image_rect );
                flush( &p_button_link->image_rect );
            }
            current_button_link.image_rect  = p_button_link->image_rect;
            current_button_link.sprite_no   = p_button_link->sprite_no;
            current_button_link.button_type = p_button_link->button_type;
            current_button_link.exbtn_ctl   = p_button_link->exbtn_ctl;
            shortcut_mouse_line = c;
        }
        else{
            if ( exbtn_d_button_link.exbtn_ctl ){
                drawExbtn( accumulation_surface, exbtn_d_button_link.exbtn_ctl );
            }
        }
    }
    current_over_button = button;
}

void ONScripterLabel::executeLabel()
{
  executeLabelTop:    
#if 0
    printf("*****  executeLabel %s:%d:%d *****\n",
           current_link_label_info->label_info.name,
           current_link_label_info->current_line,
           current_link_label_info->offset );
#endif

    int i, ret1, ret2;
    int line_cache = -1;
    bool first_read_flag = true;

    char *p_script_buffer = current_link_label_info->label_info.start_address;
    for ( i=0 ; i<current_link_label_info->current_line  ; i++ ) readLine( &p_script_buffer );

    //printf("from %d to %d\n", current_link_label_info->current_line, current_link_label_info->label_info.num_of_lines );

    i = current_link_label_info->current_line;
    while ( i<current_link_label_info->label_info.num_of_lines ){
        //printf("process line %d(%d/%d): \n",i, line_cache, current_link_label_info->label_info.num_of_lines );
        /* If comment */
        if ( line_cache != i ){
            line_cache = i;
            current_link_label_info->current_script = p_script_buffer;
            ret1 = readLine( &p_script_buffer );
            if ( first_read_flag ){
                first_read_flag = false;
                string_buffer_offset = current_link_label_info->offset;
            }
            if ( ret1 || string_buffer[0] == ';' ){
                i++;
                continue;
            }
        }

        current_link_label_info->current_line = i;
        current_link_label_info->offset = string_buffer_offset;

        if ( string_buffer[string_buffer_offset] == '~' ){
            last_tilde.label_info = current_link_label_info->label_info;
            last_tilde.current_line = current_link_label_info->current_line;
            last_tilde.offset = current_link_label_info->offset;
            if ( jumpf_flag ) jumpf_flag = false;
            skipToken();
            if ( string_buffer[string_buffer_offset] == '\0' ) i++;
            continue;
        }
        if ( jumpf_flag || break_flag && strncmp( string_buffer + string_buffer_offset, "next", 4 ) ){
            skipToken();
            if ( string_buffer[string_buffer_offset] == '\0' ) i++;
            continue;
        }

        ret1 = ScriptParser::parseLine();
        if ( ret1 == RET_COMMENT ){
            i++;
            continue;
        }
        else if ( ret1 == RET_JUMP ){
            goto executeLabelTop;
        }
        else if ( ret1 == RET_CONTINUE ){
            if ( string_buffer[ string_buffer_offset ] == '\0' ){
                current_link_label_info->current_script = p_script_buffer;
                string_buffer_offset = 0;
                i++;
            }
        }
        else if ( ret1 == RET_WAIT ){
            return;
        }
        else if ( ret1 == RET_WAIT_NEXT ){
            if ( string_buffer[ string_buffer_offset ] == '\0' ){
                current_link_label_info->current_script = p_script_buffer;
                current_link_label_info->current_line = i+1;
                current_link_label_info->offset = 0;
            }
            else
                current_link_label_info->offset = string_buffer_offset;
            return;
        }
        else if ( ret1 == RET_NOMATCH ){
            ret2 = this->parseLine();
            if ( ret2 == RET_JUMP ){
                goto executeLabelTop;
            }
            else if ( ret2 == RET_CONTINUE ){
                if ( string_buffer[ string_buffer_offset ] == '\0' ){
                    current_link_label_info->current_script = p_script_buffer;
                    string_buffer_offset = 0;
                    i++;
                }
            }
            else if ( ret2 == RET_WAIT ){
                return;
            }
            else if ( ret2 == RET_WAIT_NEXT ){
                if ( string_buffer[ string_buffer_offset ] == '\0' ){
                    current_link_label_info->current_script = p_script_buffer;
                    current_link_label_info->current_line = i+1;
                    current_link_label_info->offset = 0;
                }
                else
                    current_link_label_info->offset = string_buffer_offset;
                return;
            }
        }
    }

    current_link_label_info->label_info = lookupLabelNext( current_link_label_info->label_info.name );
    current_link_label_info->current_line = 0;
    current_link_label_info->offset = 0;

    if ( current_link_label_info->label_info.start_address != NULL ) goto executeLabelTop;
    else fprintf( stderr, " ***** End *****\n");
}

int ONScripterLabel::parseLine( )
{
    int ret, lut_counter = 0;
    unsigned int command_len;

    char *p_string_buffer = string_buffer + string_buffer_offset;
    //printf("parseline %d %s\n",string_buffer_offset,p_string_buffer );
    
    readToken( &p_string_buffer, tmp_string_buffer );
    command_len = strlen( tmp_string_buffer );
    
    while( func_lut[ lut_counter ].command[0] ){
        if ( !strcmp( func_lut[ lut_counter ].command, tmp_string_buffer ) ){
            ret = (this->*func_lut[ lut_counter ].method)();
            if ( ret == RET_CONTINUE || ret == RET_WAIT_NEXT ){
                skipToken();
            }
            return ret;
        }
        lut_counter++;
    }

    if ( !text_line_flag &&
         string_buffer[string_buffer_offset] != '@' && string_buffer[string_buffer_offset] != '\\' &&
         string_buffer[string_buffer_offset] != '/' && string_buffer[string_buffer_offset] != '!' &&
         string_buffer[string_buffer_offset] != '#' && string_buffer[string_buffer_offset] != '_' &&
         string_buffer[string_buffer_offset] != '%' && !(string_buffer[string_buffer_offset] & 0x80 ) ){
        printf(" command [%s] is not supported yet!!\n", &string_buffer[string_buffer_offset] );
        skipToken();
        return RET_CONTINUE;
    }

    /* Text */
    ret = textCommand( &string_buffer[string_buffer_offset] );
    if ( string_buffer[ string_buffer_offset ] == '\0' ){
        if ( !new_line_skip_flag && text_char_flag ){
            sentence_font.xy[0] = 0;
            sentence_font.xy[1]++;
            text_char_flag = false;
        }

        event_mode = IDLE_EVENT_MODE;
    }
    new_line_skip_flag = false;
    return ret;
}

SDL_Surface *ONScripterLabel::loadImage( char *file_name )
{
    unsigned long length;
    unsigned char *buffer;
    SDL_Surface *ret = NULL, *tmp;

    length = cBR->getFileLength( file_name );
    if ( length == 0 ){
        printf( " *** can't load file [%s] ***\n",file_name );
        return NULL;
    }
    //printf(" ... loading %s length %ld\n", file_name, length );
    buffer = new unsigned char[length];
    cBR->getFile( file_name, buffer );
    tmp = IMG_Load_RW(SDL_RWFromMem( buffer, length ),1);
    ret = SDL_ConvertSurface( tmp, text_surface->format, DEFAULT_SURFACE_FLAG );
    SDL_FreeSurface( tmp );
    delete[] buffer;

    return ret;
}

SDL_Surface *ONScripterLabel::loadTaggedImage( struct TaggedInfo *tag )
{
    SDL_Surface *ret = NULL;
    
    if ( tag->trans_mode == TRANS_STRING ){
        int top_xy[2], xy[2];
        
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];
        top_xy[0] = sentence_font.top_xy[0];
        top_xy[1] = sentence_font.top_xy[1];
        sentence_font.top_xy[0] = tag->pos.x;
        sentence_font.top_xy[1] = tag->pos.y;
        sentence_font.xy[0] = 0;
        sentence_font.xy[1] = 0;

        drawString( tag->file_name, tag->color_list[ tag->current_cell ], &sentence_font, true, NULL, &tag->pos );

        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];
        sentence_font.top_xy[0] = top_xy[0];
        sentence_font.top_xy[1] = top_xy[1];
    }
    else{
        ret = loadImage( tag->file_name );
    }
    return ret;
}

/* ---------------------------------------- */
/* Delete label link */
void ONScripterLabel::deleteLabelLink()
{
    LinkLabelInfo *info;
    
    current_link_label_info = root_link_label_info.next;
    while ( current_link_label_info ){
        info = current_link_label_info;
        current_link_label_info = current_link_label_info->next;
        delete info;
    }
    root_link_label_info.next = NULL;
    current_link_label_info = &root_link_label_info;
}

/* ---------------------------------------- */
/* Delete button link */
void ONScripterLabel::deleteButtonLink()
{
    struct ButtonLink *link;

    last_button_link = root_button_link.next;
    while( last_button_link ){
        link = last_button_link;
        last_button_link = last_button_link->next;
        if ( link->image_surface ) SDL_FreeSurface( link->image_surface );
        delete link;
    }
    root_button_link.next = NULL;
    last_button_link = &root_button_link;
}

void ONScripterLabel::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = 0;
    first_mouse_over_flag = true;
    SDL_GetMouseState( &mx, &my );
    mouseOverCheck( mx, my );
}

/* ---------------------------------------- */
/* Delete select link */
void ONScripterLabel::deleteSelectLink()
{
    struct SelectLink *link, *last_select_link = root_select_link.next;

    while ( last_select_link ){
        link = last_select_link;
        last_select_link = last_select_link->next;
        delete[] link->text;
        delete[] link->label;
        delete link;
    }
    root_select_link.next = NULL;
}

int ONScripterLabel::enterTextDisplayMode()
{
    if ( !(display_mode & TEXT_DISPLAY_MODE) ){
        //printf("enterTextDisplayMode2 %d %d\n",display_mode,event_mode);
        if ( event_mode & EFFECT_EVENT_MODE ){
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode |= TEXT_DISPLAY_MODE;
                text_on_flag = true;
                return RET_CONTINUE;
            }
            return RET_WAIT;
        }
        else{
            flush();
            SDL_BlitSurface( text_surface, NULL, effect_src_surface, NULL );
            shadowTextDisplay();
            restoreTextBuffer();

            SDL_BlitSurface( text_surface, NULL, effect_dst_surface, NULL );

            char *buf = new char[ strlen( string_buffer + string_buffer_offset ) + 1 ];
            memcpy( buf, string_buffer + string_buffer_offset, strlen( string_buffer + string_buffer_offset ) + 1 );
            int ret = setEffect( window_effect.effect, buf );
            if ( ret == RET_WAIT_NEXT ) return RET_WAIT;
            return ret;
        }
    }
    
    return RET_NOMATCH;
}

void ONScripterLabel::alphaBlend( SDL_Surface *dst_surface, int x, int y,
                                  SDL_Surface *src1_surface, int x1, int y1, int wx, int wy,
                                  SDL_Surface *src2_surface, int x2, int y2,
                                  int x3, int y3, int mask_value, unsigned int effect_value )
{
    int i, j;
    SDL_Rect src1_rect, src2_rect, dst_rect;
    Uint32 mask;
    Uint32 *src2_buffer;

    if ( x < 0 ){
        x1 -= x;
        x2 -= x;
        x3 -= x;
        wx += x;
        x = 0;
    }
    if ( x+wx > dst_surface->w ){
        wx -= x + wx - dst_surface->w;
    }
    if ( y < 0 ){
        y1 -= y;
        y2 -= y;
        y3 -= y;
        wy += y;
        y = 0;
    }
    if ( y+wy > dst_surface->h ){
        wy -= y + wy - dst_surface->h;
    }
        
    dst_rect.x = x;
    dst_rect.y = y;
    src1_rect.x = x1;
    src1_rect.y = y1;
    src2_rect.x = x2;
    src2_rect.y = y2;
    dst_rect.w = src1_rect.w = src2_rect.w = wx;
    dst_rect.h = src1_rect.h = src2_rect.h = wy;

    if ( src1_surface != dst_surface )
        SDL_BlitSurface( src1_surface, &src1_rect, dst_surface, &dst_rect );

    SDL_LockSurface( src2_surface );
    src2_buffer  = (Uint32 *)src2_surface->pixels;

    if ( mask_value < 0 ){
        if ( mask_value == -TRANS_ALPHA ){
            for ( i=0; i<wy ; i++ ) {
                for ( j=0 ; j<wx ; j++ ){
                    mask = (*(src2_buffer + src2_surface->w * (y3+i) + x3 + j) >> src2_surface->format->Rshift) & 0xff;
                    mask = (0xff - mask) << src2_surface->format->Ashift;
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) |= mask;
                }
            }
        }
        else if ( mask_value == -TRANS_TOPLEFT || mask_value == -TRANS_TOPRIGHT ){
            *src2_buffer &= ~amask;
            Uint32 ref;
            if ( mask_value == -TRANS_TOPLEFT ) ref = *src2_buffer;
            else                                ref = *(src2_buffer + src2_surface->w - 1);
            mask = (Uint32)0xff << src2_surface->format->Ashift;
            for ( i=0; i<wy ; i++ ) {
                for ( j=0 ; j<wx ; j++ ){
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                    if ( *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) != ref )
                        *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) |= mask;
                }
            }
        }
        else if ( mask_value == -TRANS_FADE_MASK ){
            SDL_LockSurface( effect_mask_surface );
            Uint32 *mask_buffer = (Uint32 *)effect_mask_surface->pixels;
            for ( i=0; i<wy ; i++ ) {
                int y4 = effect_mask_surface->w * ((y2+i) % effect_mask_surface->h );
                for ( j=0 ; j<wx ; j++ ){
                    mask = (*(mask_buffer + y4 + (x2 + j) % effect_mask_surface->w ) >> effect_mask_surface->format->Rshift) & 0xff;
                    if ( effect_value > mask )
                        mask = amask;
                    else
                        mask = 0;
                        
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) |= mask;
                }
            }
            SDL_UnlockSurface( effect_mask_surface );
        }
        else if ( mask_value == -TRANS_CROSSFADE_MASK ){
            SDL_LockSurface( effect_mask_surface );
            Uint32 *mask_buffer = (Uint32 *)effect_mask_surface->pixels;
            for ( i=0; i<wy ; i++ ) {
                int y4 = effect_mask_surface->w * ((y2+i) % effect_mask_surface->h );
                for ( j=0 ; j<wx ; j++ ){
                    mask = (*(mask_buffer + y4 + (x2 + j) % effect_mask_surface->w ) >> effect_mask_surface->format->Rshift) & 0xff;
                    if ( effect_value > mask ){
                        mask = effect_value - mask;
                        if ( mask > 0xff )
                            mask = amask;
                        else
                            mask = mask << src2_surface->format->Ashift;
                    }
                    else
                        mask = 0;
                        
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) |= mask;
                }
            }
            SDL_UnlockSurface( effect_mask_surface );
        }
    }
    else{
        mask = (0xff - (unsigned int)mask_value) << src2_surface->format->Ashift;
        for ( i=0; i<wy ; i++ ) {
            for ( j=0 ; j<wx ; j++ ){
                *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) |= mask;
            }
        }
    }
    
    SDL_UnlockSurface( src2_surface );
    SDL_SetAlpha( src2_surface, SDL_SRCALPHA | DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    SDL_BlitSurface( src2_surface, &src2_rect, dst_surface, &dst_rect );
    SDL_SetAlpha( src2_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
}

void ONScripterLabel::parseTaggedString( char *buffer, struct TaggedInfo *tag )
{
    //printf(" parseTaggeString %s\n", buffer);
    tag->remove();
    
    int i;
    tag->num_of_cells = 1;
    tag->trans_mode = trans_mode;

    if ( buffer[0] == ':' ){
        buffer++;
        if ( buffer[0] == 'a' ){ // alpha blend
            tag->trans_mode = TRANS_ALPHA;
            buffer++;
        }
        else if ( buffer[0] == 'l' ){ // top left
            tag->trans_mode = TRANS_TOPLEFT;
            buffer++;
        }
        else if ( buffer[0] == 'r' ){ // top right
            tag->trans_mode = TRANS_TOPRIGHT;
            buffer++;
        }
        else if ( buffer[0] == 'c' ){ // copy
            tag->trans_mode = TRANS_COPY;
            buffer++;
        }
        else if ( buffer[0] == 's' ){ // string
            tag->trans_mode = TRANS_STRING;
            buffer++;
            i=0;
            tag->num_of_cells = 0;
            while( buffer[i] == '#' ){
                tag->num_of_cells++;
                i += 7;
            }
            tag->color_list = new uchar3[ tag->num_of_cells ];
            for ( i=0 ; i<tag->num_of_cells ; i++ ){
                readColor( &tag->color_list[i], buffer + 1 );
                buffer += 7;
            }
        }
        else if ( buffer[0] == 'm' ){ // mask
            tag->trans_mode = TRANS_MASK;
            buffer++;
            readStr( &buffer, tmp_string_buffer );
            setStr( &tag->mask_file_name, tmp_string_buffer );
        }
        else if ( buffer[0] == '#' ){ // direct color
            tag->trans_mode = TRANS_DIRECT;
            memcpy( tag->direct_color, buffer, 7 );
            tag->direct_color[7] = '\0';
            buffer += 7;
        }
        else if ( buffer[0] == '!' ){ // pallet number
            tag->trans_mode = TRANS_PALLET;
            buffer++;
            tag->pallet_number = 0;
            while ( *buffer >= '0' && *buffer <= '9' )
                tag->pallet_number = tag->pallet_number * 10 + *buffer++ - '0';
        }
    }
    //printf("trans_mode = %d\n", tag->trans_mode );

    if ( buffer[0] == '/' ){
        buffer++;
        tag->num_of_cells = 0;
        while ( *buffer >= '0' && *buffer <= '9' )
            tag->num_of_cells = tag->num_of_cells * 10 + *buffer++ - '0';
        buffer++;
        //printf("number of cells = %d\n", tag->num_of_cells );
        assert( tag->num_of_cells != 0 );

        tag->duration_list = new int[ tag->num_of_cells ];

        if ( *buffer == '<' ){
            buffer++;
            for ( i=0 ; i<tag->num_of_cells ; i++ ){
                tag->duration_list[i] = 0;
                while ( *buffer >= '0' && *buffer <= '9' )
                    tag->duration_list[i] = tag->duration_list[i] * 10 + *buffer++ - '0';
                buffer++;
            }
            buffer++; // skip '>'
        }
        else{
            tag->duration_list[0] = 0;
            while ( *buffer >= '0' && *buffer <= '9' )
                tag->duration_list[0] = tag->duration_list[0] * 10 + *buffer++ - '0';
            for ( i=1 ; i<tag->num_of_cells ; i++ )
                tag->duration_list[i] = tag->duration_list[0];
            buffer++;
        }
        
        tag->loop_mode = *buffer++ - '0'; // 3...no animation
    }

    if ( buffer[0] == ';' ) buffer++;

    if ( tag->trans_mode == TRANS_STRING && buffer[0] == '$' ){
        buffer++;
        setStr( &tag->file_name, str_variables[ readInt( &buffer ) ] );
    }
    else{
        setStr( &tag->file_name, buffer );
    }
}

void ONScripterLabel::clearCurrentTextBuffer()
{
    int i, j;
    current_text_buffer->xy[0] = sentence_font.xy[0] = 0;
    current_text_buffer->xy[1] = sentence_font.xy[1] = 0;
    text_char_flag = false;

    if ( current_text_buffer->buffer &&
         (current_text_buffer->num_xy[0] != sentence_font.num_xy[0] ||
          current_text_buffer->num_xy[1] != sentence_font.num_xy[1] ) ){
        delete[] current_text_buffer->buffer;
        current_text_buffer->buffer = NULL;
    }
    if ( !current_text_buffer->buffer ){
        current_text_buffer->buffer = new char[ sentence_font.num_xy[0] * sentence_font.num_xy[1] * 2];
        current_text_buffer->num_xy[0] = sentence_font.num_xy[0];
        current_text_buffer->num_xy[1] = sentence_font.num_xy[1];
    }

    for ( i=0 ; i<current_text_buffer->num_xy[1] ; i++ ){
        for ( j=0 ; j<current_text_buffer->num_xy[0] ; j++ ){
            current_text_buffer->buffer[ (i * current_text_buffer->num_xy[0] + j) * 2 ] = ((char*)"@")[0];
            current_text_buffer->buffer[ (i * current_text_buffer->num_xy[0] + j) * 2 + 1 ] = ((char*)"@")[1];
        }
    }
}

void ONScripterLabel::shadowTextDisplay( SDL_Surface *dst_surface, SDL_Surface *src_surface )
{
    if ( !dst_surface ) dst_surface = text_surface;
    if ( !src_surface ) src_surface = accumulation_surface;
    
    if ( sentence_font.display_transparency ){
        SDL_BlitSurface( src_surface, NULL, dst_surface, NULL );
        makeMonochromeSurface( dst_surface, &sentence_font_info.pos, false );
    }
    else{
        if ( sentence_font_info.image_surface ){
            /* drawTaggedSurface must be used intead !! */
            int tmp_x3 = 0;
            if ( sentence_font_info.tag.trans_mode == TRANS_ALPHA ){
                tmp_x3 = sentence_font_info.pos.w;
            }
            alphaBlend( dst_surface, sentence_font_info.pos.x, sentence_font_info.pos.y,
                        src_surface, sentence_font_info.pos.x, sentence_font_info.pos.y,
                        sentence_font_info.pos.w, sentence_font_info.pos.h,
                        sentence_font_info.image_surface, 0, 0,
                        tmp_x3, 0, -sentence_font_info.tag.trans_mode );
        }
    }
}

void ONScripterLabel::newPage( bool next_flag )
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( next_flag ){
        if ( current_text_buffer->xy[0] != 0 || current_text_buffer->xy[1] != 0 )
            current_text_buffer = current_text_buffer->next;
    }

    clearCurrentTextBuffer();
    if ( display_mode & TEXT_DISPLAY_MODE )
        shadowTextDisplay();
    else
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
    flush();
}

int ONScripterLabel::playMP3( int cd_no )
{
    if ( !audio_open_flag ) return -1;

    if ( mp3_file_name == NULL ){
        char file_name[128];
        
        sprintf( file_name, "cd%ctrack%2.2d.mp3", DELIMITER, cd_no );
        printf("playMP3 %s", file_name );
        mp3_sample = SMPEG_new( file_name, &mp3_info, 0 );
    }
    else{
        unsigned long length;
    
        length = cBR->getFileLength( mp3_file_name );
        printf(" ... loading %s length %ld\n",mp3_file_name, length );
        printf("playMP3 %s", mp3_file_name );
        mp3_buffer = new unsigned char[length];
        cBR->getFile( mp3_file_name, mp3_buffer );
        mp3_sample = SMPEG_new_rwops( SDL_RWFromMem( mp3_buffer, length ), &mp3_info, 0 );
    }

    if ( SMPEG_error( mp3_sample ) ){
        printf(" failed. [%s]\n",SMPEG_error( mp3_sample ));
        // The line below fails. ?????
        //SMPEG_delete( mp3_sample );
        mp3_sample = NULL;
    }
    else{
        SMPEG_enableaudio( mp3_sample, 0 );
        SMPEG_actualSpec( mp3_sample, &audio_format );

        printf(" at vol %d once %d\n", mp3_volume, mp3_play_once_flag );
        Mix_HookMusic( mp3callback, mp3_sample );
        SMPEG_enableaudio( mp3_sample, 1 );
        //SMPEG_loop( mp3_sample, mp3_play_once_flag?0:1 );
        SMPEG_play( mp3_sample );

        SMPEG_setvolume( mp3_sample, mp3_volume );
    }

    return 0;
}

int ONScripterLabel::playCDAudio( int cd_no )
{
    int length = cdrom_info->track[cd_no - 1].length / 75;

    printf("playCDAudio %d\n", cd_no );
    SDL_CDPlayTracks( cdrom_info, cd_no - 1, 0, 1, 0 );

    timer_cdaudio_id = SDL_AddTimer( length * 1000, cdaudioCallback, NULL );

    return 0;
}

int ONScripterLabel::playWave( char *file_name, bool loop_flag, int channel )
{
    unsigned long length;
    unsigned char *buffer;

    if ( !audio_open_flag ) return -1;
    
    if ( channel >= MIX_CHANNELS ) channel = MIX_CHANNELS - 1;

    length = cBR->getFileLength( file_name );
    buffer = new unsigned char[length];
    cBR->getFile( file_name, buffer );
    wave_sample[channel] = Mix_LoadWAV_RW(SDL_RWFromMem( buffer, length ), 1);
    delete[] buffer;

    if ( channel == 0 ) Mix_Volume( channel, voice_volume * 128 / 100 );
    else                Mix_Volume( channel, se_volume * 128 / 100 );

    printf("playWave %s at vol %d\n", file_name, (channel==0)?voice_volume:se_volume );
    
    Mix_PlayChannel( channel, wave_sample[channel], loop_flag?-1:0 );

    return 0;
}

void ONScripterLabel::stopBGM( bool continue_flag )
{
    if ( cdaudio_flag && cdrom_info ){
        extern SDL_TimerID timer_cdaudio_id;

        if ( timer_cdaudio_id ){
            SDL_RemoveTimer( timer_cdaudio_id );
            timer_cdaudio_id = NULL;
        }

        if (SDL_CDStatus( cdrom_info ) >= CD_PLAYING )
            SDL_CDStop( cdrom_info );
    }

    if ( mp3_sample ){
        SMPEG_stop( mp3_sample );
        SMPEG_delete( mp3_sample );
        Mix_HookMusic( NULL, NULL );
        mp3_sample = NULL;

        if ( mp3_buffer ){
            delete[] mp3_buffer;
            mp3_buffer = NULL;
        }
        if ( !continue_flag ) setStr( &mp3_file_name, NULL );
    }

    if ( !continue_flag ) current_cd_track = -1;
}

struct ONScripterLabel::ButtonLink *ONScripterLabel::getSelectableSentence( char *buffer, struct FontInfo *info, bool flush_flag, bool nofile_flag )
{
    int current_text_xy[2];
    
    ButtonLink *button_link = new ButtonLink();
    button_link->button_type = NORMAL_BUTTON;

    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];
    
    /* ---------------------------------------- */
    /* Draw selected characters */
    drawString( buffer, info->on_color, info, false, text_surface, &button_link->image_rect );

    button_link->select_rect = button_link->image_rect;

    button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, button_link->image_rect.w, button_link->image_rect.h, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( button_link->image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    SDL_BlitSurface( text_surface, &button_link->image_rect, button_link->image_surface, NULL );
    
    /* ---------------------------------------- */
    /* Draw shadowed characters */
    info->xy[0] = current_text_xy[0];
    info->xy[1] = current_text_xy[1];
    if ( nofile_flag )
        drawString( buffer, info->nofile_color, info, flush_flag, text_surface, NULL );
    else
        drawString( buffer, info->off_color, info, flush_flag, text_surface, NULL );
        
    info->xy[0] = current_text_xy[0];
    info->xy[1]++;

    return button_link;
}

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, SDL_Rect *pos, SDL_Rect *clip,
                                         SDL_Surface *src_surface, TaggedInfo *tag )
{
    SDL_Rect dst_rect = *pos, src_rect;
    int offset, aoffset = 0, w, h, src_y;

    //printf("drawTagged %d %d\n",x,y);
    if ( tag->trans_mode == TRANS_STRING ){
        int top_xy[2], xy[2];
        
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];
        top_xy[0] = sentence_font.top_xy[0];
        top_xy[1] = sentence_font.top_xy[1];
        sentence_font.top_xy[0] = tag->pos.x;
        sentence_font.top_xy[1] = tag->pos.y;
        sentence_font.xy[0] = 0;
        sentence_font.xy[1] = 0;

        drawString( tag->file_name, tag->color_list[ tag->current_cell ], &sentence_font, true, dst_surface, NULL );
        
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];
        sentence_font.top_xy[0] = top_xy[0];
        sentence_font.top_xy[1] = top_xy[1];
        return;
    }
    else if ( !src_surface ) return;

    w = src_surface->w / tag->num_of_cells;
    h = src_surface->h;
    src_y = 0;
    offset = w * tag->current_cell;

    if ( tag->trans_mode == TRANS_ALPHA ||
              tag->trans_mode == TRANS_TOPLEFT ||
              tag->trans_mode == TRANS_TOPRIGHT ){
        if ( tag->trans_mode == TRANS_ALPHA ){
            w /= 2;
            aoffset = offset + w;
        }
        /* Ugly workaround. Clipping must be processed in another part !! */
        if ( pos->x >= clip->x + clip->w ||
             pos->x + w <= clip->x ||
             pos->y >= clip->y + clip->h ||
             pos->y + h <= clip->y ) return;
        if ( dst_rect.x < clip->x ){
            w -= clip->x - pos->x;
            aoffset += clip->x - pos->x;
            offset += clip->x - pos->x;
            dst_rect.x = clip->x;
        }
        if ( clip->x + clip->w < dst_rect.x + w ){
            w -= dst_rect.x + w - clip->x - clip->w;
        }
        if ( dst_rect.y < clip->y ){
            h -= clip->y - pos->y;
            src_y = clip->y - pos->y;
            dst_rect.y = clip->y;
        }
        if ( clip->y + clip->h < dst_rect.y + h ){
            h -= dst_rect.y + h - clip->y - clip->h;
        }
        alphaBlend( dst_surface, dst_rect.x, dst_rect.y,
                    dst_surface, dst_rect.x, dst_rect.y, w, h,
                    src_surface, offset, src_y,
                    aoffset, src_y, -tag->trans_mode );
    }
    else if ( tag->trans_mode == TRANS_COPY ){
        src_rect.x = offset;
        src_rect.y = 0;
        src_rect.w = w;
        src_rect.h = src_surface->h;
        dst_rect.x = pos->x;
        dst_rect.y = pos->y;
        SDL_BlitSurface( src_surface, &src_rect, dst_surface, &dst_rect );
    }
}

void ONScripterLabel::makeMonochromeSurface( SDL_Surface *surface, SDL_Rect *dst_rect, bool one_color_flag )
{
    int i, j;
    SDL_Rect rect;
    Uint32 *buf, c;
    
    if ( dst_rect ){
        rect.x = dst_rect->x;
        rect.y = dst_rect->y;
        rect.w = dst_rect->w;
        rect.h = dst_rect->h;
    }
    else{
        rect.x = rect.y = 0;
        rect.w = surface->w;
        rect.h = surface->h;
    }

    SDL_LockSurface( surface );
    buf = (Uint32 *)surface->pixels + rect.y * surface->w + rect.x;
    
    for ( i=rect.y ; i<rect.y + rect.h ; i++ ){
        for ( j=rect.x ; j<rect.x + rect.w ; j++, buf++ ){
            if ( one_color_flag ){
                c = (((*buf >> surface->format->Rshift) & 0xff) * 77 +
                     ((*buf >> surface->format->Gshift) & 0xff) * 151 +
                     ((*buf >> surface->format->Bshift) & 0xff) * 28 ) >> 8; 
                *buf = monocro_color_lut[c][0] << surface->format->Rshift |
                    monocro_color_lut[c][1] << surface->format->Gshift |
                    monocro_color_lut[c][2] << surface->format->Bshift;
            }
            else{
                c = ((((*buf >> surface->format->Rshift) & 0xff) * sentence_font.window_color[0] >> 8) << surface->format->Rshift |
                     (((*buf >> surface->format->Gshift) & 0xff) * sentence_font.window_color[1] >> 8) << surface->format->Gshift |
                     (((*buf >> surface->format->Bshift) & 0xff) * sentence_font.window_color[2] >> 8) << surface->format->Bshift );
                *buf = c;
            }
        }
        buf += surface->w - rect.w;
    }
    SDL_UnlockSurface( surface );
}

void ONScripterLabel::refreshAccumulationSurface( SDL_Surface *surface, SDL_Rect *rect )
{
    int i;
    SDL_Rect clip;

    if ( !rect ){
        clip.x = 0;
        clip.y = 0;
        clip.w = surface->w;
        clip.h = surface->h;
    }
    else clip = *rect;
        
    SDL_BlitSurface( background_surface, rect, surface, rect );
    
    for ( i=MAX_SPRITE_NUM-1 ; i>z_order ; i-- ){
        if ( sprite_info[i].valid ){
            drawTaggedSurface( surface, &sprite_info[i].pos, &clip,
                               sprite_info[i].image_surface, &sprite_info[i].tag );
        }
    }
    for ( i=0 ; i<3 ; i++ ){
        if ( tachi_info[i].image_surface ){
            drawTaggedSurface( surface, &tachi_info[i].pos, &clip,
                               tachi_info[i].image_surface, &tachi_info[i].tag );
        }
    }
    for ( i=z_order ; i>=0 ; i-- ){
        if ( sprite_info[i].valid ){
            drawTaggedSurface( surface, &sprite_info[i].pos, &clip,
                               sprite_info[i].image_surface, &sprite_info[i].tag );
        }
    }

    if ( monocro_flag ) makeMonochromeSurface( surface, &clip );
}

int ONScripterLabel::refreshSprite( SDL_Surface *surface, int sprite_no, bool active_flag, int cell_no, bool draw_flag )
{
    if ( sprite_no == -1 ){
        sprite_no = cell_no;
        cell_no = -1;
    }
    //printf("refreshSprite no. %d: active %d: cell %d (%d,%d,%d,%d)\n", sprite_no, active_flag, cell_no,
    //sprite_info[ sprite_no ].pos.x, sprite_info[ sprite_no ].pos.y, sprite_info[ sprite_no ].pos.w, sprite_info[ sprite_no ].pos.h );
    sprite_info[ sprite_no ].valid = active_flag;
    if ( cell_no >= 0 ) sprite_info[ sprite_no ].tag.current_cell = cell_no;

    if ( draw_flag ){
        refreshAccumulationSurface( surface, &sprite_info[ sprite_no ].pos );
        SDL_BlitSurface( surface, &sprite_info[ sprite_no ].pos, text_surface, &sprite_info[ sprite_no ].pos );
        flush( &sprite_info[ sprite_no ].pos );
    }
    return sprite_info[ sprite_no ].pos.w * sprite_info[ sprite_no ].pos.h;
}

int ONScripterLabel::decodeExbtnControl( SDL_Surface *surface, char *ctl_str, bool draw_flag )
{
    int num, sprite_no, area = 0;
    bool active_flag;
    bool first_flag = true;
    
    while( *ctl_str ){
        if ( *ctl_str == 'C' ){
            if ( !first_flag ) area += refreshSprite( surface, sprite_no, active_flag, num, draw_flag );
            active_flag = false;
            num = 0;
            sprite_no = -1;
        }
        else if ( *ctl_str == 'P' ){
            if ( !first_flag ) area += refreshSprite( surface, sprite_no, active_flag, num, draw_flag );
            active_flag = true;
            num = 0;
            sprite_no = -1;
        }
        else if ( *ctl_str == ',' ){
            sprite_no = num;
            num = 0;
        }
        else{
            num = num * 10 + *ctl_str - '0';
        }
        first_flag = false;
        ctl_str++;
    }
    if ( !first_flag ) area += refreshSprite( surface, sprite_no, active_flag, num, draw_flag );

    return area;
}

void ONScripterLabel::drawExbtn( SDL_Surface *surface, char *ctl_str )
{
    int area = decodeExbtnControl( surface, ctl_str, false );

    if ( area > screen_width * screen_height ){
        refreshAccumulationSurface( surface, NULL );
        SDL_BlitSurface( surface, NULL, text_surface, NULL );
        flush();
    }
    else{
        decodeExbtnControl( surface, ctl_str, true );
    }
}

void ONScripterLabel::setupAnimationInfo( struct AnimationInfo *anim )
{
    anim->deleteImageSurface();
    anim->image_surface = loadTaggedImage( &anim->tag );
    anim->abs_flag = true;

    if ( anim->tag.trans_mode == TRANS_STRING ){
        anim->pos = anim->tag.pos;
    }
    else if ( anim->image_surface ){
        anim->pos.w = anim->image_surface->w / anim->tag.num_of_cells;
        anim->pos.h = anim->image_surface->h;
        if ( anim->tag.trans_mode == TRANS_ALPHA ) anim->pos.w /= 2;
        if ( anim->preserve_surface ) SDL_FreeSurface( anim->preserve_surface );
        anim->preserve_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, anim->pos.w, anim->pos.h, 32, rmask, gmask, bmask, amask );
        SDL_SetAlpha( anim->preserve_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    }
}

void ONScripterLabel::loadCursor( int no, char *str, int x, int y, bool abs_flag )
{
    cursor_info[ no ].setImageName( str );
    cursor_info[ no ].pos.x = x;
    cursor_info[ no ].pos.y = y;

    parseTaggedString( cursor_info[ no ].image_name, &cursor_info[ no ].tag );
    setupAnimationInfo( &cursor_info[ no ] );
    cursor_info[ no ].abs_flag = abs_flag;
}

void ONScripterLabel::startCursor( int click )
{
    SDL_Rect src_rect;
    int no;
    
    if ( click == CLICK_WAIT ) no = CURSOR_WAIT_NO;
    else if ( click == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
    else return;

    if ( cursor_info[ no ].valid ) {
        src_rect.x = cursor_info[ no ].pos.x;
        src_rect.y = cursor_info[ no ].pos.y;
    }
    else{
        src_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].pos.x;
        src_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].pos.y;
    }
    if ( cursor_info[ no ].image_surface ){
        src_rect.w = cursor_info[ no ].image_surface->w / cursor_info[ no ].tag.num_of_cells;
        src_rect.h = cursor_info[ no ].image_surface->h;
        SDL_BlitSurface( text_surface, &src_rect, cursor_info[ no ].preserve_surface, NULL );
    }
    event_mode |= WAIT_ANIMATION_MODE;
}

void ONScripterLabel::endCursor( int click )
{
    SDL_Rect dst_rect;
    int no;

    if ( autoclick_timer > 0 ) return;
    
    if ( click == CLICK_WAIT ) no = CURSOR_WAIT_NO;
    else if ( click == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
    else return;
    
    if ( cursor_info[ no ].valid ) {
        dst_rect.x = cursor_info[ no ].pos.x;
        dst_rect.y = cursor_info[ no ].pos.y;
    }
    else{
        dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].pos.x;
        dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].pos.y;
    }
    if ( cursor_info[ no ].preserve_surface ){
        SDL_BlitSurface( cursor_info[ no ].preserve_surface, NULL, text_surface, &dst_rect );
        flush( dst_rect.x, dst_rect.y, cursor_info[ no ].preserve_surface->w, cursor_info[ no ].preserve_surface->h );
    }
    event_mode &= ~WAIT_ANIMATION_MODE;
}

