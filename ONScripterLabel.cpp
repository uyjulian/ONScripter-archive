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
extern unsigned short convSJIS2UTF16( unsigned short in );

static SDL_TimerID timer_id = NULL;

#define FONT_SIZE 26

#define EFFECT_STRIPE_WIDTH 16
#define EFFECT_STRIPE_CURTAIN_WIDTH 24

#define DEFAULT_DECODEBUF 16384
#define DEFAULT_AUDIOBUF  4096

#define ONS_TIMER_EVENT (SDL_USEREVENT)
#define ONS_SOUND_EVENT (SDL_USEREVENT+1)

#define DEFAULT_TEXT_SPEED1 40 // Low speed
#define DEFAULT_TEXT_SPEED2 20 // Middle speed
#define DEFAULT_TEXT_SPEED3 10 // High speed

#define CURSOR_WAIT_NO    0
#define CURSOR_NEWPAGE_NO 1

#define DEFAULT_CURSOR_WAIT    ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

typedef int (ONScripterLabel::*FuncList)();
static struct FuncLUT{
    char command[40];
    FuncList method;
} func_lut[100] = {
    {"wavestop",   &ONScripterLabel::wavestopCommand},
    {"waveloop",   &ONScripterLabel::waveCommand},
    {"wave",   &ONScripterLabel::waveCommand},
    {"waittimer",   &ONScripterLabel::waittimerCommand},
    {"wait",   &ONScripterLabel::waitCommand},
    {"vsp",   &ONScripterLabel::vspCommand},
    {"voicevol",   &ONScripterLabel::voicevolCommand},
    {"trap",   &ONScripterLabel::trapCommand},
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
    {"game", &ONScripterLabel::gameCommand},
    {"exbtn_d", &ONScripterLabel::exbtnCommand},
    {"exbtn", &ONScripterLabel::exbtnCommand},
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

    SDL_FillRect( screen_surface, NULL, SDL_MapRGBA( screen_surface->format, 0, 0, 0, 0 ) );
	SDL_UpdateRect(screen_surface, 0, 0, 0, 0);

    initSJIS2UTF16();
    
	return(0);
}

void mp3callback( void *userdata, Uint8 *stream, int len )
{
    if ( SMPEG_playAudio( (SMPEG*)userdata, stream, len ) == 0 ){
        SDL_Event event;
        event.type = ONS_SOUND_EVENT;
        SDL_PushEvent(&event);
    }
}

Uint32 timerCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_id );
    timer_id = NULL;

	SDL_Event event;
	event.type = ONS_TIMER_EVENT;
	SDL_PushEvent( &event );

    return interval;
}

void ONScripterLabel::startTimer( Uint32 count )
{
    if ( timer_id != NULL ){
        SDL_RemoveTimer( timer_id );
    }
    timer_id = SDL_AddTimer( count, timerCallback, NULL );
}

ONScripterLabel::ONScripterLabel()
{
    int i;

    if ( open() ) exit(-1);

    printf("ONScripterLabel::ONScripterLabel\n");
    
    SetVideoMode();
    SDL_WM_SetCaption( "ONScripter", "ONScripter" );

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
#if 0
    SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
    SDL_BlitSurface( accumulation_surface, NULL, select_surface, NULL );
    SDL_BlitSurface( select_surface, NULL, text_surface, NULL );
#endif
    flush();

    internal_timer = SDL_GetTicks();
    autoclick_timer = 0;

    monocro_flag = false;
    trap_flag = false;
    trap_dist = NULL;
    
    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    skip_flag = false;
    draw_one_page_flag = false;
    //draw_one_page_flag = true;
    key_pressed_flag = false;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    
    start_delayed_effect_info = NULL;
    
    root_button_link.next = NULL;
    last_button_link = &root_button_link;
    root_select_link.next = NULL;
    last_select_link = &root_select_link;
    btndef_surface = NULL;
    current_over_button = 0;

    /* ---------------------------------------- */
    /* Tachi related variables */
    for ( i=0 ; i<3 ; i++ ){
        tachi_info[i].pos.x = 0;
        tachi_info[i].pos.w = 0;
    }

    /* ---------------------------------------- */
    /* Sprite related variables */
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        sprite_info[i].valid = false;
        sprite_info[i].tag.file_name = NULL;
    }
    
    /* ---------------------------------------- */
    /* Cursor related variables */
    for ( i=0 ; i<2 ; i++ ){
        if ( i==CURSOR_WAIT_NO ) loadCursor( CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0 );
        else                     loadCursor( CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0 );
    }
    
    /* ---------------------------------------- */
    /* Sound related variables */
    mp3_sample = NULL;
    mp3_file_name = NULL;
    mp3_buffer = NULL;
    current_cd_track = -1;
    for ( i=0 ; i<MIX_CHANNELS ; i++ ) wave_sample[i] = NULL;
    
    /* ---------------------------------------- */
    /* Initialize font */
    text_char_flag = false;
    default_text_speed[0] = DEFAULT_TEXT_SPEED1;
    default_text_speed[1] = DEFAULT_TEXT_SPEED2;
    default_text_speed[2] = DEFAULT_TEXT_SPEED3;
    text_speed_no = 1;
    
    new_line_skip_flag = false;
    sentence_font.ttf_font = (void*)TTF_OpenFont( FONT_NAME, FONT_SIZE );
    if ( !sentence_font.ttf_font ){
        fprintf( stderr, "can't open %s\n", FONT_NAME );
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
    sentence_font.window_image = NULL;
    sentence_font.window_rect[0] = 0;
    sentence_font.window_rect[1] = 0;
    sentence_font.window_rect[2] = 639;
    sentence_font.window_rect[3] = 479;

    sentence_font.on_color[0] = sentence_font.on_color[1] = sentence_font.on_color[2] = 0xff;
    sentence_font.off_color[0] = sentence_font.off_color[1] = sentence_font.off_color[2] = 0x80;
    
    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;

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

int ONScripterLabel::eventLoop()
{
	SDL_Event event;

    startTimer( MINIMUM_TIMER_RESOLUTION );

	while ( SDL_WaitEvent(&event) ) {
		switch (event.type) {
          case SDL_MOUSEMOTION:
            mouseMoveEvent( (SDL_MouseMotionEvent*)&event );
            break;
            
          case SDL_MOUSEBUTTONDOWN:
            mousePressEvent( (SDL_MouseButtonEvent*)&event );
            break;

          case SDL_KEYDOWN:
            keyPressEvent( (SDL_KeyboardEvent*)&event );
            break;

          case ONS_TIMER_EVENT:
            timerEvent();
            break;
                
          case ONS_SOUND_EVENT:
            playstopCommand();
            if ( !mp3_play_once_flag ) playMP3( current_cd_track );
            break;
                
          case SDL_QUIT:
            saveGlovalData();
            saveFileLog();
            saveLabelLog();
            return(0);
            
          default:
            break;
		}
	}
    return -1;
}

void ONScripterLabel::keyPressEvent( SDL_KeyboardEvent *event )
{
    int i;
    
    if ( skip_flag && event->keysym.sym == SDLK_s) skip_flag = false;

    if ( trap_flag && (event->keysym.sym == SDLK_RETURN ||
                       event->keysym.sym == SDLK_SPACE ) ){
        printf("trap by key\n");
        trap_flag = false;
        current_link_label_info->label_info = lookupLabel( trap_dist );
        current_link_label_info->current_line = 0;
        current_link_label_info->offset = 0;
        endCursor( clickstr_state );
        event_mode = IDLE_EVENT_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
        return;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE ){
        if ( event->keysym.sym == SDLK_UP || event->keysym.sym == SDLK_p ){
            if ( --shortcut_mouse_line < 0 ) shortcut_mouse_line = 0;
            struct ButtonLink *p_button_link = root_button_link.next;
            for ( i=0 ; i<shortcut_mouse_line && p_button_link ; i++ ) p_button_link  = p_button_link->next;
            if ( p_button_link ) SDL_WarpMouse( p_button_link->select_rect.x + p_button_link->select_rect.w / 2, p_button_link->select_rect.y + p_button_link->select_rect.h / 2 );
        }
        else if ( event->keysym.sym == SDLK_DOWN || event->keysym.sym == SDLK_n ){
            shortcut_mouse_line++;
            struct ButtonLink *p_button_link = root_button_link.next;
            for ( i=0 ; i<shortcut_mouse_line && p_button_link ; i++ ) p_button_link  = p_button_link->next;
            if ( !p_button_link ){
                shortcut_mouse_line = i-1;
                p_button_link = root_button_link.next;
                for ( i=0 ; i<shortcut_mouse_line ; i++ ) p_button_link  = p_button_link->next;
            }
            if ( p_button_link ) SDL_WarpMouse( p_button_link->select_rect.x + p_button_link->select_rect.w / 2, p_button_link->select_rect.y + p_button_link->select_rect.h / 2 );
        }
        else if ( event->keysym.sym == SDLK_RETURN ||
                  event->keysym.sym == SDLK_SPACE ||
                  ( event->keysym.sym == SDLK_ESCAPE && rmode_flag ) ){
            if ( shortcut_mouse_line >= 0 ){
                if ( event->keysym.sym == SDLK_ESCAPE  ){
                    current_button_state.button = -1;
                    volatile_button_state.button = -1;
                }
                else{
                    current_button_state.button = current_over_button;
                    volatile_button_state.button = current_over_button;
                }
                if ( event_mode & WAIT_BUTTON_MODE || event_mode & WAIT_MOUSE_MODE ){
                    startTimer( MINIMUM_TIMER_RESOLUTION );
                }
            }
        }
    }

    if ( event_mode & WAIT_MOUSE_MODE ){
        if ( event->keysym.sym == SDLK_ESCAPE && rmode_flag ){
            current_button_state.button = -1;
            volatile_button_state.button = -1;
            system_menu_mode = SYSTEM_MENU;
            endCursor( clickstr_state );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
    }
    
    if ( event_mode & WAIT_KEY_MODE && !key_pressed_flag ){
        if (event->keysym.sym == SDLK_RETURN || event->keysym.sym == SDLK_SPACE ){
            skip_flag = false;
            key_pressed_flag = true;
            endCursor( clickstr_state );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
        else if (event->keysym.sym == SDLK_s){
            skip_flag = !skip_flag;
            printf("toggle skip to %s\n", (skip_flag?"true":"false") );
            key_pressed_flag = true;
            if ( skip_flag ){
                endCursor( clickstr_state );
                startTimer( MINIMUM_TIMER_RESOLUTION );
            }
        }
        else if (event->keysym.sym == SDLK_o){
            draw_one_page_flag = !draw_one_page_flag;
            printf("toggle draw one page flag to %s\n", (draw_one_page_flag?"true":"false") );
            if ( draw_one_page_flag ){
                endCursor( clickstr_state );
                startTimer( MINIMUM_TIMER_RESOLUTION );
            }
        }
        else if ( event->keysym.sym == SDLK_1 ){
            text_speed_no = 0;
            sentence_font.wait_time = default_text_speed[ text_speed_no ];
        }
        else if ( event->keysym.sym == SDLK_2 ){
            text_speed_no = 1;
            sentence_font.wait_time = default_text_speed[ text_speed_no ];
        }
        else if ( event->keysym.sym == SDLK_3 ){
            text_speed_no = 2;
            sentence_font.wait_time = default_text_speed[ text_speed_no ];
        }
    }
}

void ONScripterLabel::mousePressEvent( SDL_MouseButtonEvent *event )
{
    current_button_state.x = event->x;
    current_button_state.y = event->y;
    int button = event->button;
    
    if ( button == SDL_BUTTON_RIGHT && rmode_flag ){
        current_button_state.button = -1;
        volatile_button_state.button = -1;
        last_mouse_state.button = -1;
    }
    else if ( button == SDL_BUTTON_LEFT ){
        current_button_state.button = current_over_button;
        volatile_button_state.button = current_over_button;
        last_mouse_state.button = current_over_button;
        if ( trap_flag ){
            printf("trap by left mouse\n");
            trap_flag = false;
            current_link_label_info->label_info = lookupLabel( trap_dist );
            current_link_label_info->current_line = 0;
            current_link_label_info->offset = 0;
            if ( !(event_mode & WAIT_BUTTON_MODE) ) endCursor( clickstr_state );
            event_mode = IDLE_EVENT_MODE;
            startTimer( MINIMUM_TIMER_RESOLUTION );
            return;
        }
    }
    else return;
    
#if 0
    printf("mousePressEvent %d %d %d %d\n", event_mode,
           current_button_state.x,
           current_button_state.y,
           current_button_state.button );
#endif
    if ( skip_flag ) skip_flag = false;
    
    if ( event_mode & WAIT_MOUSE_MODE && volatile_button_state.button == -1){
        system_menu_mode = SYSTEM_MENU;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE || event_mode & WAIT_MOUSE_MODE ){
        if ( !(event_mode & WAIT_BUTTON_MODE) ) endCursor( clickstr_state );
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }
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

void ONScripterLabel::timerEvent( void )
{
  timerEventTop:
    
    int ret;
    unsigned int i;

    if ( event_mode & WAIT_CURSOR_MODE ){
        int no;
    
        if ( clickstr_state == CLICK_WAIT )         no = CURSOR_WAIT_NO;
        else if ( clickstr_state == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
        
        if ( cursor_info[ no ].tag.duration_list ){
            if ( cursor_info[ no ].image_surface ){
                SDL_Rect src_rect, dst_rect;
                src_rect.x = cursor_info[ no ].image_surface->w * cursor_info[ no ].tag.current_cell / cursor_info[ no ].tag.num_of_cells;
                src_rect.y = 0;
                src_rect.w = cursor_info[ no ].pos.w;
                src_rect.h = cursor_info[ no ].pos.h;
                if ( cursor_info[ no ].valid ) {
                    dst_rect.x = cursor_info[ no ].pos.x;
                    dst_rect.y = cursor_info[ no ].pos.y;
                }
                else{
                    dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].pos.x;
                    dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].pos.y;
                }
                alphaBlend( text_surface, dst_rect.x, dst_rect.y,
                            cursor_info[ no ].preserve_surface, 0, 0, cursor_info[ no ].preserve_surface->w, cursor_info[ no ].preserve_surface->h,
                            cursor_info[ no ].image_surface, src_rect.x, src_rect.y,
                            0, 0, -cursor_info[ no ].tag.trans_mode );
                //SDL_BlitSurface( cursor_info[ no ].image_surface, &src_rect, text_surface, &dst_rect );
                flush( dst_rect.x, dst_rect.y, src_rect.w, src_rect.h );
            }

            proceedAnimation( &cursor_info[ no ] );

            //printf("timer %d %d\n",cursor_info[ no ].count,cursor_info[ no ].tag.duration_list[ cursor_info[ no ].count ]);
            startTimer( cursor_info[ no ].tag.duration_list[ cursor_info[ no ].tag.current_cell ] );
        }
    }
    else if ( event_mode & EFFECT_EVENT_MODE ){
        if ( display_mode & TEXT_DISPLAY_MODE ){
            if ( effect_counter == 0 ){
                flush();
                SDL_BlitSurface( accumulation_surface, NULL, effect_dst_surface, NULL );
                SDL_BlitSurface( text_surface, NULL, effect_src_surface, NULL );
            }
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode &= ~TEXT_DISPLAY_MODE;
                effect_counter = 0;
            }
            startTimer( MINIMUM_TIMER_RESOLUTION );
            return;
        }
        while(1){
            string_buffer_offset = 0;
            int c=0;
            for ( i=0 ; i<strlen(start_delayed_effect_info->command)+1 ; i++ )
                addStringBuffer( start_delayed_effect_info->command[i], c++ );
            ret = this->parseLine();

            if ( ret == RET_CONTINUE ){
                effect_counter = 0;
                struct DelayedInfo *p_effect_info = start_delayed_effect_info;
                start_delayed_effect_info = start_delayed_effect_info->next;
                delete[] p_effect_info->command;
                delete p_effect_info;
            
                if ( !start_delayed_effect_info ){
                    event_mode = IDLE_EVENT_MODE;
                    //printf("stopped\n");
                    if ( effect_blank == 0 ) goto timerEventTop;
                    startTimer( effect_blank );
                    return;
                }
            }
            else{
                startTimer( MINIMUM_TIMER_RESOLUTION );
                return;
            }
        }
    }
    else{
        if ( system_menu_mode != SYSTEM_NULL || (event_mode & WAIT_MOUSE_MODE && volatile_button_state.button == -1)  )
            executeSystemCall();
        else
            executeLabel();
    }
    volatile_button_state.button = 0;
}

void ONScripterLabel::mouseOverCheck( int x, int y )
{
    int c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;
    
    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    struct ButtonLink *p_button_link = root_button_link.next;
    while( p_button_link ){
        if ( x > p_button_link->select_rect.x && x < p_button_link->select_rect.x + p_button_link->select_rect.w &&
             y > p_button_link->select_rect.y && y < p_button_link->select_rect.y + p_button_link->select_rect.h ){
            button = p_button_link->no;
            break;
        }
        p_button_link = p_button_link->next;
        c++;
    }
    if ( current_over_button != button ){
        if ( event_mode & WAIT_BUTTON_MODE && !first_mouse_over_flag ){
            if ( current_over_button_link.button_type == NORMAL_BUTTON ){
                SDL_BlitSurface( select_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );
            }
            else if ( current_over_button_link.button_type == SPRITE_BUTTON || current_over_button_link.button_type == EX_SPRITE_BUTTON ){
                sprite_info[ current_over_button_link.sprite_no ].tag.current_cell = 0;
                refreshAccumulationSurface( accumulation_surface, &current_over_button_link.image_rect );
                SDL_BlitSurface( accumulation_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );
            }
            flush( &current_over_button_link.image_rect );
        }
        first_mouse_over_flag = false;

        if ( p_button_link ){
            if ( event_mode & WAIT_BUTTON_MODE ){
                if ( p_button_link->button_type == NORMAL_BUTTON && p_button_link->image_surface ){
                    SDL_BlitSurface( p_button_link->image_surface, NULL, text_surface, &p_button_link->image_rect );
                }
                else if ( p_button_link->button_type == SPRITE_BUTTON || p_button_link->button_type == EX_SPRITE_BUTTON ){
                    sprite_info[ p_button_link->sprite_no ].tag.current_cell = 1;
                    refreshAccumulationSurface( accumulation_surface, &p_button_link->image_rect );
                    SDL_BlitSurface( accumulation_surface, &p_button_link->image_rect, text_surface, &p_button_link->image_rect );
                    if ( p_button_link->button_type == EX_SPRITE_BUTTON ){
                        drawExbtn( accumulation_surface, p_button_link->exbtn_ctl );
                    }
                }
                if ( monocro_flag && !(event_mode & WAIT_MOUSE_MODE) ) makeMonochromeSurface( text_surface, &p_button_link->image_rect );
                flush( &p_button_link->image_rect );
            }
            current_over_button_link.image_rect = p_button_link->image_rect;
            current_over_button_link.sprite_no = p_button_link->sprite_no;
            current_over_button_link.button_type = p_button_link->button_type;
            current_over_button_link.exbtn_ctl = p_button_link->exbtn_ctl;
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

void ONScripterLabel::mouseMoveEvent( SDL_MouseMotionEvent *event )
{
    mouseOverCheck( event->x, event->y );
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
            //startTimer( MINIMUM_TIMER_RESOLUTION );
            //return;
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

    //if ( current_link_label_info->label_info.start_address != NULL ) startTimer( MINIMUM_TIMER_RESOLUTION );
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

    if ( string_buffer[string_buffer_offset] != '@' && string_buffer[string_buffer_offset] != '\\' &&
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

SDL_Surface *ONScripterLabel::loadPixmap( struct TaggedInfo *tag )
{
    unsigned long length;
    unsigned char *buffer;
    SDL_Surface *ret = NULL, *tmp;
    
    if ( tag->trans_mode != TRANS_STRING ){
        length = cBR->getFileLength( tag->file_name );
        if ( length == 0 ){
            printf( " *** can't load file [%s] ***\n",tag->file_name );
            return NULL;
        }
        //printf(" ... loading %s length %ld\n", tag->file_name, length );
        buffer = new unsigned char[length];
        cBR->getFile( tag->file_name, buffer );
        tmp = IMG_Load_RW(SDL_RWFromMem( buffer, length ),1);
        ret = SDL_ConvertSurface( tmp, text_surface->format, DEFAULT_SURFACE_FLAG );
        SDL_FreeSurface( tmp );
        delete[] buffer;
    }
    return ret;
}

int ONScripterLabel::setEffect( int immediate_flag, char *buf )
{
    //printf("setEffect %d\n",immediate_flag);
    DelayedInfo *p_effect = new DelayedInfo();
    p_effect->next = NULL;
    p_effect->command = buf;

    if ( !start_delayed_effect_info ) start_delayed_effect_info = p_effect;
    else{
        DelayedInfo *p_delayed_effect_info = start_delayed_effect_info;
        while( p_delayed_effect_info->next ) p_delayed_effect_info = p_delayed_effect_info->next;
        p_delayed_effect_info->next = p_effect;
    }

    if ( immediate_flag == RET_WAIT || immediate_flag == RET_WAIT_NEXT ){
        effect_counter = 0;
        event_mode = EFFECT_EVENT_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }

    return immediate_flag;
}

int ONScripterLabel::doEffect( int effect_no, struct TaggedInfo *tag, int effect_image )
{
    effect_start_time = SDL_GetTicks();
    if ( effect_counter == 0 ) effect_start_time_old = effect_start_time - 1;
    //printf("effect_counter %d timer between %d %d\n",effect_counter,effect_start_time,effect_start_time_old);
    effect_timer_resolution = effect_start_time - effect_start_time_old;
    effect_start_time_old = effect_start_time;
    
    //printf("doEffect %d %d\n", effect_no, effect_counter );
    int i;
    int width, width2;
    int height, height2;
    SDL_Rect src_rect, dst_rect;

    if ( effect_counter == 0 ){
        SDL_Surface *tmp_surface = NULL;

        if ( effect_image != DIRECT_EFFECT_IMAGE )
            SDL_BlitSurface( accumulation_surface, NULL, effect_src_surface, NULL );

        switch( effect_image ){
          case DIRECT_EFFECT_IMAGE:
            break;
            
          case COLOR_EFFECT_IMAGE:
            SDL_FillRect( background_surface, NULL, SDL_MapRGB( effect_dst_surface->format, tag->color[0], tag->color[1], tag->color[2]) );
            refreshAccumulationSurface( effect_dst_surface );
            break;

          case BG_EFFECT_IMAGE:
            tmp_surface = loadPixmap( tag );
            if ( tmp_surface ){
                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.w = tmp_surface->w;
                src_rect.h = tmp_surface->h;
                dst_rect.x = (screen_width - tmp_surface->w) / 2;
                dst_rect.y = (screen_height - tmp_surface->h) / 2;

                SDL_BlitSurface( tmp_surface, &src_rect, background_surface, &dst_rect );
            }
            refreshAccumulationSurface( effect_dst_surface );
            break;
            
          case TACHI_EFFECT_IMAGE:
            refreshAccumulationSurface( effect_dst_surface );
            break;
        }
        if ( tmp_surface ) SDL_FreeSurface( tmp_surface );
    }

    
    /* ---------------------------------------- */
    /* Execute effect */
    struct EffectLink effect = getEffect( effect_no );
    //printf("Effect number %d\n", effect.effect );
    switch ( effect.effect ){
      case 0: // Instant display
      case 1: // Instant display
        SDL_BlitSurface( effect_dst_surface, NULL, text_surface, NULL );
        break;

      case 2: // Left shutter
        width = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=0 ; i<screen_width/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = screen_height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 3: // Right shutter
        width = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=screen_width/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH - width - 1;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = screen_height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 4: // Top shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=0 ; i<screen_height/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH;
            src_rect.w = dst_rect.w = screen_width;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 5: // Bottom shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=screen_height/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH - height - 1;
            src_rect.w = dst_rect.w = screen_width;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 6: // Left curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_width/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_width;
            if ( width2 >= 0 ){
                src_rect.x = dst_rect.x = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = screen_height;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 7: // Right curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_width/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_width;
            if ( width2 >= 0 ){
                if ( width2 > EFFECT_STRIPE_CURTAIN_WIDTH ) width2 = EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.x = dst_rect.x = screen_width - i * EFFECT_STRIPE_CURTAIN_WIDTH - width2;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = screen_height;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 8: // Top curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_height/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_height;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.w = dst_rect.w = screen_width;
                src_rect.h = dst_rect.h = height2;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 9: // Bottom curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_height/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_height;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = screen_height - i * EFFECT_STRIPE_CURTAIN_WIDTH - height2;
                src_rect.w = dst_rect.w = screen_width;
                src_rect.h = dst_rect.h = height2;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      default:
        printf("effect.effect %d is not implemented. Crossfade is substituted for that.\n",effect.effect);
        
      case 10: // Cross fade
        height = 255 - 256 * effect_counter / effect.duration;
        alphaBlend( text_surface, 0, 0,
                    effect_src_surface, 0, 0, screen_width, screen_height,
                    effect_dst_surface, 0, 0,
                    0, 0, height );
        break;
        
      case 11: // Left scroll
        width = screen_width * effect_counter / effect.duration;
        src_rect.x = 0;
        dst_rect.x = width;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width - width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = screen_width - width - 1;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 12: // Right scroll
        width = screen_width * effect_counter / effect.duration;
        src_rect.x = width;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width - width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = 0;
        dst_rect.x = screen_width - width - 1;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 13: // Top scroll
        width = screen_height * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = width;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = screen_height - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = screen_height - width - 1;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = width;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 14: // Bottom scroll
        width = screen_height * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = width;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = screen_height - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = screen_height - width - 1;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = width;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

#if 0
      default:
        bitBlt( text_surface, 0, 0, effect_dst_surface );
#endif        
    }

    //printf("%d / %d\n", effect_counter, effect.duration);
        
    effect_counter += effect_timer_resolution;
    if ( effect_counter < effect.duration ){
        if ( effect.effect != 0 ) flush();
        return RET_WAIT;
    }
    else{
        //monocro_flag = false;
        if ( effect_image == COLOR_EFFECT_IMAGE || effect_image == BG_EFFECT_IMAGE ){
            SDL_BlitSurface( effect_dst_surface, NULL, accumulation_surface, NULL );
            SDL_BlitSurface( accumulation_surface, NULL, select_surface, NULL );
            SDL_BlitSurface( select_surface, NULL, text_surface, NULL );
        }
        else if ( effect_image == DIRECT_EFFECT_IMAGE ){
            SDL_BlitSurface( effect_dst_surface, NULL, select_surface, NULL );
            SDL_BlitSurface( select_surface, NULL, text_surface, NULL );
        }
        else{
            SDL_BlitSurface( effect_dst_surface, NULL, accumulation_surface, NULL );
            SDL_BlitSurface( accumulation_surface, NULL, select_surface, NULL );
            SDL_BlitSurface( select_surface, NULL, text_surface, NULL );
        }
        if ( effect.effect != 0 ) flush();
        return RET_CONTINUE;
    }
}

void ONScripterLabel::drawChar( char* text, struct FontInfo *info, bool flush_flag, SDL_Surface *surface )
{
    int xy[2];
    SDL_Rect rect;
    SDL_Surface *tmp_surface = NULL;
    SDL_Color color;
    unsigned short index, unicode;
    int minx, maxx, miny, maxy, advanced;

    //printf("draw %x-%x[%s]\n", text[0], text[1], text);

    if ( !surface ) surface = text_surface;
    
    if ( !info->font_valid_flag && info->ttf_font ){
        TTF_CloseFont( (TTF_Font*)info->ttf_font );
        info->ttf_font = NULL;
    }
    if ( info->ttf_font == NULL ){
        info->ttf_font = TTF_OpenFont(FONT_NAME, info->font_size );
        if ( !info->ttf_font ){
            fprintf( stderr, "can't open font file %s\n", FONT_NAME );
            SDL_Quit();
            exit(-1);
        }
        info->font_valid_flag = true;
    }

    if ( text[0] & 0x80 ){
        index = ((unsigned char*)text)[0];
        index = index << 8 | ((unsigned char*)text)[1];
        unicode = convSJIS2UTF16( index );
    }
    else{
        unicode = text[0];
    }

    TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, unicode,
                      &minx, &maxx, &miny, &maxy, &advanced );
    //printf("minx %d maxx %d miny %d maxy %d ace %d\n",minx, maxx, miny, maxy, advanced );
    
    if ( info->xy[0] >= info->num_xy[0] ){
        info->xy[0] = 0;
        info->xy[1]++;
    }
    xy[0] = info->xy[0] * info->pitch_xy[0] + info->top_xy[0];
    xy[1] = info->xy[1] * info->pitch_xy[1] + info->top_xy[1];
    
    rect.x = xy[0] + 1 + minx;
    rect.y = xy[1] + TTF_FontAscent( (TTF_Font*)info->ttf_font ) - maxy;
    
    if ( info->display_shadow ){
        color.r = color.g = color.b = 0;
        tmp_surface = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );
        rect.w = tmp_surface->w;
        rect.h = tmp_surface->h;
        SDL_BlitSurface( tmp_surface, NULL, surface, &rect );
        SDL_FreeSurface( tmp_surface );
    }

    color.r = info->color[0];
    color.g = info->color[1];
    color.b = info->color[2];

    tmp_surface = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );
    rect.x--;
    rect.w = tmp_surface->w;
    rect.h = tmp_surface->h;
    SDL_BlitSurface( tmp_surface, NULL, surface, &rect );
    if ( tmp_surface ) SDL_FreeSurface( tmp_surface );
    
    if ( flush_flag ) flush( rect.x, rect.y, rect.w + 1, rect.h );

    /* ---------------------------------------- */
    /* Update text buffer */
    if ( !system_menu_enter_flag && surface == text_surface ){
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 ] = text[0];
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 + 1 ] = text[1];
    }
    info->xy[0] ++;
    if ( !system_menu_enter_flag && surface == text_surface ){
        current_text_buffer->xy[0] = info->xy[0];
        current_text_buffer->xy[1] = info->xy[1];
    }
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
    struct ButtonLink *tmp_button_link;
    last_button_link = root_button_link.next;
    while( last_button_link ){
        tmp_button_link = last_button_link;
        last_button_link = last_button_link->next;
        if ( tmp_button_link->image_surface ) SDL_FreeSurface( tmp_button_link->image_surface );
        delete tmp_button_link;
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
    struct SelectLink *tmp_select_link;
    last_select_link = root_select_link.next;
    while ( last_select_link ){
        tmp_select_link = last_select_link;
        last_select_link = last_select_link->next;
        delete[] tmp_select_link->text;
        delete[] tmp_select_link->label;
        delete tmp_select_link;
    }
    root_select_link.next = NULL;
    last_select_link = &root_select_link;
}

void ONScripterLabel::restoreTextBuffer()
{
    int i, end;
    int xy[2];

    char out_text[3] = { '\0','\0','\0' };
    xy[0] = sentence_font.xy[0];
    xy[1] = sentence_font.xy[1];
    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;
    end = current_text_buffer->xy[1] * current_text_buffer->num_xy[0] + current_text_buffer->xy[0];
    for ( i=0 ; i<current_text_buffer->num_xy[1] * current_text_buffer->num_xy[0] ; i++ ){
        if ( sentence_font.xy[1] * current_text_buffer->num_xy[0] + sentence_font.xy[0] >= end ) break;
        out_text[0] = current_text_buffer->buffer[ i * 2 ];
        out_text[1] = current_text_buffer->buffer[ i * 2 + 1];
        drawChar( out_text, &sentence_font, false );
    }
    sentence_font.xy[0] = xy[0];
    sentence_font.xy[1] = xy[1];
    if ( xy[0] == 0 ) text_char_flag = false;
    else              text_char_flag = true;
}

int ONScripterLabel::enterTextDisplayMode()
{
    if ( !(display_mode & TEXT_DISPLAY_MODE) ){
        //printf("enterTextDisplayMode %d\n",event_mode);
        if ( event_mode & EFFECT_EVENT_MODE ){
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode |= TEXT_DISPLAY_MODE;
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
            setEffect( RET_WAIT, buf );
            return RET_WAIT;
        }
    }
    
    return RET_NOMATCH;
}

void ONScripterLabel::alphaBlend( SDL_Surface *dst_surface, int x, int y,
                                  SDL_Surface *src1_surface, int x1, int y1, int wx, int wy,
                                  SDL_Surface *src2_surface, int x2, int y2,
                                  int x3, int y3, int mask_value )
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
                    mask = *(src2_buffer + src2_surface->w * (y3+i) + x3 + j) >> src2_surface->format->Rshift;
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
    
    int i, tmp;
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
        
        tag->duration_list[0] = 0;
        while ( *buffer >= '0' && *buffer <= '9' )
            tag->duration_list[0] = tag->duration_list[0] * 10 + *buffer++ - '0';
        buffer++;

        tmp = 0;
        while ( *buffer >= '0' && *buffer <= '9' )
            tmp = tmp * 10 + *buffer++ - '0';
        //buffer++;
        
        if ( buffer[0] == ',' ){
            tag->duration_list[1] = tmp;
            for ( i=2 ; i<tag->num_of_cells ; i++ ){
                tag->duration_list[i] = 0;
                while ( *buffer >= '0' && *buffer <= '9' )
                    tag->duration_list[i] = tag->duration_list[i] * 10 + *buffer++ - '0';
                buffer++;
            }

            tmp = 0;
            while ( *buffer >= '0' && *buffer <= '9' )
                tmp = tmp * 10 + *buffer++ - '0';
        }
        else{
            for ( i=1 ; i<tag->num_of_cells ; i++ ) tag->duration_list[i] = tag->duration_list[0];
        }
            
        tag->loop_mode = tmp; // 3...no animation

        //printf("loop_mode = %d\n", tag->loop_mode );
    }

    if ( buffer[0] == ';' ) buffer++;

    tag->file_name = new char[ strlen( buffer ) + 1 ];
    memcpy( tag->file_name, buffer, strlen( buffer ) + 1 );
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

void ONScripterLabel::shadowTextDisplay()
{
    if ( sentence_font.display_transparency ){
        SDL_Rect rect;
        SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        rect.x = sentence_font.window_rect[0];
        rect.y = sentence_font.window_rect[1];
        rect.w = sentence_font.window_rect[2] - sentence_font.window_rect[0] + 1;
        rect.h = sentence_font.window_rect[3] - sentence_font.window_rect[1] + 1;
        makeMonochromeSurface( text_surface, &rect, false );
    }
    else{
        SDL_Surface *tmp_surface = loadPixmap( &sentence_font_tag );
        if ( tmp_surface ){
            int tmp_w, tmp_x3;
            if ( sentence_font_tag.trans_mode == TRANS_ALPHA )
                tmp_x3 = tmp_w = tmp_surface->w / 2;
            else
                tmp_x3 = 0, tmp_w  = tmp_surface->w;
            alphaBlend( text_surface, sentence_font.window_rect[0], sentence_font.window_rect[1],
                        accumulation_surface, sentence_font.window_rect[0], sentence_font.window_rect[1], tmp_w, tmp_surface->h,
                        tmp_surface, 0, 0,
                        tmp_x3, 0, -sentence_font_tag.trans_mode );
            SDL_FreeSurface( tmp_surface );
        }
    }
}

void ONScripterLabel::enterNewPage()
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( current_text_buffer->xy[0] != 0 || current_text_buffer->xy[1] != 0 )
        current_text_buffer = current_text_buffer->next;
    clearCurrentTextBuffer();

    /* ---------------------------------------- */
    /* Clear the screen */
    shadowTextDisplay();
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

struct ONScripterLabel::ButtonLink *ONScripterLabel::getSelectableSentence( char *buffer, struct FontInfo *info, bool flush_flag, bool nofile_flag )
{
    int i;
    uchar3 color;
    struct ButtonLink *button_link;
    int current_text_xy[2];
    char *p_text, text[3] = { '\0', '\0', '\0' };
    
    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];
    
    /* ---------------------------------------- */
    /* Draw selected characters */
    for ( i=0 ; i<3 ; i++ ) color[i] = info->color[i];
    for ( i=0 ; i<3 ; i++ ) info->color[i] = info->on_color[i];
    p_text = buffer;
    while( *p_text ){
        if ( *p_text & 0x80 ){
            text[0] = *p_text++;
            text[1] = *p_text++;
        }
        else{
            text[0] = *p_text++;
            text[1] = '\0';
        }
        drawChar( text, info, false );
    }

    /* ---------------------------------------- */
    /* Create ButtonLink from the rendered text */
    button_link = new ButtonLink();
    button_link->button_type = NORMAL_BUTTON;
    
    if ( current_text_xy[1] == info->xy[1] ){
        button_link->image_rect.x = info->top_xy[0] + current_text_xy[0] * info->pitch_xy[0];
        button_link->image_rect.w = info->pitch_xy[0] * (info->xy[0] - current_text_xy[0] + 1);
    }
    else{
        button_link->image_rect.x = info->top_xy[0];
        button_link->image_rect.w = info->pitch_xy[0] * info->num_xy[0];
    }
    button_link->image_rect.y = current_text_xy[1] * info->pitch_xy[1] + info->top_xy[1];// - info->pitch_xy[1] + 3;
    button_link->image_rect.h = (info->xy[1] - current_text_xy[1] + 1) * info->pitch_xy[1];
    button_link->select_rect = button_link->image_rect;

    button_link->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, button_link->image_rect.w, button_link->image_rect.h, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( button_link->image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    SDL_BlitSurface( text_surface, &button_link->image_rect, button_link->image_surface, NULL );
    
    /* ---------------------------------------- */
    /* Draw shadowed characters */
    if ( nofile_flag )
        for ( i=0 ; i<3 ; i++ ) info->color[i] = info->nofile_color[i];
    else
        for ( i=0 ; i<3 ; i++ ) info->color[i] = info->off_color[i];
    info->xy[0] = current_text_xy[0];
    info->xy[1] = current_text_xy[1];
    p_text = buffer;
    while( *p_text ){
        if ( *p_text & 0x80 ){
            text[0] = *p_text++;
            text[1] = *p_text++;
        }
        else{
            text[0] = *p_text++;
            text[1] = '\0';
        }
        drawChar( text, info, flush_flag );
    }
        
    info->xy[0] = current_text_xy[0];
    info->xy[1]++;

    for ( i=0 ; i<3 ; i++ ) info->color[i] = color[i];

    return button_link;
}

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, SDL_Rect *pos, SDL_Rect *clip,
                                         SDL_Surface *src_surface, TaggedInfo *tagged_info )
{
    SDL_Rect dst_rect = *pos, src_rect;
    int offset, aoffset = 0, i, w, h, src_y;

    //printf("drawTagged %d %d\n",x,y);
    if ( tagged_info->trans_mode == TRANS_STRING ){
        char *p_text, text[3] = { '\0', '\0', '\0' };
        uchar3 shelter_color;
        int top_xy[2], xy[2];
        
        for ( i=0 ; i<3 ; i++ ) shelter_color[i] = sentence_font.color[i];
        for ( i=0 ; i<3 ; i++ ) sentence_font.color[i] = tagged_info->color_list[0][i];
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];
        top_xy[0] = sentence_font.top_xy[0];
        top_xy[1] = sentence_font.top_xy[1];
        sentence_font.top_xy[0] = pos->x;
        sentence_font.top_xy[1] = pos->y;
        sentence_font.xy[0] = 0;
        sentence_font.xy[1] = 0;
        p_text = tagged_info->file_name;
        while( *p_text ){
            if ( *p_text & 0x80 ){
                text[0] = *p_text++;
                text[1] = *p_text++;
            }
            else{
                text[0] = *p_text++;
                text[1] = '\0';
            }
            drawChar( text, &sentence_font, false, dst_surface );
        }
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];
        sentence_font.top_xy[0] = top_xy[0];
        sentence_font.top_xy[1] = top_xy[1];
        for ( i=0 ; i<3 ; i++ ) sentence_font.color[i] = shelter_color[i];
        return;
    }
    else if ( !src_surface ) return;

    w = src_surface->w / tagged_info->num_of_cells;
    h = src_surface->h;
    src_y = 0;
    offset = w * tagged_info->current_cell;

    if ( tagged_info->trans_mode == TRANS_ALPHA ||
              tagged_info->trans_mode == TRANS_TOPLEFT ||
              tagged_info->trans_mode == TRANS_TOPRIGHT ){
        if ( tagged_info->trans_mode == TRANS_ALPHA ){
            w /= 2;
            aoffset = offset + w;
        }
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
                    aoffset, src_y, -tagged_info->trans_mode );
    }
    else if ( tagged_info->trans_mode == TRANS_COPY ){
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
    int i, w;
    SDL_Rect pos, clip;

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
        if ( tachi_info[i].image_name ){
            w = tachi_info[i].image_surface->w;
            if ( tachi_info[i].tag.trans_mode == TRANS_ALPHA ) w /= 2;
            pos.x = screen_width * (i+1) / 4 - w / 2;
            pos.y = underline_value - tachi_info[i].image_surface->h + 1;
            drawTaggedSurface( surface, &pos, &clip,
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

int ONScripterLabel::clickWait( char *out_text )
{
    if ( skip_flag || draw_one_page_flag ){
        clickstr_state = CLICK_NONE;
        if ( out_text ){
            drawChar( out_text, &sentence_font, false );
            string_buffer_offset += 2;
        }
        else{
            flush();
            string_buffer_offset++;
        }
        return RET_CONTINUE;
    }
    else{
        clickstr_state = CLICK_WAIT;
        if ( out_text ) drawChar( out_text, &sentence_font );
        event_mode = WAIT_MOUSE_MODE | WAIT_KEY_MODE;
        key_pressed_flag = false;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else if ( cursor_info[ CURSOR_WAIT_NO ].tag.num_of_cells > 0 ){
            startCursor( CLICK_WAIT );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
        return RET_WAIT;
    }
}

int ONScripterLabel::clickNewPage( char *out_text )
{
    clickstr_state = CLICK_NEWPAGE;
    if ( out_text ) drawChar( out_text, &sentence_font, false );
    if ( skip_flag || draw_one_page_flag ) flush();
    
    if ( skip_flag ){
        event_mode = WAIT_SLEEP_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }
    else{
        event_mode = WAIT_MOUSE_MODE | WAIT_KEY_MODE;
        key_pressed_flag = false;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else if ( cursor_info[ CURSOR_NEWPAGE_NO ].tag.num_of_cells > 0 ){
            startCursor( CLICK_NEWPAGE );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
    }
    return RET_WAIT;
}

void ONScripterLabel::makeEffectStr( char **buf, char *dst_buf )
{
    int num = readEffect( buf, &tmp_effect );

    sprintf( dst_buf + strlen(dst_buf), "%d", tmp_effect.effect );
    if ( num >= 2 ){
        sprintf( dst_buf + strlen(dst_buf), ",%d", tmp_effect.duration );
        if ( num >= 3 ){
            sprintf( dst_buf + strlen(dst_buf), ",%s", tmp_effect.image );
        }
    }
}

int ONScripterLabel::readEffect( char **buf, struct EffectLink *effect )
{
    int num = 1;
    
    effect->effect = readInt( buf );
    if ( end_with_comma_flag ){
        num++;
        effect->duration = readInt( buf );
        if ( end_with_comma_flag ){
            num++;
            readStr( buf, tmp_string_buffer );
            if ( effect->image ) delete[] effect->image;
            effect->image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( effect->image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        }
    }

    //printf("readEffect %d: %d %d %s\n", num, effect->effect, effect->duration, effect->image );
    return num;
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
    anim->image_surface = loadPixmap( &anim->tag );

    if ( anim->image_surface ){
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
    //printf("load Cursor %s\n",str);
    cursor_info[ no ].setImageName( str );
    cursor_info[ no ].valid = abs_flag;
    cursor_info[ no ].pos.x = x;
    cursor_info[ no ].pos.y = y;

    parseTaggedString( cursor_info[ no ].image_name, &cursor_info[ no ].tag );
    setupAnimationInfo( &cursor_info[ no ] );
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
    event_mode |= WAIT_CURSOR_MODE;
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
    event_mode &= ~WAIT_CURSOR_MODE;
}

/* ---------------------------------------- */
/* Commands */

int ONScripterLabel::textCommand( char *text )
{
    int i, j, t, ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    char out_text[20], num_buf[10];
    char *p_string_buffer;

    if ( event_mode & (WAIT_MOUSE_MODE | WAIT_KEY_MODE | WAIT_SLEEP_MODE) ){
        if ( clickstr_state == CLICK_WAIT ){
            event_mode = IDLE_EVENT_MODE;
            if ( string_buffer[ string_buffer_offset ] != '@' ) current_link_label_info->offset = ++string_buffer_offset;
            current_link_label_info->offset = ++string_buffer_offset;
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE;
        }
        else if ( clickstr_state == CLICK_NEWPAGE ){
            event_mode = IDLE_EVENT_MODE;
            if ( string_buffer[ string_buffer_offset ] != '\\' ) current_link_label_info->offset = ++string_buffer_offset;
            current_link_label_info->offset = ++string_buffer_offset;
            enterNewPage();
            new_line_skip_flag = true;
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE;
        }
        else if ( string_buffer[ string_buffer_offset ] & 0x80 ){
            string_buffer_offset += 2;
        }
        else if ( string_buffer[ string_buffer_offset ] == '!' ){
            string_buffer_offset++;
            if ( string_buffer[ string_buffer_offset ] == 'w' || string_buffer[ string_buffer_offset ] == 'd' ){
                string_buffer_offset++;
                p_string_buffer = &string_buffer[ string_buffer_offset ];
                readInt( &p_string_buffer );
                string_buffer_offset = p_string_buffer - string_buffer;
            }
        }
        else{
            string_buffer_offset++;
        }

        event_mode = IDLE_EVENT_MODE;
        current_link_label_info->offset = string_buffer_offset;
    }

    if ( string_buffer[ string_buffer_offset ] == '\0' ) return RET_CONTINUE;
    new_line_skip_flag = false;
    
    //printf("*** textCommand %d %d(%d) %s\n", string_buffer_offset, sentence_font.xy[0], sentence_font.pitch_xy[0], string_buffer + string_buffer_offset );
    
    char ch = string_buffer[ string_buffer_offset ];
    if ( ch & 0x80 ){ // Shift jis
        text_char_flag = true;
        /* ---------------------------------------- */
        /* Kinsoku process */
        if ( sentence_font.xy[0] + 1 == sentence_font.num_xy[0] &&
             string_buffer[ string_buffer_offset + 2 ] & 0x80 &&
             (( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x41 ) ||
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x42 ) ||
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x48 ) ||
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x49 ) ||
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x76 ) ||
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x5b )) ){
            sentence_font.xy[0] = 0;
            sentence_font.xy[1]++;
        }
        
        out_text[0] = string_buffer[ string_buffer_offset ];
        out_text[1] = string_buffer[ string_buffer_offset + 1 ];
        out_text[2] = '\0';
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            clickstr_state = CLICK_NONE;
            for ( i=0 ; i<clickstr_num ; i++ ){
                if ( clickstr_list[i*2] == out_text[0] && clickstr_list[i*2+1] == out_text[1] ){
                    if ( sentence_font.xy[1] >= sentence_font.num_xy[1] - clickstr_line ){
                        if ( string_buffer[ string_buffer_offset + 2 ] != '@' && string_buffer[ string_buffer_offset + 2 ] != '\\' ){
                            clickstr_state = CLICK_NEWPAGE;
                        }
                    }
                    else{
                        if ( string_buffer[ string_buffer_offset + 2 ] != '@' && string_buffer[ string_buffer_offset + 2 ] != '\\' ){
                            clickstr_state = CLICK_WAIT;
                        }
                    }
                    for ( j=0 ; j<clickstr_num ; j++ ){
                        if ( clickstr_list[j*2] == string_buffer[ string_buffer_offset + 2 ] &&
                             clickstr_list[j*2+1] == string_buffer[ string_buffer_offset + 3 ] ){
                            clickstr_state = CLICK_NONE;
                        }
                    }
                    if ( string_buffer[ string_buffer_offset + 2 ] == '!' ){
                        if ( string_buffer[ string_buffer_offset + 3 ] == 'w' ||
                             string_buffer[ string_buffer_offset + 3 ] == 'd' )
                            clickstr_state = CLICK_NONE;
                    }
                }
            }
        }
        if ( clickstr_state == CLICK_WAIT ){
            return clickWait( out_text );
        }
        else if ( clickstr_state == CLICK_NEWPAGE ){
            return clickNewPage( out_text );
        }
        else{
            if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ){
                drawChar( out_text, &sentence_font, false );
                string_buffer_offset += 2;
                return RET_CONTINUE;
            }
            else{
                drawChar( out_text, &sentence_font );
                event_mode = WAIT_SLEEP_MODE;
                startTimer( sentence_font.wait_time );
                return RET_WAIT;
            }
        }
    }
    else if ( ch == '@' ){ // wait for click
        return clickWait( NULL );
    }
    else if ( ch == '/' ){ // skip new line
        new_line_skip_flag = true;
        string_buffer_offset++;
        return RET_CONTINUE;
    }
    else if ( ch == '\\' ){ // new page
        return clickNewPage( NULL );
    }
    else if ( ch == '!' ){
        string_buffer_offset++;
        if ( string_buffer[ string_buffer_offset ] == 's' ){
            string_buffer_offset++;
            if ( string_buffer[ string_buffer_offset ] == 'd' ){
                sentence_font.wait_time = default_text_speed[ text_speed_no ];
                string_buffer_offset++;
            }
            else{
                p_string_buffer = &string_buffer[ string_buffer_offset ];
                sentence_font.wait_time = readInt( &p_string_buffer );
                string_buffer_offset = p_string_buffer - string_buffer;
            }
        }
        else if ( string_buffer[ string_buffer_offset ] == 'w' || string_buffer[ string_buffer_offset ] == 'd' ){
            bool flag = false;
            if ( string_buffer[ string_buffer_offset ] == 'd' ) flag = true;
            string_buffer_offset++;
            p_string_buffer = &string_buffer[ string_buffer_offset ];
            t = readInt( &p_string_buffer );
            if ( skip_flag || draw_one_page_flag ){
                string_buffer_offset = p_string_buffer - string_buffer;
                return RET_CONTINUE;
            }
            else{
                event_mode = WAIT_SLEEP_MODE;
                if ( flag ) event_mode |= WAIT_MOUSE_MODE | WAIT_KEY_MODE;
                key_pressed_flag = false;
                startTimer( t );
                string_buffer_offset -= 2;
                return RET_WAIT;
            }
        }
        return RET_CONTINUE;
    }
    else if ( ch == '%' ){ // number variable
        text_char_flag = true;
        p_string_buffer = &string_buffer[ string_buffer_offset ];
        int j = readInt( &p_string_buffer );
        printf("read Int %d\n",j);
        sprintf( num_buf, "%d", j);
        if ( j < 0 ){
            drawChar( "„Ÿ", &sentence_font, false );
            j = -j;
        }
        for ( unsigned int i=0 ; i<strlen(num_buf) ; i++ ){
            getSJISFromInteger( out_text, num_buf[i] - '0', false );
            drawChar( out_text, &sentence_font, false );
        }
        string_buffer_offset = p_string_buffer - string_buffer;
        return RET_CONTINUE;
    }
    else if ( ch == '_' ){ // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE;
    }
    else if ( ch == '\0' ){ // End of line
        printf("end of text\n");
        return RET_CONTINUE;
    }
    else if ( ch == '#' ){
        readColor( &sentence_font.color, string_buffer + string_buffer_offset + 1 );
        string_buffer_offset += 7;
        return RET_CONTINUE;
    }
    else{
        printf("unrecognized text %c\n",ch);
        text_char_flag = true;
        out_text[0] = ch;
        out_text[1] = '\0';
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0){
            drawChar( out_text, &sentence_font, false );
            string_buffer_offset++;
            return RET_CONTINUE;
        }
        else{
            drawChar( out_text, &sentence_font );
            event_mode = WAIT_SLEEP_MODE;
            startTimer( sentence_font.wait_time );
            printf("dispatch timer %d\n",sentence_font.wait_time);
            return RET_WAIT;
        }
    }

    return RET_NOMATCH;
}
