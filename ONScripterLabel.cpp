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

#define DEFAULT_TEXT_SPEED1 60 // Low speed
#define DEFAULT_TEXT_SPEED2 40 // Middle speed
#define DEFAULT_TEXT_SPEED3 20 // High speed

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
    {"textclear",   &ONScripterLabel::textclearCommand},
    {"systemcall",   &ONScripterLabel::systemcallCommand},
    {"stop",   &ONScripterLabel::stopCommand},
    {"setwindow",   &ONScripterLabel::setwindowCommand},
    {"setcursor",   &ONScripterLabel::setcursorCommand},
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
    {"end", &ONScripterLabel::endCommand},
    {"dwavestop", &ONScripterLabel::dwavestopCommand},
    {"dwaveloop", &ONScripterLabel::dwaveCommand},
    {"dwave", &ONScripterLabel::dwaveCommand},
    {"delay", &ONScripterLabel::delayCommand},
    {"csp", &ONScripterLabel::cspCommand},
    {"click", &ONScripterLabel::clickCommand},
    {"cl", &ONScripterLabel::clCommand},
    {"btnwait", &ONScripterLabel::btnwaitCommand},
    {"btndef",  &ONScripterLabel::btndefCommand},
    {"btn",     &ONScripterLabel::btnCommand},
    {"br",      &ONScripterLabel::brCommand},
    {"blt",      &ONScripterLabel::bltCommand},
    {"bg",      &ONScripterLabel::bgCommand},
    {"autoclick",      &ONScripterLabel::autoclickCommand},
    {"amsp",      &ONScripterLabel::amspCommand},
    {"", NULL}
};

int ONScripterLabel::SetVideoMode(int w, int h)
{
    /* ---------------------------------------- */
    /* Initialize SDL */
    if ( TTF_Init() < 0 ){
        fprintf( stderr, "can't initialize SDL TTF\n");
        SDL_Quit();
        exit(-1);
    }
        
    if ( !Sound_Init() ){
        fprintf( stderr, "can't initialize SDL SOUND\n");
        SDL_Quit();
        exit(-1);
    }

	screen_surface = SDL_SetVideoMode(w, h, 32, DEFAULT_SURFACE_FLAG );
	if ( screen_surface == NULL ) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
					w, h, 32, SDL_GetError());
		return(-1);
	}

    SDL_FillRect( screen_surface, NULL, SDL_MapRGBA( screen_surface->format, 0, 0, 0, 0 ) );
	SDL_UpdateRect(screen_surface, 0, 0, 0, 0);

    initSJIS2UTF16();
    
	return(0);
}

static void mp3_callback( void *userdata, Uint8 *stream, int len )
{
    Sound_Sample *sample = (Sound_Sample *)userdata;
    int bw = 0;
    static Uint8 *mp3_decoded_ptr = NULL;
    static Uint32 mp3_decoded_bytes = 0;

    //printf("callback %d\n",Mix_PlayingMusic());
    
    while (bw < len){
        unsigned int cpysize;

        if ( !mp3_decoded_bytes ){
            if (sample->flags & (SOUND_SAMPLEFLAG_ERROR|SOUND_SAMPLEFLAG_EOF)){
                memset( stream + bw, '\0', len - bw );

                SDL_Event event;
                printf("stop music\n");
                event.type = ONS_SOUND_EVENT;
                SDL_PushEvent(&event);

                return;
            }

            mp3_decoded_bytes = Sound_Decode(sample);
            mp3_decoded_ptr = (Uint8*)sample->buffer;
        }

        cpysize = len - bw;
        if ( cpysize > mp3_decoded_bytes ) cpysize = mp3_decoded_bytes;

        if ( cpysize > 0 ){
            memcpy( stream + bw, mp3_decoded_ptr, cpysize );
            bw += cpysize;
            mp3_decoded_ptr += cpysize;
            mp3_decoded_bytes -= cpysize;
        }
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
    
    SetVideoMode( WIDTH, HEIGHT );

    int audio_rate;
    Uint16 audio_format;
    int audio_channels;

    if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF ) < 0 ){
        fprintf(stderr, "Couldn't open audio device!\n"
                "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else{
        audio_rate = MIX_DEFAULT_FREQUENCY;
        audio_format = MIX_DEFAULT_FORMAT;
        audio_channels = 2;

        Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
        printf("Opened audio at %d Hz %d bit %s\n", audio_rate,
               (audio_format&0xFF),
               (audio_channels > 1) ? "stereo" : "mono");
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

    background_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( background_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    accumulation_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( accumulation_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    select_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( select_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    text_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( text_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    effect_src_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( effect_src_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    effect_dst_surface   = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( effect_dst_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    shelter_select_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( shelter_select_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    shelter_text_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask );
    SDL_SetAlpha( shelter_text_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );

    SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
    SDL_BlitSurface( accumulation_surface, NULL, select_surface, NULL );
    SDL_BlitSurface( select_surface, NULL, text_surface, NULL );

    flush();

    internal_timer = SDL_GetTicks();
    autoclick_timer = 0;

    monocro_flag = false;

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    skip_flag = false;
    draw_one_page_flag = false;
    //draw_one_page_flag = true;
    key_pressed_flag = false;
    new_page_flag = false;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    rmode_flag = true;
    
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
        tachi_image_x[i] = 0;
        tachi_image_width[i] = 0;
        tachi_image_name[i] = NULL;
        tachi_image_surface[i] = NULL;
    }

    /* ---------------------------------------- */
    /* Sprite related variables */
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        sprite_info[i].valid = false;
        sprite_info[i].name = NULL;
        sprite_info[i].tag.file_name = NULL;
        sprite_info[i].image_surface = NULL;
    }
    
    /* ---------------------------------------- */
    /* Cursor related variables */
    for ( i=0 ; i<2 ; i++ ){
        cursor_info[i].image_name = NULL;
        cursor_info[i].image_surface = NULL;
        cursor_info[i].preserve_surface = NULL;
        
        cursor_info[i].count = 0;
        cursor_info[i].direction = 1;
        if ( i==CURSOR_WAIT_NO ) loadCursor( CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0 );
        else                     loadCursor( CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0 );
    }
    
    /* ---------------------------------------- */
    /* Sound related variables */
    mp3_sample = NULL;
    mp3_file_name = NULL;
    mp3_buffer = NULL;
    current_cd_track = -1;
    wave_sample = NULL;
    
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
    memcpy( sentence_font.window_color, "#999999", 8 );
    sentence_font.window_color_mask[0] = sentence_font.window_color_mask[1] = sentence_font.window_color_mask[2] = 0x99;
    sentence_font.window_image = NULL;
    sentence_font.window_rect[0] = 0;
    sentence_font.window_rect[1] = 0;
    sentence_font.window_rect[2] = 639;
    sentence_font.window_rect[3] = 479;

    sentence_font.on_color[0] = sentence_font.on_color[1] = sentence_font.on_color[2] = 0xff;
    sentence_font.off_color[0] = sentence_font.off_color[1] = sentence_font.off_color[2] = 0x80;
    
    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;

    /* for savfile */
    bg_effect_image = COLOR_EFFECT_IMAGE;

    /* for debugging */
    //num_variables[351] = 1;
    //num_variables[1020] = 1;
    //timer_id = NULL;

    clearCurrentTextBuffer();
    
    startTimer( MINIMUM_TIMER_RESOLUTION );
}

ONScripterLabel::~ONScripterLabel( )
{
}

void ONScripterLabel::flush( int x, int y, int wx, int wy )
{
    SDL_Rect rect;

    if ( x >= 0 ){
        rect.x = x;
        rect.y = y;
        rect.w = wx;
        rect.h = wy;
        
        SDL_BlitSurface( text_surface, &rect, screen_surface, &rect );
        SDL_UpdateRect( screen_surface, x, y, wx, wy );
    }
    else{
        SDL_BlitSurface( text_surface, NULL, screen_surface, NULL );
        SDL_UpdateRect( screen_surface, 0, 0, 0, 0 );
    }
}

int ONScripterLabel::eventLoop()
{
	SDL_Event event;

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
    }
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

void ONScripterLabel::timerEvent( void )
{
    
    if ( sentence_font.wait_time == 0 )
        printf("timer wait_time = 0\n");

    //printf("timerEvent %d\n", event_mode);

  timerEventTop:
    
    int ret;
    unsigned int i;

    if ( event_mode & WAIT_CURSOR_MODE ){
        int no;
    
        if ( clickstr_state == CLICK_WAIT )         no = CURSOR_WAIT_NO;
        else if ( clickstr_state == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
        //printf("cursor dayo %d %d\n", no,cursor_info[ no ].tag.num_of_cells);
        //clickstr_state = CLICK_NONE;
        
        if ( cursor_info[ no ].tag.num_of_cells > 0 ){
            if ( cursor_info[ no ].image_surface ){
                SDL_Rect src_rect, dst_rect;
                src_rect.x = cursor_info[ no ].image_surface->w * cursor_info[ no ].count / cursor_info[ no ].tag.num_of_cells;
                src_rect.y = 0;
                src_rect.w = cursor_info[ no ].w;
                src_rect.h = cursor_info[ no ].h;
                dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].xy[0];
                dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].xy[1];
                alphaBlend( text_surface, dst_rect.x, dst_rect.y,
                            cursor_info[ no ].preserve_surface, 0, 0, cursor_info[ no ].preserve_surface->w, cursor_info[ no ].preserve_surface->h,
                            cursor_info[ no ].image_surface, src_rect.x, src_rect.y,
                            0, 0, -cursor_info[ no ].tag.trans_mode );
                //SDL_BlitSurface( cursor_info[ no ].image_surface, &src_rect, text_surface, &dst_rect );
                flush( dst_rect.x, dst_rect.y, src_rect.w, src_rect.h );
            }

            cursor_info[ no ].count +=  cursor_info[ no ].direction;

            if ( cursor_info[ no ].count < 0 ){
                if ( cursor_info[ no ].tag.loop_mode == 1 )
                    cursor_info[ no ].count = 0;
                else
                    cursor_info[ no ].count = 1;
                cursor_info[ no ].direction = 1;
            }
            else if ( cursor_info[ no ].count >= cursor_info[ no ].tag.num_of_cells ){
                if ( cursor_info[ no ].tag.loop_mode == 0 ){
                    cursor_info[ no ].count = 0;
                }
                else if ( cursor_info[ no ].tag.loop_mode == 1 ){
                    cursor_info[ no ].count--;
                }
                else{
                    cursor_info[ no ].count = cursor_info[ no ].tag.num_of_cells - 2;
                    cursor_info[ no ].direction = -1;
                }
            }
            //printf("timer %d %d\n",cursor_info[ no ].count,cursor_info[ no ].tag.duration_list[ cursor_info[ no ].count ]);
            startTimer( cursor_info[ no ].tag.duration_list[ cursor_info[ no ].count ] );
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
        if ( system_menu_mode != SYSTEM_NULL || (event_mode & WAIT_MOUSE_MODE && volatile_button_state.button == -1)  ){
            executeSystemCall();
        }
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
            SDL_BlitSurface( select_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );
            flush( current_over_button_link.image_rect.x, current_over_button_link.image_rect.y, current_over_button_link.image_rect.w, current_over_button_link.image_rect.h );
        }
        first_mouse_over_flag = false;

        if ( p_button_link ){
            if ( event_mode & WAIT_BUTTON_MODE && p_button_link->image_surface ){
                SDL_BlitSurface( p_button_link->image_surface, NULL, text_surface, &p_button_link->image_rect );
                if ( monocro_flag && !(event_mode & WAIT_MOUSE_MODE) ) makeMonochromeSurface( text_surface, &p_button_link->image_rect );
                flush( p_button_link->image_rect.x, p_button_link->image_rect.y, p_button_link->image_rect.w, p_button_link->image_rect.h );
            }
            current_over_button_link.image_rect.x = p_button_link->image_rect.x;
            current_over_button_link.image_rect.y = p_button_link->image_rect.y;
            current_over_button_link.image_rect.w = p_button_link->image_rect.w;
            current_over_button_link.image_rect.h = p_button_link->image_rect.h;
            shortcut_mouse_line = c;
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
    if ( sentence_font.wait_time == 0 )
        printf("exe wait_time = 0\n");

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
        if ( jumpf_flag ){
            skipToken();
            if ( string_buffer[string_buffer_offset] == '\0' ) i++;
            continue;
        }
        if ( break_flag ){
            if ( !strncmp( string_buffer + string_buffer_offset, "next", 4 ) ) break_flag = false;
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
                //startTimer( MINIMUM_TIMER_RESOLUTION );
                //return;
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
            refreshAccumulationSruface( effect_dst_surface );
            break;

          case BG_EFFECT_IMAGE:
            tmp_surface = loadPixmap( tag );
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.w = tmp_surface->w;
            src_rect.h = tmp_surface->h;
            dst_rect.x = (WIDTH - tmp_surface->w) / 2;
            dst_rect.y = (HEIGHT - tmp_surface->h) / 2;

            SDL_BlitSurface( tmp_surface, &src_rect, background_surface, &dst_rect );
            refreshAccumulationSruface( effect_dst_surface );
            break;
            
          case TACHI_EFFECT_IMAGE:
            refreshAccumulationSruface( effect_dst_surface );
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
        for ( i=0 ; i<WIDTH/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = HEIGHT;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 3: // Right shutter
        width = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=WIDTH/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH - width - 1;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = HEIGHT;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 4: // Top shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=0 ; i<HEIGHT/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH;
            src_rect.w = dst_rect.w = WIDTH;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 5: // Bottom shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=HEIGHT/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH - height - 1;
            src_rect.w = dst_rect.w = WIDTH;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 6: // Left curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=WIDTH/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / WIDTH;
            if ( width2 >= 0 ){
                src_rect.x = dst_rect.x = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = HEIGHT;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 7: // Right curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=WIDTH/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / WIDTH;
            if ( width2 >= 0 ){
                if ( width2 > EFFECT_STRIPE_CURTAIN_WIDTH ) width2 = EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.x = dst_rect.x = WIDTH - i * EFFECT_STRIPE_CURTAIN_WIDTH - width2;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = HEIGHT;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 8: // Top curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=HEIGHT/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / HEIGHT;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.w = dst_rect.w = WIDTH;
                src_rect.h = dst_rect.h = height2;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 9: // Bottom curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=HEIGHT/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / HEIGHT;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = HEIGHT - i * EFFECT_STRIPE_CURTAIN_WIDTH - height2;
                src_rect.w = dst_rect.w = WIDTH;
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
                    effect_src_surface, 0, 0, WIDTH, HEIGHT,
                    effect_dst_surface, 0, 0,
                    0, 0, height );
        break;
        
      case 11: // Left scroll
        width = WIDTH * effect_counter / effect.duration;
        src_rect.x = 0;
        dst_rect.x = width;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = WIDTH - width;
        src_rect.h = dst_rect.h = HEIGHT;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = WIDTH - width - 1;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = HEIGHT;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 12: // Right scroll
        width = WIDTH * effect_counter / effect.duration;
        src_rect.x = width;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = WIDTH - width;
        src_rect.h = dst_rect.h = HEIGHT;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = 0;
        dst_rect.x = WIDTH - width - 1;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = HEIGHT;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 13: // Top scroll
        width = HEIGHT * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = width;
        src_rect.w = dst_rect.w = WIDTH;
        src_rect.h = dst_rect.h = HEIGHT - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = HEIGHT - width - 1;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = WIDTH;
        src_rect.h = dst_rect.h = width;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 14: // Bottom scroll
        width = HEIGHT * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = width;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = WIDTH;
        src_rect.h = dst_rect.h = HEIGHT - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = HEIGHT - width - 1;
        src_rect.w = dst_rect.w = WIDTH;
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
#if 0    
    if ( not_selected_flag ) color.r = color.g = color.b = 0x80;
    else{
#endif        
        color.r = info->color[0];
        color.g = info->color[1];
        color.b = info->color[2];
        //}

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
            SDL_FillRect( effect_dst_surface, NULL, SDL_MapRGBA( effect_dst_surface->format, 0, 0, 0, 0 ) );
            alphaBlend( text_surface, 0, 0,
                        select_surface, 0, 0, WIDTH, HEIGHT,
                        effect_dst_surface, 0, 0,
                        0, 0, sentence_font.window_color_mask[0] );

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
        else if ( mask_value == -TRANS_TOPLEFT ){
            *src2_buffer &= ~amask;
            Uint32 topleft = *src2_buffer;
            mask = (Uint32)0xff << src2_surface->format->Ashift;
            for ( i=0; i<wy ; i++ ) {
                for ( j=0 ; j<wx ; j++ ){
                    *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) &= ~amask;
                    if ( *(src2_buffer + src2_surface->w * (y2+i) + x2 + j) != topleft )
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
                tag->color_list[i][0] = convHexToDec( buffer[1] ) << 4 | convHexToDec( buffer[2] );
                tag->color_list[i][1] = convHexToDec( buffer[3] ) << 4 | convHexToDec( buffer[4] );
                tag->color_list[i][2] = convHexToDec( buffer[5] ) << 4 | convHexToDec( buffer[6] );
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
            
        tag->loop_mode = tmp;

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

void ONScripterLabel::enterNewPage( )
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( current_text_buffer->xy[0] != 0 || current_text_buffer->xy[1] != 0 )
        current_text_buffer = current_text_buffer->next;
    clearCurrentTextBuffer();

    /* ---------------------------------------- */
    /* Clear the screen */
    SDL_FillRect( effect_src_surface, NULL, SDL_MapRGB( effect_src_surface->format, 0, 0, 0 ) );
    alphaBlend( text_surface, 0, 0,
                accumulation_surface, 0, 0, WIDTH, HEIGHT,
                effect_src_surface, 0, 0,
                0, 0, sentence_font.window_color_mask[0] );

    flush();
}


int ONScripterLabel::playMP3( int cd_no )
{
    char file_name[128];

    if ( mp3_file_name == NULL ){
        sprintf( file_name, "cd%ctrack%2.2d.mp3", DELIMITER, cd_no );
        printf("playMP3 %s\n", file_name );
        mp3_sample = Sound_NewSampleFromFile( file_name, NULL, DEFAULT_DECODEBUF );
    }
    else{
        unsigned long length;
    
        length = cBR->getFileLength( mp3_file_name );
        printf(" ... loading %s length %ld\n",mp3_file_name, length );
        mp3_buffer = new unsigned char[length];
        cBR->getFile( mp3_file_name, mp3_buffer );
        mp3_sample = Sound_NewSample( SDL_RWFromMem( mp3_buffer, length ), "mp3", NULL, DEFAULT_DECODEBUF );
    }

    if ( !mp3_sample ){
        fprintf( stderr, "Couldn't load \"%s\"!\n"
                 "  reason: [%s].\n", file_name, Sound_GetError() );
        return -1;
    }

    Mix_HookMusic( mp3_callback, mp3_sample );

    return 0;
}

int ONScripterLabel::playWave( char *file_name, bool loop_flag )
{
    printf("playWave %s\n", file_name );

    unsigned long length;
    unsigned char *buffer;
    
    length = cBR->getFileLength( file_name );
    buffer = new unsigned char[length];
    cBR->getFile( file_name, buffer );
    wave_sample = Mix_LoadWAV_RW(SDL_RWFromMem( buffer, length ), 1);
    delete[] buffer;

    Mix_PlayChannel( 0, wave_sample, loop_flag?-1:0 );

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
    button_link = new struct ButtonLink();

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
    button_link->next = NULL;

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

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, int x, int y, int w, int h,
                                         SDL_Surface *src_surface, TaggedInfo *tagged_info )
{
    SDL_Rect dst_rect;
    int offset = 0, i;

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
        sentence_font.top_xy[0] = x;
        sentence_font.top_xy[1] = y;
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
    }
    else if ( tagged_info->trans_mode == TRANS_ALPHA ||
              tagged_info->trans_mode == TRANS_TOPLEFT ){
        if ( tagged_info->trans_mode == TRANS_ALPHA ) offset = w;
        alphaBlend( dst_surface, x, y,
                    dst_surface, x, y, w, h,
                    src_surface, 0, 0,
                    offset, 0, -tagged_info->trans_mode );
    }
    else if ( tagged_info->trans_mode == TRANS_COPY ){
        dst_rect.x = x;
        dst_rect.y = y;
        SDL_BlitSurface( src_surface, NULL, dst_surface, &dst_rect );
    }
}

void ONScripterLabel::makeMonochromeSurface( SDL_Surface *surface, SDL_Rect *dst_rect )
{
    int i, j;
    SDL_Rect rect;
    Uint32 *buf;
    Uint16 c;
    
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
            c = (((*buf >> surface->format->Rshift) & 0xff) * 77 +
                 ((*buf >> surface->format->Gshift) & 0xff) * 151 +
                 ((*buf >> surface->format->Bshift) & 0xff) * 28 ) >> 8; 
            *buf = monocro_color_lut[c][0] << surface->format->Rshift |
                monocro_color_lut[c][1] << surface->format->Gshift |
                monocro_color_lut[c][2] << surface->format->Bshift;
        }
        buf += surface->w - rect.w;
    }
    SDL_UnlockSurface( surface );
}

void ONScripterLabel::refreshAccumulationSruface( SDL_Surface *surface )
{
    int i, w, h;
    TaggedInfo tag;
    
    SDL_BlitSurface( background_surface, NULL, surface, NULL );

    for ( i=MAX_SPRITE_NUM-1 ; i>z_order ; i-- ){
        if ( sprite_info[i].valid ){
            if ( sprite_info[i].image_surface ){
                w = sprite_info[i].image_surface->w;
                h = sprite_info[i].image_surface->h;
            }
            if ( sprite_info[i].tag.trans_mode == TRANS_ALPHA ) w /= 2;
            drawTaggedSurface( surface, sprite_info[i].x, sprite_info[i].y, w, h,
                               sprite_info[i].image_surface, &sprite_info[i].tag );
        }
    }
    for ( i=0 ; i<3 ; i++ ){
        if ( tachi_image_name[i] ){
            if ( tachi_image_surface[i] ){
                w = tachi_image_surface[i]->w;
                h = tachi_image_surface[i]->h;
            }
            parseTaggedString( tachi_image_name[i], &tag );
            if ( tag.trans_mode == TRANS_ALPHA ) w /= 2;
            drawTaggedSurface( surface,
                               WIDTH * (i+1) / 4 - w / 2,
                               underline_value - h + 1,
                               w,
                               h,
                               tachi_image_surface[i],
                               &tag );
        }
    }
    for ( i=z_order ; i>=0 ; i-- ){
        if ( sprite_info[i].valid ){
            if ( sprite_info[i].image_surface ){
                w = sprite_info[i].image_surface->w;
                h = sprite_info[i].image_surface->h;
            }
            if ( sprite_info[i].tag.trans_mode == TRANS_ALPHA ) w /= 2;
            drawTaggedSurface( surface, sprite_info[i].x, sprite_info[i].y, w, h,
                               sprite_info[i].image_surface, &sprite_info[i].tag );
        }
    }

    if ( monocro_flag ) makeMonochromeSurface( surface );
}

int ONScripterLabel::clickWait( char *out_text )
{
    clickstr_state = CLICK_WAIT;
    if ( skip_flag || draw_one_page_flag ){
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
        if ( out_text ){
            drawChar( out_text, &sentence_font );
        }
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
    if ( skip_flag ){
        if ( out_text ){
            drawChar( out_text, &sentence_font, false );
        }
        if ( !new_page_flag ){
            flush();
            new_page_flag = true;
            event_mode = WAIT_SLEEP_MODE;
            startTimer( MINIMUM_TIMER_RESOLUTION );
            return RET_WAIT;
        }

        new_page_flag = false;
        event_mode = IDLE_EVENT_MODE;

        string_buffer_offset++;
        if ( out_text ) string_buffer_offset++;
        enterNewPage();
        new_line_skip_flag = true;

        return RET_CONTINUE;
    }
    else{
        if ( out_text ){
            drawChar( out_text, &sentence_font );
        }
        if ( draw_one_page_flag ) flush();
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
        return RET_WAIT;
    }
}

void ONScripterLabel::makeEffectStr( char *buf, int no, int duration, char *image )
{
    sprintf( buf + strlen(buf), "%d", no );
    if ( duration ){
        sprintf( buf + strlen(buf), ",%d", duration );
        if ( image[0] != '\0' ){
            sprintf( buf + strlen(buf), ",%s", image );
        }
    }
}

void ONScripterLabel::loadCursor( int no, char *str, int x, int y )
{
    //printf("load Cursor %s\n",str);
    if ( cursor_info[ no ].image_name ) delete[] cursor_info[ no ].image_name;
    cursor_info[ no ].image_name = new char[ strlen( str ) + 1 ];
    memcpy( cursor_info[ no ].image_name, str, strlen( str ) + 1 );

    cursor_info[ no ].xy[0] = x;
    cursor_info[ no ].xy[1] = y;

    parseTaggedString( cursor_info[ no ].image_name, &cursor_info[ no ].tag );

    if ( cursor_info[ no ].image_surface ) SDL_FreeSurface( cursor_info[ no ].image_surface );
    cursor_info[ no ].image_surface = loadPixmap( &cursor_info[ no ].tag );

    if ( cursor_info[ no ].tag.num_of_cells > 0 && cursor_info[ no ].image_surface ){
        cursor_info[ no ].w = cursor_info[ no ].image_surface->w / cursor_info[ no ].tag.num_of_cells;
        cursor_info[ no ].h = cursor_info[ no ].image_surface->h;
        if ( cursor_info[ no ].preserve_surface ) SDL_FreeSurface( cursor_info[ no ].preserve_surface );
        cursor_info[ no ].preserve_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, cursor_info[ no ].w, cursor_info[ no ].h, 32, rmask, gmask, bmask, amask );
        SDL_SetAlpha( cursor_info[ no ].preserve_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    }
}

void ONScripterLabel::startCursor( int click )
{
    SDL_Rect src_rect;
    int no;
    
    if ( click == CLICK_WAIT ) no = CURSOR_WAIT_NO;
    else if ( click == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
    else return;

    src_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].xy[0];
    src_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].xy[1];
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
    
    dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + cursor_info[ no ].xy[0];
    dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + cursor_info[ no ].xy[1];
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
                readInt( &p_string_buffer, tmp_string_buffer );
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
              ( string_buffer[ string_buffer_offset + 2 ] == (char)0x81 && string_buffer[ string_buffer_offset + 3 ] == (char)0x76 )) ){
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
            if ( skip_flag || draw_one_page_flag ){
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
    else if ( ch == '/' ){ // skip new page
        new_line_skip_flag = true;
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
                sentence_font.wait_time = readInt( &p_string_buffer, tmp_string_buffer );
                if ( sentence_font.wait_time == 0 ){
                    printf("damedayo wait = 0\n");
                    exit(0);
                }
                string_buffer_offset = p_string_buffer - string_buffer;
            }
        }
        else if ( string_buffer[ string_buffer_offset ] == 'w' || string_buffer[ string_buffer_offset ] == 'd' ){
            bool flag = false;
            if ( string_buffer[ string_buffer_offset ] == 'd' ) flag = true;
            string_buffer_offset++;
            p_string_buffer = &string_buffer[ string_buffer_offset ];
            t = readInt( &p_string_buffer, tmp_string_buffer );
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
        int j = readInt( &p_string_buffer, tmp_string_buffer );
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
        string_buffer_offset++;
        sentence_font.color[0] = convHexToDec( string_buffer[ string_buffer_offset++ ] ) << 4 |
            convHexToDec( string_buffer[ string_buffer_offset++ ] );
        sentence_font.color[1] = convHexToDec( string_buffer[ string_buffer_offset++ ] ) << 4 |
            convHexToDec( string_buffer[ string_buffer_offset++ ] );
        sentence_font.color[2] = convHexToDec( string_buffer[ string_buffer_offset++ ] ) << 4 |
            convHexToDec( string_buffer[ string_buffer_offset++ ] );
        return RET_CONTINUE;
    }
    else{
        printf("unrecognized text %c\n",ch);
        text_char_flag = true;
        out_text[0] = ch;
        out_text[1] = '\0';
        if ( skip_flag || draw_one_page_flag ){
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
    playWave( tmp_string_buffer, wave_play_once_flag );
        
    return RET_CONTINUE;
}

int ONScripterLabel::wavestopCommand()
{
    if ( wave_sample ){
        Mix_Pause(0);
        Mix_FreeChunk( wave_sample );
        wave_sample = NULL;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::waittimerCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("waittimer") = 9

    int count = readInt( &p_string_buffer, tmp_string_buffer ) + internal_timer - SDL_GetTicks();
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

    startTimer( readInt( &p_string_buffer, tmp_string_buffer ) );

    return RET_WAIT_NEXT;
}

int ONScripterLabel::vspCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("vsp") = 3

    int no = readInt( &p_string_buffer, tmp_string_buffer );
    int v = readInt( &p_string_buffer, tmp_string_buffer );
    printf(" vspCommand %d %d\n", no, v );

    sprite_info[ no ].valid = (v==1)?true:false;

    return RET_CONTINUE;
}

int ONScripterLabel::textclearCommand()
{
    printf("textclearCommand\n");
    clearCurrentTextBuffer();

    /* ---------------------------------------- */
    /* Clear the screen */
    SDL_FillRect( effect_src_surface, NULL, SDL_MapRGB( effect_src_surface->format, 0, 0, 0 ) );
    alphaBlend( text_surface, 0, 0,
                accumulation_surface, 0, 0, WIDTH, HEIGHT,
                effect_src_surface, 0, 0,
                0, 0, sentence_font.window_color_mask[0] );
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

int ONScripterLabel::setwindowCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("setwindow") = 9

    sentence_font.font_valid_flag = false;
    sentence_font.top_xy[0] = readInt( &p_string_buffer, tmp_string_buffer );
    sentence_font.top_xy[1] = readInt( &p_string_buffer, tmp_string_buffer );
    sentence_font.num_xy[0] = readInt( &p_string_buffer, tmp_string_buffer );
    sentence_font.num_xy[1] = readInt( &p_string_buffer, tmp_string_buffer );
    sentence_font.font_size = readInt( &p_string_buffer, tmp_string_buffer );
    //if ( sentence_font.font_size < 18 ) sentence_font.font_size = 10; // work aroud for embedded bitmaps
    readInt( &p_string_buffer, tmp_string_buffer ); // Ignore font size along Y axis
    sentence_font.pitch_xy[0] = readInt( &p_string_buffer, tmp_string_buffer ) + sentence_font.font_size;
    sentence_font.pitch_xy[1] = readInt( &p_string_buffer, tmp_string_buffer ) + sentence_font.font_size;
    sentence_font.wait_time = readInt( &p_string_buffer, tmp_string_buffer );
    if ( sentence_font.wait_time == 0 ) sentence_font.wait_time = 10;
    sentence_font.display_bold = readInt( &p_string_buffer, tmp_string_buffer )?true:false;
    sentence_font.display_shadow = readInt( &p_string_buffer, tmp_string_buffer )?true:false;

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
        assert( strlen( tmp_string_buffer ) == 7 );
        sentence_font.window_color_mask[0] = convHexToDec( tmp_string_buffer[1] ) << 4 | convHexToDec( tmp_string_buffer[2] );
        sentence_font.window_color_mask[1] = convHexToDec( tmp_string_buffer[3] ) << 4 | convHexToDec( tmp_string_buffer[4] );
        sentence_font.window_color_mask[2] = convHexToDec( tmp_string_buffer[5] ) << 4 | convHexToDec( tmp_string_buffer[6] );

        sentence_font.window_rect[0] = readInt( &p_string_buffer, tmp_string_buffer );
        sentence_font.window_rect[1] = readInt( &p_string_buffer, tmp_string_buffer );
        sentence_font.window_rect[2] = readInt( &p_string_buffer, tmp_string_buffer );
        sentence_font.window_rect[3] = readInt( &p_string_buffer, tmp_string_buffer );
#if 0
        printf("    trans %s rect %d %d %d %d\n",
               sentence_font.window_color,
               sentence_font.window_rect[0], sentence_font.window_rect[1], sentence_font.window_rect[2], sentence_font.window_rect[3] );
#endif        
    }
    else{
        sentence_font.display_transparency = false;
        if ( sentence_font.window_image ) delete[] sentence_font.window_image;
        sentence_font.window_image = new char[ strlen( tmp_string_buffer ) + 1 ];
        memcpy( sentence_font.window_image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        sentence_font.window_rect[0] = readInt( &p_string_buffer, tmp_string_buffer );
        sentence_font.window_rect[1] = readInt( &p_string_buffer, tmp_string_buffer );
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("setcursor") = 9
    int x, y, no;
    char *str;
    
    no = readInt( &p_string_buffer, tmp_string_buffer );

    readStr( &p_string_buffer, tmp_string_buffer );
    str = new char[ strlen( tmp_string_buffer ) + 1 ];
    memcpy( str, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

    x = readInt( &p_string_buffer, tmp_string_buffer );
    y = readInt( &p_string_buffer, tmp_string_buffer );

    loadCursor( no, str, x, y );
    delete[] str;
    
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
    char *p_string_buffer;
    if ( !strncmp( string_buffer + string_buffer_offset, "selgosub", 8 ) ){
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
        }
        else{
            current_link_label_info->current_line = select_label_info.current_line;
            current_link_label_info->offset = select_label_info.offset;
            gosubReal( last_select_link->label );
        }
        deleteSelectLink();

        SDL_BlitSurface( background_surface, NULL, accumulation_surface, NULL );
        enterNewPage();

        return RET_JUMP;
    }
    else{
        //printf("\a");
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
                if ( ++count % 2 ){
                    tmp_select_link = new SelectLink();
                    tmp_select_link->next = NULL;
                    tmp_select_link->text = new char[ strlen(tmp_string_buffer) + 1 ];
                    memcpy( tmp_select_link->text, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
                }
                else{
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

    int no = readInt( &p_string_buffer, tmp_string_buffer );
    saveSaveFile( no );
    return RET_CONTINUE;
}

int ONScripterLabel::rndCommand()
{
    char *p_string_buffer;
    int no, upper, lower;
    
    if ( !strncmp( string_buffer + string_buffer_offset, "rnd2", 4 ) ){
        p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("rnd2") = 4
        readToken( &p_string_buffer, tmp_string_buffer );
        no = atoi( tmp_string_buffer + 1 );
        lower = readInt( &p_string_buffer, tmp_string_buffer );
        upper = readInt( &p_string_buffer, tmp_string_buffer );
    }
    else{
        p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("rnd") = 3
        readToken( &p_string_buffer, tmp_string_buffer );
        no = atoi( tmp_string_buffer + 1 );
        lower = 0;
        upper = readInt( &p_string_buffer, tmp_string_buffer ) - 1;
    }

    setNumVariable( no, lower + (int)( (double)(upper-lower+1)*rand()/(RAND_MAX+1.0)) );

    return RET_CONTINUE;
}

int ONScripterLabel::rmodeCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("rmode") = 5
    if ( readInt( &p_string_buffer, tmp_string_buffer ) == 1 ) rmode_flag = true;
    else                                                       rmode_flag = false;

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
    char *buf = new char[512];
    int ret;

    sprintf( buf, "print " );
    print_effect.effect = readInt( &p_string_buffer, tmp_string_buffer );
    print_effect.duration = readInt( &p_string_buffer, tmp_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    
    if ( event_mode & EFFECT_EVENT_MODE ){
        if ( print_effect.duration ){
            if ( print_effect.image ) delete[] print_effect.image;
            print_effect.image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( print_effect.image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
            ret = doEffect( PRINT_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        }
        else
            ret = doEffect( print_effect.effect, NULL, TACHI_EFFECT_IMAGE );
        delete[] buf;
        return ret;
    }
    else{
        makeEffectStr( buf, print_effect.effect, print_effect.duration, tmp_string_buffer );
        if ( print_effect.effect == 0 ) return setEffect( RET_CONTINUE, buf );
        else                            return setEffect( RET_WAIT_NEXT, buf );
    }
    
    return RET_CONTINUE;
    
}

int ONScripterLabel::playstopCommand()
{
    if ( mp3_sample ){
        Mix_FadeOutMusic( 1000 );
        Mix_HookMusic( NULL, NULL );

        if ( mp3_sample->flags & SOUND_SAMPLEFLAG_ERROR ){
            fprintf(stderr, "Error in decoding sound file!\n"
                    "  reason: [%s].\n", Sound_GetError());
        }

        Sound_FreeSample( mp3_sample );
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

int ONScripterLabel::mspCommand()
{
    int no;
    char *p_string_buffer;

    p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("msp") = 3;

    no = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].x += readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].y += readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].trans += readInt( &p_string_buffer, tmp_string_buffer );
    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;

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

    no = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].valid = v;
    readStr( &p_string_buffer, tmp_string_buffer );
    if ( sprite_info[ no ].name ) delete[] sprite_info[ no ].name;
    sprite_info[ no ].name = new char[ strlen(tmp_string_buffer) + 1 ];
    memcpy( sprite_info[ no ].name, tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
    if ( sprite_info[ no ].image_surface ) SDL_FreeSurface( sprite_info[ no ].image_surface );
    parseTaggedString( sprite_info[ no ].name, &sprite_info[ no ].tag );

    sprite_info[ no ].image_surface = loadPixmap( &sprite_info[ no ].tag );

    sprite_info[ no ].x = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].y = readInt( &p_string_buffer, tmp_string_buffer );
    if ( *p_string_buffer != '\0' ){
        sprite_info[ no ].trans = readInt( &p_string_buffer, tmp_string_buffer );
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

    sentence_font.xy[0] = readInt( &p_string_buffer, tmp_string_buffer );
    sentence_font.xy[1] = readInt( &p_string_buffer, tmp_string_buffer );

    return RET_CONTINUE;
}

int ONScripterLabel::loadgameCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("loadgame") = 8
    
    int no = readInt( &p_string_buffer, tmp_string_buffer );
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
    char *buf = new char[512];
    int no, effect_no, i, ret;
    TaggedInfo tagged_info;
    
    readStr( &p_string_buffer, tmp_string_buffer );
    sprintf( buf, "ld %s",tmp_string_buffer );
    if ( tmp_string_buffer[0] == 'l' )      no = 0;
    else if ( tmp_string_buffer[0] == 'c' ) no = 1;
    else if ( tmp_string_buffer[0] == 'r' ) no = 2;

    readStr( &p_string_buffer, tmp_string_buffer );
    sprintf( buf+strlen(buf), ", \"%s\",", tmp_string_buffer );
    
    if ( event_mode & EFFECT_EVENT_MODE ){
        effect_no = readInt( &p_string_buffer, tmp_string_buffer );
        i = readInt( &p_string_buffer, tmp_string_buffer );
        if ( i != 0 ){
            tmp_effect.effect = effect_no;
            tmp_effect.duration = i;
            readStr( &p_string_buffer, tmp_string_buffer );
            if ( tmp_effect.image ) delete[] tmp_effect.image;
            tmp_effect.image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( tmp_effect.image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );

            ret = doEffect( TMP_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        }
        else{
            ret = doEffect( effect_no, NULL, TACHI_EFFECT_IMAGE );
        }

        delete[] buf;
        return ret;
    }
    else{
        if ( tachi_image_name[no] ) delete[] tachi_image_name[no];
        tachi_image_name[no] = new char[ strlen(tmp_string_buffer) + 1 ];
        memcpy( tachi_image_name[no], tmp_string_buffer, strlen(tmp_string_buffer) + 1 );
        parseTaggedString( tachi_image_name[no], &tagged_info );
    
        if ( tachi_image_surface[no] ) SDL_FreeSurface( tachi_image_surface[no] );
        tachi_image_surface[no] = loadPixmap( &tagged_info );

        effect_no = readInt( &p_string_buffer, tmp_string_buffer );
        i = readInt( &p_string_buffer, tmp_string_buffer );
        readStr( &p_string_buffer, tmp_string_buffer );
        makeEffectStr( buf, effect_no, i, tmp_string_buffer );

        if ( effect_no == 0 ) return setEffect( RET_CONTINUE, buf );
        else                  return setEffect( RET_WAIT_NEXT, buf );
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

    readToken( &p_string_buffer, tmp_string_buffer );
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

    int no = atoi( tmp_string_buffer + 1 );
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );

    setNumVariable( no, ONSCRITER_VERSION );
    //printf("getversionCommand %d\n", num_variables[ no ] );
    return RET_CONTINUE;
}

int ONScripterLabel::gettimerCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 8; // strlen("gettimer") = 8 
 
    readToken( &p_string_buffer, tmp_string_buffer ); 
    if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );
 
    int no = atoi( tmp_string_buffer + 1 ); 
    if ( no >= VARIABLE_RANGE ) errorAndExit( string_buffer + string_buffer_offset );
 
    setNumVariable( no, SDL_GetTicks() - internal_timer ); 
 
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
    return wavestopCommand();
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

    readInt( &p_string_buffer, tmp_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
    playWave( tmp_string_buffer, wave_play_once_flag );
        
    return RET_CONTINUE;
}

int ONScripterLabel::delayCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 5; // strlen("delay") = 5

    int t = readInt( &p_string_buffer, tmp_string_buffer );

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
    int no = readInt( &p_string_buffer, tmp_string_buffer );

    if ( no == -1 )
        for ( int i=0 ; i<MAX_SPRITE_NUM ; i++ ) sprite_info[i].valid = false;
    else{
        sprite_info[no].valid = false;
        if ( sprite_info[no].name ){
            delete[] sprite_info[no].name;
            sprite_info[no].name = NULL;
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
    char *buf = new char[512];
    int effect_no, i, ret;
    
    readStr( &p_string_buffer, tmp_string_buffer );
    sprintf( buf, "cl %s,",tmp_string_buffer );
    
    if ( tmp_string_buffer[0] == 'l' || tmp_string_buffer[0] == 'a' ){
        if ( tachi_image_name[0] ) delete[] tachi_image_name[0];
        tachi_image_name[0] = NULL;
        if ( tachi_image_surface[0] ) SDL_FreeSurface( tachi_image_surface[0] );
        tachi_image_surface[0] = NULL;
    }
    if ( tmp_string_buffer[0] == 'c' || tmp_string_buffer[0] == 'a' ){
        if ( tachi_image_name[1] ) delete[] tachi_image_name[1];
        tachi_image_name[1] = NULL;
        if ( tachi_image_surface[1] ) SDL_FreeSurface( tachi_image_surface[1] );
        tachi_image_surface[1] = NULL;
    }
    if ( tmp_string_buffer[0] == 'r' || tmp_string_buffer[0] == 'a' ){
        if ( tachi_image_name[2] ) delete[] tachi_image_name[2];
        tachi_image_name[2] = NULL;
        if ( tachi_image_surface[2] ) SDL_FreeSurface( tachi_image_surface[2] );
        tachi_image_surface[2] = NULL;
    }

    effect_no = readInt( &p_string_buffer, tmp_string_buffer );
    i = readInt( &p_string_buffer, tmp_string_buffer );
    readStr( &p_string_buffer, tmp_string_buffer );
        
    if ( event_mode & EFFECT_EVENT_MODE ){
        tmp_effect.effect = effect_no;
        tmp_effect.duration = i;
        if ( i != 0 ){
            if ( tmp_effect.image ) delete[] tmp_effect.image;
            tmp_effect.image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( tmp_effect.image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
            ret = doEffect( TMP_EFFECT, NULL, TACHI_EFFECT_IMAGE );
        }
        else{
            ret = doEffect( effect_no, NULL, TACHI_EFFECT_IMAGE );
        }
        delete[] buf;
        return ret;
    }
    else{
        makeEffectStr( buf, effect_no, i, tmp_string_buffer );

        if ( effect_no == 0 ) return setEffect( RET_CONTINUE, buf );
        else                  return setEffect( RET_WAIT_NEXT, buf );
    }
}

int ONScripterLabel::btndefCommand()
{
    //printf("ONScripterLabel::btndefCommand %d\n", event_mode);
    
    char *p_string_buffer;
    
    p_string_buffer = string_buffer + string_buffer_offset + 6; // strlen("btndef") = 6
    readStr( &p_string_buffer, tmp_string_buffer );

    unsigned long length = cBR->getFileLength( tmp_string_buffer );
    unsigned char *buffer = new unsigned char[length];
    cBR->getFile( tmp_string_buffer, buffer );
    if ( btndef_surface ) SDL_FreeSurface( btndef_surface );
    btndef_surface = IMG_Load_RW(SDL_RWFromMem( buffer, length ),1);
    delete[] buffer;
    
    deleteButtonLink();
    
    return RET_CONTINUE;
}

int ONScripterLabel::btnCommand()
{
    int x3, y3;
    
    last_button_link->next = new struct ButtonLink();
    last_button_link = last_button_link->next;
    last_button_link->next = NULL;
    
    char *p_string_buffer;
    
    p_string_buffer = string_buffer + string_buffer_offset + 3; // strlen("btn") = 3
    last_button_link->no = readInt( &p_string_buffer, tmp_string_buffer );
    last_button_link->image_rect.x = readInt( &p_string_buffer, tmp_string_buffer );
    last_button_link->image_rect.y = readInt( &p_string_buffer, tmp_string_buffer );
    last_button_link->image_rect.w = readInt( &p_string_buffer, tmp_string_buffer );
    last_button_link->image_rect.h = readInt( &p_string_buffer, tmp_string_buffer );
    last_button_link->select_rect = last_button_link->image_rect;
    x3 = readInt( &p_string_buffer, tmp_string_buffer );
    y3 = readInt( &p_string_buffer, tmp_string_buffer );
#if 0
    printf("ONScripterLabel::btnCommand %d,%d,%d,%d,%d,%d,%d\n",
           last_button_link->no,
           last_button_link->x,
           last_button_link->y,
           last_button_link->wx,
           last_button_link->wy,
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
    char *p_string_buffer = string_buffer + string_buffer_offset + 7; // strlen("btnwait") = 7
    
    if ( event_mode & WAIT_BUTTON_MODE ){
        readToken( &p_string_buffer, tmp_string_buffer );
        if ( tmp_string_buffer[0] != '%' ) errorAndExit( string_buffer + string_buffer_offset );

        setNumVariable( atoi( tmp_string_buffer+1 ), current_button_state.button );

        if ( current_button_state.button > 0 ) deleteButtonLink();

        /* ---------------------------------------- */
        /* fill the button image */
        if ( current_over_button != 0 ){
            SDL_BlitSurface( background_surface, &current_over_button_link.image_rect, accumulation_surface, &current_over_button_link.image_rect );
            SDL_BlitSurface( accumulation_surface, &current_over_button_link.image_rect, text_surface, &current_over_button_link.image_rect );

            flush( current_over_button_link.image_rect.x, current_over_button_link.image_rect.y, current_over_button_link.image_rect.w, current_over_button_link.image_rect.h );
            current_over_button = 0;
        }
            
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        event_mode = WAIT_BUTTON_MODE;
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

    dst_rect.x = readInt( &p_string_buffer, tmp_string_buffer );
    dst_rect.y = readInt( &p_string_buffer, tmp_string_buffer );
    dst_rect.w = readInt( &p_string_buffer, tmp_string_buffer );
    dst_rect.h = readInt( &p_string_buffer, tmp_string_buffer );
    src_rect.x = readInt( &p_string_buffer, tmp_string_buffer );
    src_rect.y = readInt( &p_string_buffer, tmp_string_buffer );
    src_rect.w = readInt( &p_string_buffer, tmp_string_buffer );
    src_rect.h = readInt( &p_string_buffer, tmp_string_buffer );
    
    SDL_BlitSurface( btndef_surface, &src_rect, text_surface, &dst_rect );
    flush( dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h );
    
    return RET_CONTINUE;
}

int ONScripterLabel::bgCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 2; // strlen("bg") = 2
    char *buf;
    int effect_no, ret, i;
    
    readStr( &p_string_buffer, tmp_string_buffer );
    //buf = new char[ 3 + strlen( tmp_string_buffer ) + 1 ];
    buf = new char[ 512 ];
    sprintf( buf, "bg \"%s\",",tmp_string_buffer );

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
        
        effect_no = readInt( &p_string_buffer, tmp_string_buffer );
        i = readInt( &p_string_buffer, tmp_string_buffer );
        if ( i != 0 ){
            tmp_effect.effect = effect_no;
            tmp_effect.duration = i;
            readStr( &p_string_buffer, tmp_string_buffer );
            if ( tmp_effect.image ) delete[] tmp_effect.image;
            tmp_effect.image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( tmp_effect.image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
            
            ret = doEffect( TMP_EFFECT, &bg_image_tag, bg_effect_image );
        }
        else{
            ret = doEffect( effect_no, &bg_image_tag, bg_effect_image );
        }

        delete[] buf;
        return ret;
    }
    else{
        /* ---------------------------------------- */
        /* Delete all tachi iamges if exist */
        for ( i=0 ; i<3 ; i++ ){
            if ( tachi_image_name[i] ){
                delete[] tachi_image_name[i];
                tachi_image_name[i] = NULL;
            }
            if ( tachi_image_surface[i] ) SDL_FreeSurface( tachi_image_surface[i] );
            tachi_image_surface[i] = NULL;
        }

        effect_no = readInt( &p_string_buffer, tmp_string_buffer );
        i = readInt( &p_string_buffer, tmp_string_buffer );
        readStr( &p_string_buffer, tmp_string_buffer );
        makeEffectStr( buf, effect_no, i, tmp_string_buffer );

        if ( effect_no == 0 ) return setEffect( RET_CONTINUE, buf );
        else{
            return setEffect( RET_WAIT_NEXT, buf );
        }
    }
    
    return RET_CONTINUE;
}

int ONScripterLabel::autoclickCommand()
{
    char *p_string_buffer = string_buffer + string_buffer_offset + 9; // strlen("autoclick") = 9
    autoclick_timer = readInt( &p_string_buffer, tmp_string_buffer );

    printf("autoclickCommand %ld\n", autoclick_timer );
    
    return RET_CONTINUE;
}

int ONScripterLabel::amspCommand()
{
    int no;
    char *p_string_buffer;

    p_string_buffer = string_buffer + string_buffer_offset + 4; // strlen("amsp") = 4;

    no = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].x = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].y = readInt( &p_string_buffer, tmp_string_buffer );
    sprite_info[ no ].trans = readInt( &p_string_buffer, tmp_string_buffer );
    if ( sprite_info[ no ].trans > 255 ) sprite_info[ no ].trans = 255;
    else if ( sprite_info[ no ].trans < 0 ) sprite_info[ no ].trans = 0;

    return RET_CONTINUE;
}

