/* -*- C++ -*-
 * 
 *  ONScripterLabel_event.cpp - Event handler of ONScripter
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
#if defined(LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define ONS_TIMER_EVENT   (SDL_USEREVENT)
#define ONS_SOUND_EVENT   (SDL_USEREVENT+1)
#define ONS_CDAUDIO_EVENT (SDL_USEREVENT+2)
#define ONS_MIDI_EVENT    (SDL_USEREVENT+3)
#if defined(EXTERNAL_MUSIC_PLAYER)
#define ONS_MUSIC_EVENT   (SDL_USEREVENT+4)
#endif

#define EDIT_MODE_PREFIX "[EDIT MODE]  "
#define EDIT_SELECT_STRING "MP3 vol (m)  SE vol (s)  Voice vol (v)  Numeric variable (n)"

static SDL_TimerID timer_id = NULL;
SDL_TimerID timer_cdaudio_id = NULL;
bool midi_play_once_flag = false;
#if defined(EXTERNAL_MUSIC_PLAYER)
bool ext_music_play_once_flag = false;
#endif

/* **************************************** *
 * Callback functions
 * **************************************** */
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

Uint32 cdaudioCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_cdaudio_id );
    timer_cdaudio_id = NULL;

    SDL_Event event;
    event.type = ONS_CDAUDIO_EVENT;
    SDL_PushEvent( &event );

    return interval;
}

void ONScripterLabel::flushEvent()
{
    SDL_Event event;
    while( SDL_PollEvent( &event ) );
}

void ONScripterLabel::startTimer( int count )
{
    int duration = proceedAnimation();

    if ( duration > 0 && duration < count ){
        resetRemainingTime( duration );
        advancePhase( duration );
        event_mode |= WAIT_ANIMATION_MODE;
        remaining_time = count;
    }
    else{
        advancePhase( count );
    }
}

void ONScripterLabel::advancePhase( int count )
{
    if ( timer_id != NULL ){
        SDL_RemoveTimer( timer_id );
    }

    if ( count > 0 ){
        timer_id = SDL_AddTimer( count, timerCallback, NULL );
    }
    else{
        SDL_Event event;
        event.type = ONS_TIMER_EVENT;
        SDL_PushEvent( &event );
    }
}

void midiCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif
    if ( !midi_play_once_flag ){
        SDL_Event event;
        event.type = ONS_MIDI_EVENT;
        SDL_PushEvent(&event);
    }
}

#if defined(EXTERNAL_MUSIC_PLAYER)
void musicCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif

    if ( !ext_music_play_once_flag ){
        SDL_Event event;
        event.type = ONS_MUSIC_EVENT;
        SDL_PushEvent(&event);
    }
}
#endif

void ONScripterLabel::trapHandler()
{
    trap_flag = false;
    current_link_label_info->label_info = script_h.lookupLabel( trap_dist );
    current_link_label_info->current_line = 0;
    script_h.setCurrent( current_link_label_info->label_info.start_address );
    string_buffer_offset = 0;
    stopAnimation( clickstr_state );
    event_mode = IDLE_EVENT_MODE;
    advancePhase();
}

/* **************************************** *
 * Event handlers
 * **************************************** */
void ONScripterLabel::mouseMoveEvent( SDL_MouseMotionEvent *event )
{
    if      ( mouse_rotation_mode == MOUSE_ROTATION_NONE )
        mouseOverCheck( event->x, event->y );
    else if ( mouse_rotation_mode == MOUSE_ROTATION_PDA ||
              mouse_rotation_mode == MOUSE_ROTATION_PDA_VGA )
        mouseOverCheck( event->y, screen_height - event->x - 1 );
}

void ONScripterLabel::mousePressEvent( SDL_MouseButtonEvent *event )
{
    if ( variable_edit_mode ) return;
    
    if      ( mouse_rotation_mode == MOUSE_ROTATION_NONE ){
        current_button_state.x = event->x;
        current_button_state.y = event->y;
    }
    else if ( mouse_rotation_mode == MOUSE_ROTATION_PDA ||
              mouse_rotation_mode == MOUSE_ROTATION_PDA_VGA ){
        current_button_state.x = event->y;
        current_button_state.y = screen_height - event->x - 1;
    }
    current_button_state.down_flag = false;
    
    if ( event->button == SDL_BUTTON_RIGHT &&
         event->type == SDL_MOUSEBUTTONUP &&
         ( rmode_flag || (event_mode & WAIT_BUTTON_MODE) ) ) {
        current_button_state.button = -1;
        volatile_button_state.button = -1;
    }
    else if ( event->button == SDL_BUTTON_LEFT &&
              ( event->type == SDL_MOUSEBUTTONUP || btndown_flag ) ){
        current_button_state.button = current_over_button;
        volatile_button_state.button = current_over_button;
        if ( event->type == SDL_MOUSEBUTTONUP )
                current_button_state.down_flag = true;
        if ( trap_flag ){
            trapHandler();
            return;
        }
    }
#if SDL_VERSION_ATLEAST(1, 2, 5)
    else if ( event->button == SDL_BUTTON_WHEELUP && usewheel_flag ){
        current_button_state.button = -2;
        volatile_button_state.button = -2;
    }
    else if ( event->button == SDL_BUTTON_WHEELDOWN && usewheel_flag ){
        current_button_state.button = -3;
        volatile_button_state.button = -3;
    }
#endif
    else return;
    
    if ( skip_flag ) skip_flag = false;

    if ( ( event_mode & WAIT_INPUT_MODE ) &&
         ( autoclick_timer == 0 || (event_mode & WAIT_BUTTON_MODE) ) &&
         volatile_button_state.button == -1 && 
         root_menu_link.next ){
        system_menu_mode = SYSTEM_MENU;
    }
    
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) &&
         ( autoclick_timer == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        playClickVoice();
        stopAnimation( clickstr_state );
        advancePhase();
    }
}

void ONScripterLabel::variableEditMode( SDL_KeyboardEvent *event )
{
    int  i;
    char *var_name, var_index[12];

    switch ( event->keysym.sym ) {
      case SDLK_m:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_MP3_VOLUME_MODE;
        variable_edit_num = mp3_volume;
        break;

      case SDLK_s:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_SE_VOLUME_MODE;
        variable_edit_num = se_volume;
        break;

      case SDLK_v:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VOICE_VOLUME_MODE;
        variable_edit_num = voice_volume;
        break;

      case SDLK_n:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VARIABLE_INDEX_MODE;
        variable_edit_num = 0;
        break;

      case SDLK_9: case SDLK_KP9: variable_edit_num = variable_edit_num * 10 + 9; break;
      case SDLK_8: case SDLK_KP8: variable_edit_num = variable_edit_num * 10 + 8; break;
      case SDLK_7: case SDLK_KP7: variable_edit_num = variable_edit_num * 10 + 7; break;
      case SDLK_6: case SDLK_KP6: variable_edit_num = variable_edit_num * 10 + 6; break;
      case SDLK_5: case SDLK_KP5: variable_edit_num = variable_edit_num * 10 + 5; break;
      case SDLK_4: case SDLK_KP4: variable_edit_num = variable_edit_num * 10 + 4; break;
      case SDLK_3: case SDLK_KP3: variable_edit_num = variable_edit_num * 10 + 3; break;
      case SDLK_2: case SDLK_KP2: variable_edit_num = variable_edit_num * 10 + 2; break;
      case SDLK_1: case SDLK_KP1: variable_edit_num = variable_edit_num * 10 + 1; break;
      case SDLK_0: case SDLK_KP0: variable_edit_num = variable_edit_num * 10 + 0; break;

      case SDLK_MINUS: case SDLK_KP_MINUS:
        if ( variable_edit_mode == EDIT_VARIABLE_NUM_MODE && variable_edit_num == 0 ) variable_edit_sign = -1;
        break;

      case SDLK_BACKSPACE:
        if ( variable_edit_num ) variable_edit_num /= 10;
        else if ( variable_edit_sign == -1 ) variable_edit_sign = 1;
        break;

      case SDLK_RETURN: case SDLK_KP_ENTER:
        switch( variable_edit_mode ){

          case EDIT_VARIABLE_INDEX_MODE:
            variable_edit_index = variable_edit_num;
            variable_edit_num = script_h.num_variables[ variable_edit_index ];
            if ( variable_edit_num < 0 ){
                variable_edit_num = -variable_edit_num;
                variable_edit_sign = -1;
            }
            else{
                variable_edit_sign = 1;
            }
            break;

          case EDIT_VARIABLE_NUM_MODE:
            script_h.setNumVariable( variable_edit_index, variable_edit_sign * variable_edit_num );
            break;

          case EDIT_MP3_VOLUME_MODE:
            mp3_volume = variable_edit_num;
            if ( mp3_sample ) SMPEG_setvolume( mp3_sample, mp3_volume );
            break;

          case EDIT_SE_VOLUME_MODE:
            se_volume = variable_edit_num;
            for ( i=1 ; i<ONS_MIX_CHANNELS ; i++ )
                if ( wave_sample[i] ) Mix_Volume( i, se_volume * 128 / 100 );
            break;

          case EDIT_VOICE_VOLUME_MODE:
            voice_volume = variable_edit_num;
            if ( wave_sample[0] ) Mix_Volume( 0, se_volume * 128 / 100 );

          default:
            break;
        }
        if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE )
            variable_edit_mode = EDIT_VARIABLE_NUM_MODE;
        else
            variable_edit_mode = EDIT_SELECT_MODE;
        break;

      case SDLK_ESCAPE:
        if ( variable_edit_mode == EDIT_SELECT_MODE ){
            variable_edit_mode = NOT_EDIT_MODE;
            SDL_WM_SetCaption( DEFAULT_WM_TITLE, DEFAULT_WM_ICON );
            SDL_Delay( 100 );
            SDL_WM_SetCaption( wm_title_string, wm_icon_string );
            return;
        }
        variable_edit_mode = EDIT_SELECT_MODE;

      default:
        break;
    }

    if ( variable_edit_mode == EDIT_SELECT_MODE ){
        sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
    }
    else if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE ) {
        sprintf( wm_edit_string, "%s%s%d", EDIT_MODE_PREFIX, "Variable Index?  %", variable_edit_sign * variable_edit_num );
    }
    else if ( variable_edit_mode >= EDIT_VARIABLE_NUM_MODE ){
        int p=0;
        
        switch( variable_edit_mode ){

          case EDIT_VARIABLE_NUM_MODE:
            sprintf( var_index, "%%%d", variable_edit_index );
            var_name = var_index; p = script_h.num_variables[ variable_edit_index ]; break;

          case EDIT_MP3_VOLUME_MODE:
            var_name = "MP3 Volume"; p = mp3_volume; break;

          case EDIT_VOICE_VOLUME_MODE:
            var_name = "Voice Volume"; p = voice_volume; break;

          case EDIT_SE_VOLUME_MODE:
            var_name = "Sound effect Volume"; p = se_volume; break;

          default:
            var_name = "";
        }
        sprintf( wm_edit_string, "%sCurrent %s=%d  New value? %s%d",
                 EDIT_MODE_PREFIX, var_name, p, (variable_edit_sign==1)?"":"-", variable_edit_num );
    }

    SDL_WM_SetCaption( wm_edit_string, wm_icon_string );
}

void ONScripterLabel::shiftCursorOnButton( int diff )
{
    int i;
    
    shortcut_mouse_line += diff;
    if ( shortcut_mouse_line < 0 ) shortcut_mouse_line = 0;

    ButtonLink *p_button_link = root_button_link.next;
    
    for ( i=0 ; i<shortcut_mouse_line && p_button_link ; i++ ) 
        p_button_link  = p_button_link->next;
    
    if ( !p_button_link ){
        if ( diff == -1 )
            shortcut_mouse_line = 0;
        else
            shortcut_mouse_line = i-1;

        p_button_link = root_button_link.next;
        for ( i=0 ; i<shortcut_mouse_line ; i++ ) 
            p_button_link  = p_button_link->next;
    }
    if ( p_button_link ){
        if ( mouse_rotation_mode == MOUSE_ROTATION_NONE ||
             mouse_rotation_mode == MOUSE_ROTATION_PDA_VGA ){
            SDL_WarpMouse( p_button_link->select_rect.x + p_button_link->select_rect.w / 2,
                           p_button_link->select_rect.y + p_button_link->select_rect.h / 2 );
        }
        else if ( mouse_rotation_mode == MOUSE_ROTATION_PDA ){
            SDL_WarpMouse( screen_height - (p_button_link->select_rect.y + p_button_link->select_rect.h / 2) - 1,
                           p_button_link->select_rect.x + p_button_link->select_rect.w / 2);
        }
    }
}

void ONScripterLabel::keyDownEvent( SDL_KeyboardEvent *event )
{
    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  |= 0x01;
        goto ctrl_pressed;
      case SDLK_LCTRL:
        ctrl_pressed_status  |= 0x02;
        goto ctrl_pressed;
      case SDLK_RSHIFT:
        shift_pressed_status |= 0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status |= 0x02;
        break;
      default:
        break;
    }

    return;

  ctrl_pressed:
    current_button_state.button  = 0;
    volatile_button_state.button = 0;
    playClickVoice();
    stopAnimation( clickstr_state );
    advancePhase();
    return;
}

void ONScripterLabel::keyUpEvent( SDL_KeyboardEvent *event )
{
    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  &= ~0x01;
        break;
      case SDLK_LCTRL:
        ctrl_pressed_status  &= ~0x02;
        break;
      case SDLK_RSHIFT:
        shift_pressed_status &= ~0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status &= ~0x02;
        break;
      default:
        break;
    }
}

void ONScripterLabel::keyPressEvent( SDL_KeyboardEvent *event )
{
    current_button_state.button = 0;
    
    if ( event->type == SDL_KEYUP ){
        if ( variable_edit_mode ){
            variableEditMode( event );
            return;
        }

        if ( edit_flag && event->keysym.sym == SDLK_z ){
            variable_edit_mode = EDIT_SELECT_MODE;
            variable_edit_sign = 1;
            variable_edit_num = 0;
            sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
            SDL_WM_SetCaption( wm_edit_string, wm_icon_string );
        }

        if ( skip_flag &&
             ( ( !getcursor_flag && event->keysym.sym == SDLK_LEFT ) ||
               event->keysym.sym == SDLK_s ) )
            skip_flag = false;
    }
    if ( shift_pressed_status && event->keysym.sym == SDLK_q && current_mode == NORMAL_MODE ){
        endCommand();
    }

    if ( trap_flag && (event->keysym.sym == SDLK_RETURN ||
                       event->keysym.sym == SDLK_SPACE ) ){
        trapHandler();
        return;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE &&
         ( event->type == SDL_KEYUP ||
           ( btndown_flag && event->keysym.sym == SDLK_RETURN) ) ){
        if ( ( !getcursor_flag && event->keysym.sym == SDLK_UP ) ||
             event->keysym.sym == SDLK_p ){

            shiftCursorOnButton( -1 );
            return;
        }
        else if ( ( !getcursor_flag && event->keysym.sym == SDLK_DOWN ) ||
                  event->keysym.sym == SDLK_n ){

            shiftCursorOnButton( 1 );
            return;
        }
        else if ( ( !getenter_flag  && event->keysym.sym == SDLK_RETURN ) ||
                  ( (spclclk_flag || !useescspc_flag) && event->keysym.sym == SDLK_SPACE  ) ){
            if ( event->keysym.sym == SDLK_RETURN ||
                 spclclk_flag && event->keysym.sym == SDLK_SPACE ){
                current_button_state.button = current_over_button;
                volatile_button_state.button = current_over_button;
                if ( event->type == SDL_KEYDOWN )
                    current_button_state.down_flag = true;
                else
                    current_button_state.down_flag = false;
            }
            else{
                current_button_state.button = 0;
                volatile_button_state.button = 0;
            }
            playClickVoice();
            stopAnimation( clickstr_state );
            advancePhase();
            return;
        }
    }

    if ( event->type == SDL_KEYDOWN ) return;
    
    if ( ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ) &&
         ( autoclick_timer == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if ( !useescspc_flag && event->keysym.sym == SDLK_ESCAPE && rmode_flag ){
            current_button_state.button  = -1;
            if ( event_mode & WAIT_INPUT_MODE &&
                 root_menu_link.next ){
                system_menu_mode = SYSTEM_MENU;
            }
        }
        else if ( useescspc_flag && event->keysym.sym == SDLK_ESCAPE ){
            current_button_state.button  = -10;
        }
        else if ( !spclclk_flag && useescspc_flag && event->keysym.sym == SDLK_SPACE ){
            current_button_state.button  = -11;
        }
        else if ( getpageup_flag && event->keysym.sym == SDLK_PAGEUP ){
            current_button_state.button  = -12;
        }
        else if ( getenter_flag && event->keysym.sym == SDLK_RETURN ){
            current_button_state.button  = -19;
        }
        else if ( gettab_flag && event->keysym.sym == SDLK_TAB ){
            current_button_state.button  = -20;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_UP ){
            current_button_state.button  = -40;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_RIGHT ){
            current_button_state.button  = -41;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_DOWN ){
            current_button_state.button  = -42;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_LEFT ){
            current_button_state.button  = -43;
        }
        else if ( getfunction_flag ){
            if      ( event->keysym.sym == SDLK_F1 )
                current_button_state.button = -21;
            else if ( event->keysym.sym == SDLK_F2 )
                current_button_state.button = -22;
            else if ( event->keysym.sym == SDLK_F3 )
                current_button_state.button = -23;
            else if ( event->keysym.sym == SDLK_F4 )
                current_button_state.button = -24;
            else if ( event->keysym.sym == SDLK_F5 )
                current_button_state.button = -25;
            else if ( event->keysym.sym == SDLK_F6 )
                current_button_state.button = -26;
            else if ( event->keysym.sym == SDLK_F7 )
                current_button_state.button = -27;
            else if ( event->keysym.sym == SDLK_F8 )
                current_button_state.button = -28;
            else if ( event->keysym.sym == SDLK_F9 )
                current_button_state.button = -29;
            else if ( event->keysym.sym == SDLK_F10 )
                current_button_state.button = -30;
            else if ( event->keysym.sym == SDLK_F11 )
                current_button_state.button = -31;
            else if ( event->keysym.sym == SDLK_F12 )
                current_button_state.button = -32;
        }
        if ( current_button_state.button != 0 ){
            volatile_button_state.button = current_button_state.button;
            stopAnimation( clickstr_state );
            advancePhase();
            return;
        }
    }
    
    if ( event_mode & WAIT_INPUT_MODE && !key_pressed_flag &&
         ( autoclick_timer == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if (event->keysym.sym == SDLK_RETURN || 
            event->keysym.sym == SDLK_SPACE ){
            skip_flag = false;
            key_pressed_flag = true;
            playClickVoice();
            stopAnimation( clickstr_state );
            advancePhase();
        }
    }
    
    if ( event_mode & ( WAIT_INPUT_MODE | WAIT_TEXTBTN_MODE ) && 
         !key_pressed_flag ){
        if ( event->keysym.sym == SDLK_LEFT || event->keysym.sym == SDLK_s ){
            skip_flag = true;
            printf("toggle skip to true\n");
            key_pressed_flag = true;
            stopAnimation( clickstr_state );
            if ( (event_mode & (WAIT_BUTTON_MODE | WAIT_TEXTBTN_MODE)) != WAIT_BUTTON_MODE )
                advancePhase();
        }
        else if ( event->keysym.sym == SDLK_RIGHT || event->keysym.sym == SDLK_o ){
            draw_one_page_flag = !draw_one_page_flag;
            printf("toggle draw one page flag to %s\n", (draw_one_page_flag?"true":"false") );
            if ( draw_one_page_flag ){
                stopAnimation( clickstr_state );
                if ( (event_mode & (WAIT_BUTTON_MODE | WAIT_TEXTBTN_MODE)) != WAIT_BUTTON_MODE )
                    advancePhase();
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

    if ( event_mode & ( WAIT_INPUT_MODE | WAIT_BUTTON_MODE ) ){
        if ( event->keysym.sym == SDLK_f ){
            if ( fullscreen_mode ) menu_windowCommand();
            else                   menu_fullCommand();
        }
    }
}

void ONScripterLabel::timerEvent( void )
{
  timerEventTop:

    int ret;

    if ( event_mode & WAIT_ANIMATION_MODE ){
        int duration = proceedAnimation();

        if ( duration == 0 ||
             ( remaining_time > 0 &&
               remaining_time-duration <= 0 ) ){

            if ( remaining_time > 0 ){
                remaining_time = 0;
                advancePhase();
            }

            if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) && 
                 ( clickstr_state == CLICK_WAIT || 
                   clickstr_state == CLICK_NEWPAGE ) ){ 
                playClickVoice(); 
                stopAnimation( clickstr_state ); 
            } 

            event_mode &= ~WAIT_ANIMATION_MODE;
        }
        else{
            if ( remaining_time > 0 )
                remaining_time -= duration;
            resetRemainingTime( duration );
            advancePhase( duration );
        }
    }
    else if ( event_mode & EFFECT_EVENT_MODE ){

        if ( display_mode & TEXT_DISPLAY_MODE &&
             erase_text_window_mode != 0 &&
             strncmp( script_h.getStringBuffer(), "quake", 5 ) ){

            if ( effect_counter == 0 ){
                next_display_mode = NORMAL_DISPLAY_MODE;
                SDL_BlitSurface( accumulation_surface, NULL, effect_dst_surface, NULL );
                dirty_rect_tmp = dirty_rect;
                dirty_rect.clear();
                dirty_rect.add( sentence_font_info.pos );
            }
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode = NORMAL_DISPLAY_MODE;
                effect_counter = 0;
                event_mode = EFFECT_EVENT_MODE;
                dirty_rect = dirty_rect_tmp;
            }
            advancePhase();
            return;
        }

        char *current = script_h.getCurrent();
        ret = this->parseLine();
        
        if ( ret == RET_CONTINUE || ret == RET_CONTINUE_NOREAD){
            if ( ret == RET_CONTINUE ){
                script_h.readToken(); // skip tailing \0 and mark kidoku
                string_buffer_offset = 0;
            }
            if ( effect_blank == 0 || effect_counter == 0 ) goto timerEventTop;
            startTimer( effect_blank );
        }
        else{
            script_h.setCurrent( current );
            advancePhase();
        }
        return;
    }
    else{
        if ( system_menu_mode != SYSTEM_NULL || 
             ( event_mode & WAIT_INPUT_MODE && 
               volatile_button_state.button == -1 ) )
            executeSystemCall();
        else
            executeLabel();
    }
    volatile_button_state.button = 0;
}

/* **************************************** *
 * Event loop
 * **************************************** */
int ONScripterLabel::eventLoop()
{
    SDL_Event event;

    advancePhase();

    while ( SDL_WaitEvent(&event) ) {
        switch (event.type) {
          case SDL_MOUSEMOTION:
            mouseMoveEvent( (SDL_MouseMotionEvent*)&event );
            break;
            
          case SDL_MOUSEBUTTONDOWN:
            if ( !btndown_flag ) break;
          case SDL_MOUSEBUTTONUP:
            mousePressEvent( (SDL_MouseButtonEvent*)&event );
            break;

          case SDL_KEYDOWN:
            keyDownEvent( (SDL_KeyboardEvent*)&event );
            if ( btndown_flag )
                keyPressEvent( (SDL_KeyboardEvent*)&event );
            break;
          case SDL_KEYUP:
            keyUpEvent( (SDL_KeyboardEvent*)&event );
            keyPressEvent( (SDL_KeyboardEvent*)&event );
            break;

          case ONS_TIMER_EVENT:
            timerEvent();
            break;
                
          case ONS_SOUND_EVENT:
            if ( !music_play_once_flag ){
                stopBGM( true );
                playMP3( current_cd_track );
            }
            else{
                stopBGM( false );
            }
            break;
                
          case ONS_CDAUDIO_EVENT:
            if ( !music_play_once_flag ){
                stopBGM( true );
                playCDAudio( current_cd_track );
            }
            else{
                stopBGM( false );
            }
            break;

          case ONS_MIDI_EVENT:
            midi_play_once_flag = music_play_once_flag;
            Mix_FreeMusic( midi_info );
            playMIDI();
            break;

#if defined(EXTERNAL_MUSIC_PLAYER)
          case ONS_MUSIC_EVENT:
            ext_music_play_once_flag = music_play_once_flag;
            Mix_FreeMusic( music_info );
            playMusic();
            break;
#endif

          case SDL_ACTIVEEVENT:
            if ( event.active.gain ){
                SDL_Rect rect;
                rect.x = rect.y = 0;
                rect.w = screen_width;
                rect.h = screen_height;
                flush( &rect, false, true );
            }
            break;
            
          case SDL_QUIT:
            saveAll();
            if ( cdrom_info ){
                SDL_CDStop( cdrom_info );
                SDL_CDClose( cdrom_info );
            }
            if ( midi_info ){
                Mix_HaltMusic();
                Mix_FreeMusic( midi_info );
            }
            return(0);
            
          default:
            break;
        }
    }
    return -1;
}


