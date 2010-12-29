/* -*- C++ -*-
 * 
 *  ONScripterLabel_event.cpp - Event handler of ONScripter
 *
 *  Copyright (c) 2001-2010 Ogapee. All rights reserved.
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
#define ONS_MUSIC_EVENT   (SDL_USEREVENT+1)
#define ONS_CDAUDIO_EVENT (SDL_USEREVENT+2)
#define ONS_MIDI_EVENT    (SDL_USEREVENT+3)
#define ONS_CHUNK_EVENT   (SDL_USEREVENT+4)
#define ONS_BREAK_EVENT   (SDL_USEREVENT+5)

#define EDIT_MODE_PREFIX "[EDIT MODE]  "
#define EDIT_SELECT_STRING "MP3 vol (m)  SE vol (s)  Voice vol (v)  Numeric variable (n)"

static SDL_TimerID timer_id = NULL;
static SDL_TimerID break_id = NULL;
SDL_TimerID timer_cdaudio_id = NULL;
bool ext_music_play_once_flag = false;

/* **************************************** *
 * Callback functions
 * **************************************** */
extern "C" void musicFinishCallback()
{
    SDL_Event event;
    event.type = ONS_MUSIC_EVENT;
    SDL_PushEvent(&event);
}

extern "C" Uint32 SDLCALL breakCallback(Uint32 interval, void *param)
{
    SDL_RemoveTimer(break_id);
    break_id = NULL;

	SDL_Event event;
	event.type = ONS_BREAK_EVENT;
	SDL_PushEvent(&event);

    return interval;
}

extern "C" Uint32 SDLCALL timerCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_id );
    timer_id = NULL;

	SDL_Event event;
	event.type = ONS_TIMER_EVENT;
	SDL_PushEvent( &event );

    return interval;
}

extern "C" Uint32 cdaudioCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_cdaudio_id );
    timer_cdaudio_id = NULL;

    SDL_Event event;
    event.type = ONS_CDAUDIO_EVENT;
    SDL_PushEvent( &event );

    return interval;
}

/* **************************************** *
 * OS Dependent Input Translation
 * **************************************** */

SDLKey transKey(SDLKey key)
{
#if defined(IPODLINUX)
 	switch(key){
      case SDLK_m:      key = SDLK_UP;      break; /* Menu                   */
      case SDLK_d:      key = SDLK_DOWN;    break; /* Play/Pause             */
      case SDLK_f:      key = SDLK_RIGHT;   break; /* Fast forward           */
      case SDLK_w:      key = SDLK_LEFT;    break; /* Rewind                 */
      case SDLK_RETURN: key = SDLK_RETURN;  break; /* Action                 */
      case SDLK_h:      key = SDLK_ESCAPE;  break; /* Hold                   */
      case SDLK_r:      key = SDLK_UNKNOWN; break; /* Wheel clockwise        */
      case SDLK_l:      key = SDLK_UNKNOWN; break; /* Wheel counterclockwise */
      default: break;
    }
#endif
#if defined(WINCE)
    switch(key){
      case SDLK_UP:     key = SDLK_UP;      break; /* vkUP    */
      case SDLK_DOWN:   key = SDLK_DOWN;    break; /* vkDOWN  */
      case SDLK_LEFT:   key = SDLK_LCTRL;   break; /* vkLEFT  */
      case SDLK_RIGHT:  key = SDLK_RETURN;  break; /* vkRIGHT */
      case SDLK_KP0:    key = SDLK_q;       break; /* vkStart */
      case SDLK_KP1:    key = SDLK_RETURN;  break; /* vkA     */
      case SDLK_KP2:    key = SDLK_SPACE;   break; /* vkB     */
      case SDLK_KP3:    key = SDLK_ESCAPE;  break; /* vkC     */
      default: break;
    }
#endif
    return key;
}

SDLKey transJoystickButton(Uint8 button)
{
#if defined(PSP)    
    SDLKey button_map[] = { SDLK_ESCAPE, /* TRIANGLE */
                            SDLK_RETURN, /* CIRCLE   */
                            SDLK_SPACE,  /* CROSS    */
                            SDLK_RCTRL,  /* SQUARE   */
                            SDLK_o,      /* LTRIGGER */
                            SDLK_s,      /* RTRIGGER */
                            SDLK_DOWN,   /* DOWN     */
                            SDLK_LEFT,   /* LEFT     */
                            SDLK_UP,     /* UP       */
                            SDLK_RIGHT,  /* RIGHT    */
                            SDLK_0,      /* SELECT   */
                            SDLK_a,      /* START    */
                            SDLK_UNKNOWN,/* HOME     */ /* kernel mode only */
                            SDLK_UNKNOWN,/* HOLD     */};
    return button_map[button];
#elif defined(__PS3__)    
    SDLKey button_map[] = {
        SDLK_0,      /* SELECT   */
        SDLK_UNKNOWN,/* L3       */
        SDLK_UNKNOWN,/* R3       */
        SDLK_a,      /* START    */
        SDLK_UP,     /* UP       */
        SDLK_RIGHT,  /* RIGHT    */
        SDLK_DOWN,   /* DOWN     */
        SDLK_LEFT,   /* LEFT     */
        SDLK_SPACE,  /* L2       */
        SDLK_RETURN, /* R2       */
        SDLK_o,      /* L1       */
        SDLK_s,      /* R1       */
        SDLK_ESCAPE, /* TRIANGLE */
        SDLK_RETURN, /* CIRCLE   */
        SDLK_SPACE,  /* CROSS    */
        SDLK_RCTRL,  /* SQUARE   */
        SDLK_UNKNOWN,/* PS       */
        SDLK_UNKNOWN,
        SDLK_UNKNOWN,
    };
    return button_map[button];
#elif defined(GP2X)
    SDLKey button_map[] = {
        SDLK_UP,      /* UP        */
        SDLK_UNKNOWN, /* UPLEFT    */
        SDLK_LEFT,    /* LEFT      */
        SDLK_UNKNOWN, /* DOWNLEFT  */
        SDLK_DOWN,    /* DOWN      */
        SDLK_UNKNOWN, /* DOWNRIGHT */
        SDLK_RIGHT,   /* RIGHT     */
        SDLK_UNKNOWN, /* UPRIGHT   */
        SDLK_a,       /* START     */
        SDLK_0,       /* SELECT    */
        SDLK_o,       /* L         */
        SDLK_s,       /* R         */
        SDLK_RCTRL,   /* A         */
        SDLK_RETURN,  /* B         */
        SDLK_SPACE,   /* X         */
        SDLK_ESCAPE,  /* Y         */
        SDLK_UNKNOWN, /* VOLUP     */
        SDLK_UNKNOWN, /* VOLDOWN   */
        SDLK_UNKNOWN, /* STICK     */
    };
    return button_map[button];
#endif
    return SDLK_UNKNOWN;
}

SDL_KeyboardEvent transJoystickAxis(SDL_JoyAxisEvent &jaxis)
{
    static int old_axis=-1;

    SDL_KeyboardEvent event;

    SDLKey axis_map[] = {SDLK_LEFT,  /* AL-LEFT  */
                         SDLK_RIGHT, /* AL-RIGHT */
                         SDLK_UP,    /* AL-UP    */
                         SDLK_DOWN   /* AL-DOWN  */};

    int axis = -1;
    /* rerofumi: Jan.15.2007 */
    /* ps3's pad has 0x1b axis (with analog button) */
    if (jaxis.axis < 2){
        axis = ((3200 > jaxis.value) && (jaxis.value > -3200) ? -1 :
                (jaxis.axis * 2 + (jaxis.value>0 ? 1 : 0) ));
    }

    if (axis != old_axis){
        if (axis == -1){
            event.type = SDL_KEYUP;
            event.keysym.sym = axis_map[old_axis];
        }
        else{
            event.type = SDL_KEYDOWN;
            event.keysym.sym = axis_map[axis];
        }
        old_axis = axis;
    }
    else{
        event.keysym.sym = SDLK_UNKNOWN;
    }
    
    return event;
}

void ONScripterLabel::flushEventSub( SDL_Event &event )
{
    if ( event.type == ONS_MUSIC_EVENT ){
        if ( music_play_loop_flag ||
             (cd_play_loop_flag && !cdaudio_flag ) ){
            stopBGM( true );
            if (music_file_name)
                playSound(music_file_name, SOUND_MUSIC, true);
            else
                playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }
    else if ( event.type == ONS_CDAUDIO_EVENT ){
        if ( cd_play_loop_flag ){
            stopBGM( true );
            playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }
    else if ( event.type == ONS_MIDI_EVENT ){
        ext_music_play_once_flag = !midi_play_loop_flag;
        Mix_FreeMusic( midi_info );
        playMIDI(midi_play_loop_flag);
    }
    else if ( event.type == ONS_CHUNK_EVENT ){ // for processing btntim2 and automode correctly
        if ( wave_sample[event.user.code] ){
            Mix_FreeChunk( wave_sample[event.user.code] );
            wave_sample[event.user.code] = NULL;
            if (event.user.code == MIX_LOOPBGM_CHANNEL0 && 
                loop_bgm_name[1] &&
                wave_sample[MIX_LOOPBGM_CHANNEL1])
                Mix_PlayChannel(MIX_LOOPBGM_CHANNEL1, 
                                wave_sample[MIX_LOOPBGM_CHANNEL1], -1);
        }
    }
}

void ONScripterLabel::flushEvent()
{
    SDL_Event event;
    while( SDL_PollEvent( &event ) )
        flushEventSub( event );
}

void ONScripterLabel::advancePhase( int count )
{
    if ( timer_id != NULL ){
        SDL_RemoveTimer( timer_id );
        timer_id = NULL;
    }

    timer_id = SDL_AddTimer( count, timerCallback, NULL );
}

bool ONScripterLabel::waitEventSub(int count)
{
    if (break_id != NULL) // already in wait queue
        return false;

    if (count != 0){
        timerEvent(count);
            
        if (count > 0)
            break_id = SDL_AddTimer(count, breakCallback, NULL);
    }
    
    if (count >= 0 && break_id == NULL){
        SDL_Event event;
        event.type = ONS_BREAK_EVENT;
        SDL_PushEvent( &event );
    }
 
    bool ret = runEventLoop();
    
    if (break_id) SDL_RemoveTimer(break_id);
    break_id = NULL;

    return ret; // true if interrupted
}

bool ONScripterLabel::waitEvent( int count, bool check_interruption )
{
    while(1){
        if (waitEventSub( count ) && check_interruption) return true;
        if ( system_menu_mode == SYSTEM_NULL ) break;
        if ( executeSystemCall() ) return false;
    }

    return false;
}

void midiCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif
    if ( !ext_music_play_once_flag ){
        SDL_Event event;
        event.type = ONS_MIDI_EVENT;
        SDL_PushEvent(&event);
    }
}

extern "C" void waveCallback( int channel )
{
    SDL_Event event;
    event.type = ONS_CHUNK_EVENT;
    event.user.code = channel;
    SDL_PushEvent(&event);
}

bool ONScripterLabel::trapHandler()
{
    if (trap_mode & TRAP_STOP){
        trap_mode |= TRAP_CLICKED;
        return false;
    }
    
    trap_mode = TRAP_NONE;
    stopAnimation( clickstr_state );
    setCurrentLabel( trap_dist );

    return true;
}

/* **************************************** *
 * Event handlers
 * **************************************** */
void ONScripterLabel::mouseMoveEvent( SDL_MouseMotionEvent *event )
{
    current_button_state.x = event->x;
    current_button_state.y = event->y;

    if ( event_mode & WAIT_BUTTON_MODE )
        mouseOverCheck( current_button_state.x, current_button_state.y );
}

bool ONScripterLabel::mousePressEvent( SDL_MouseButtonEvent *event )
{
    if ( variable_edit_mode ) return false;
    
    if ( automode_flag ){
        automode_flag = false;
        return false;
    }
    if ( (event->button == SDL_BUTTON_RIGHT && trap_mode & TRAP_RIGHT_CLICK) ||
         (event->button == SDL_BUTTON_LEFT  && trap_mode & TRAP_LEFT_CLICK) ){
        if (trapHandler()) return true;
    }

    current_button_state.x = event->x;
    current_button_state.y = event->y;
    current_button_state.down_flag = false;
    skip_mode &= ~SKIP_NORMAL;

    if ( event->button == SDL_BUTTON_RIGHT &&
         event->type == SDL_MOUSEBUTTONUP &&
         ((rmode_flag && event_mode & WAIT_TEXT_MODE) ||
          (event_mode & (WAIT_BUTTON_MODE | WAIT_RCLICK_MODE))) ){
        current_button_state.button = -1;
        if (event_mode & WAIT_TEXT_MODE){
            if (root_rmenu_link.next)
                system_menu_mode = SYSTEM_MENU;
            else
                system_menu_mode = SYSTEM_WINDOWERASE;
        }
    }
    else if ( event->button == SDL_BUTTON_LEFT &&
              ( event->type == SDL_MOUSEBUTTONUP || btndown_flag ) ){
        current_button_state.button = current_over_button;
        if ( event->type == SDL_MOUSEBUTTONDOWN )
            current_button_state.down_flag = true;
    }
#if SDL_VERSION_ATLEAST(1, 2, 5)
    else if (event->button == SDL_BUTTON_WHEELUP &&
             ((event_mode & WAIT_TEXT_MODE) ||
              (usewheel_flag && event_mode & WAIT_BUTTON_MODE) || 
              system_menu_mode == SYSTEM_LOOKBACK)){
        current_button_state.button = -2;
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    }
    else if ( event->button == SDL_BUTTON_WHEELDOWN &&
              ((enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE) ||
               (usewheel_flag && event_mode & WAIT_BUTTON_MODE) || 
               system_menu_mode == SYSTEM_LOOKBACK ) ){
        if (event_mode & WAIT_TEXT_MODE)
            current_button_state.button = 0;
        else
            current_button_state.button = -3;
    }
#endif
    else return false;
    
    if (!(event_mode & (WAIT_BUTTON_MODE | WAIT_TEXT_MODE)))
        skip_mode |= SKIP_TO_EOL;
    playClickVoice();
    stopAnimation( clickstr_state );

    return true;
}

void ONScripterLabel::variableEditMode( SDL_KeyboardEvent *event )
{
    int  i;
    const char *var_name;
    char var_index[12];

    switch ( event->keysym.sym ) {
      case SDLK_m:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_MP3_VOLUME_MODE;
        variable_edit_num = music_volume;
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
            variable_edit_num = script_h.getVariableData(variable_edit_index).num;
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
            music_volume = variable_edit_num;
            Mix_VolumeMusic( music_volume * MIX_MAX_VOLUME / 100 );
            break;

          case EDIT_SE_VOLUME_MODE:
            se_volume = variable_edit_num;
            for ( i=1 ; i<ONS_MIX_CHANNELS ; i++ )
                if ( wave_sample[i] ) Mix_Volume( i, se_volume * MIX_MAX_VOLUME / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL0] ) Mix_Volume( MIX_LOOPBGM_CHANNEL0, se_volume * MIX_MAX_VOLUME / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL1] ) Mix_Volume( MIX_LOOPBGM_CHANNEL1, se_volume * MIX_MAX_VOLUME / 100 );
            break;

          case EDIT_VOICE_VOLUME_MODE:
            voice_volume = variable_edit_num;
            if ( wave_sample[0] ) Mix_Volume( 0, se_volume * MIX_MAX_VOLUME / 100 );

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
            var_name = var_index; p = script_h.getVariableData(variable_edit_index).num; break;

          case EDIT_MP3_VOLUME_MODE:
            var_name = "MP3 Volume"; p = music_volume; break;

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
    int num;
    ButtonLink *button = root_button_link.next;
    for (num=0 ; button ; num++) 
        button = button->next;

    shortcut_mouse_line += diff;
    if      (shortcut_mouse_line < 0)    shortcut_mouse_line = num-1;
    else if (shortcut_mouse_line >= num) shortcut_mouse_line = 0;

    button = root_button_link.next;
    for (int i=0 ; i<shortcut_mouse_line ; i++) 
        button  = button->next;
    
    if (button){
        int x = button->select_rect.x;
        int y = button->select_rect.y;
        if      (x < 0)             x = 0;
        else if (x >= screen_width) x = screen_width-1;
        if      (y < 0)              y = 0;
        else if (y >= screen_height) y = screen_height-1;
        SDL_WarpMouse(x, y);
    }
}

bool ONScripterLabel::keyDownEvent( SDL_KeyboardEvent *event )
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

    return false;

  ctrl_pressed:
    current_button_state.button  = 0;
    playClickVoice();
    stopAnimation( clickstr_state );

    return true;
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

bool ONScripterLabel::keyPressEvent( SDL_KeyboardEvent *event )
{
    current_button_state.button = 0;
    current_button_state.down_flag = false;
    if ( automode_flag ){
        automode_flag = false;
        return false;
    }
    
    if ( event->type == SDL_KEYUP ){
        if ( variable_edit_mode ){
            variableEditMode( event );
            return false;
        }

        if ( edit_flag && event->keysym.sym == SDLK_z ){
            variable_edit_mode = EDIT_SELECT_MODE;
            variable_edit_sign = 1;
            variable_edit_num = 0;
            sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
            SDL_WM_SetCaption( wm_edit_string, wm_icon_string );
        }
    }
    
    if (event->type == SDL_KEYUP && 
        (event->keysym.sym == SDLK_RETURN || 
         event->keysym.sym == SDLK_KP_ENTER ||
         event->keysym.sym == SDLK_SPACE ||
         event->keysym.sym == SDLK_s))
        skip_mode &= ~SKIP_NORMAL;
    
    if ( shift_pressed_status && event->keysym.sym == SDLK_q && current_mode == NORMAL_MODE ){
        endCommand();
    }

    if ( (trap_mode & TRAP_LEFT_CLICK) && 
         (event->keysym.sym == SDLK_RETURN ||
          event->keysym.sym == SDLK_KP_ENTER ||
          event->keysym.sym == SDLK_SPACE ) ){
        if (trapHandler()) return true;
    }
    else if ( (trap_mode & TRAP_RIGHT_CLICK) && 
              (event->keysym.sym == SDLK_ESCAPE) ){
        if (trapHandler()) return true;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE &&
         (((event->type == SDL_KEYUP || btndown_flag) &&
           ((!getenter_flag && event->keysym.sym == SDLK_RETURN) ||
            (!getenter_flag && event->keysym.sym == SDLK_KP_ENTER))) ||
          ((spclclk_flag || !useescspc_flag) && event->keysym.sym == SDLK_SPACE)) ){
        if ( event->keysym.sym == SDLK_RETURN ||
             event->keysym.sym == SDLK_KP_ENTER ||
             (spclclk_flag && event->keysym.sym == SDLK_SPACE) ){
            current_button_state.button = current_over_button;
            if ( event->type == SDL_KEYDOWN )
                current_button_state.down_flag = true;
        }
        else{
            current_button_state.button = 0;
        }
        playClickVoice();
        stopAnimation( clickstr_state );

        return true;
    }

    if ( event->type == SDL_KEYDOWN ) return false;
    
    if ( ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ) &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if ( !useescspc_flag && event->keysym.sym == SDLK_ESCAPE){
            current_button_state.button  = -1;
            if (rmode_flag && event_mode & WAIT_TEXT_MODE){
                if (root_rmenu_link.next)
                    system_menu_mode = SYSTEM_MENU;
                else
                    system_menu_mode = SYSTEM_WINDOWERASE;
            }
        }
        else if ( useescspc_flag && event->keysym.sym == SDLK_ESCAPE ){
            current_button_state.button  = -10;
        }
        else if ( !spclclk_flag && useescspc_flag && event->keysym.sym == SDLK_SPACE ){
            current_button_state.button  = -11;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_LEFT) ||
                  event->keysym.sym == SDLK_h) &&
                 (event_mode & WAIT_TEXT_MODE ||
                  (usewheel_flag && !getcursor_flag && event_mode & WAIT_BUTTON_MODE) || 
                  system_menu_mode == SYSTEM_LOOKBACK)){
            current_button_state.button = -2;
            if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
        }
        else if ((!getcursor_flag && event->keysym.sym == SDLK_RIGHT ||
                  event->keysym.sym == SDLK_l) &&
                 (enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE ||
                  usewheel_flag && event_mode & WAIT_BUTTON_MODE || 
                  system_menu_mode == SYSTEM_LOOKBACK)){
            if (event_mode & WAIT_TEXT_MODE)
                current_button_state.button = 0;
            else
                current_button_state.button = -3;
        }
        else if ((!getcursor_flag && event->keysym.sym == SDLK_UP ||
                  event->keysym.sym == SDLK_k ||
                  event->keysym.sym == SDLK_p) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(1);
            return false;
        }
        else if ((!getcursor_flag && event->keysym.sym == SDLK_DOWN ||
                  event->keysym.sym == SDLK_j ||
                  event->keysym.sym == SDLK_n) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(-1);
            return false;
        }
        else if ( getpageup_flag && event->keysym.sym == SDLK_PAGEUP ){
            current_button_state.button  = -12;
        }
        else if ( getpagedown_flag && event->keysym.sym == SDLK_PAGEDOWN ){
            current_button_state.button  = -13;
        }
        else if ( getenter_flag && event->keysym.sym == SDLK_RETURN ||
                  getenter_flag && event->keysym.sym == SDLK_KP_ENTER ){
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
        else if ( getinsert_flag && event->keysym.sym == SDLK_INSERT ){
            current_button_state.button  = -50;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_z ){
            current_button_state.button  = -51;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_x ){
            current_button_state.button  = -52;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_c ){
            current_button_state.button  = -53;
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
            stopAnimation( clickstr_state );

            return true;
        }
    }

    if ( event_mode & WAIT_INPUT_MODE && !key_pressed_flag &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if (event->keysym.sym == SDLK_RETURN || 
            event->keysym.sym == SDLK_KP_ENTER ||
            event->keysym.sym == SDLK_SPACE ){
            if (!(event_mode & (WAIT_BUTTON_MODE | WAIT_TEXT_MODE)))
                skip_mode |= SKIP_TO_EOL;
            key_pressed_flag = true;
            playClickVoice();
            stopAnimation( clickstr_state );

            return true;
        }
    }
    
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_TEXTBTN_MODE) && 
         !key_pressed_flag ){
        if (event->keysym.sym == SDLK_s && !automode_flag ){
            skip_mode |= SKIP_NORMAL;
            printf("toggle skip to true\n");
            key_pressed_flag = true;
            stopAnimation( clickstr_state );

            return true;
        }
        else if (event->keysym.sym == SDLK_o){
            if (skip_mode & SKIP_TO_EOP)
                skip_mode &= ~SKIP_TO_EOP;
            else
                skip_mode |= SKIP_TO_EOP;
            printf("toggle draw one page flag to %s\n", (skip_mode & SKIP_TO_EOP?"true":"false") );
            if ( skip_mode & SKIP_TO_EOP ){
                stopAnimation( clickstr_state );

                return true;
            }
        }
        else if ( event->keysym.sym == SDLK_a && mode_ext_flag && !automode_flag ){
            automode_flag = true;
            skip_mode &= ~SKIP_NORMAL;
            printf("change to automode\n");
            key_pressed_flag = true;
            stopAnimation( clickstr_state );

            return true;
        }
        else if ( event->keysym.sym == SDLK_0 ){
            if (++text_speed_no > 2) text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_1 ){
            text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_2 ){
            text_speed_no = 1;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_3 ){
            text_speed_no = 2;
            sentence_font.wait_time = -1;
        }
    }

    if ( event_mode & ( WAIT_INPUT_MODE | WAIT_BUTTON_MODE ) ){
        if ( event->keysym.sym == SDLK_f ){
            if ( fullscreen_mode ) menu_windowCommand();
            else                   menu_fullCommand();
        }
    }

    return false;
}

void ONScripterLabel::timerEvent(int count)
{
    if (!(event_mode & WAIT_TIMER_MODE)) return;

    int duration = proceedAnimation();
            
    if (duration > 0){
        if (count == -1 || duration < count){
            resetRemainingTime( duration );
            advancePhase( duration );
        }
        else{
            resetRemainingTime( count );
        }
    }
}

bool ONScripterLabel::runEventLoop()
{
    SDL_Event event, tmp_event;

    while ( SDL_WaitEvent(&event) ) {
        bool ret = false;
        // ignore continous SDL_MOUSEMOTION
        while (event.type == SDL_MOUSEMOTION){
            if ( SDL_PeepEvents( &tmp_event, 1, SDL_PEEKEVENT, SDL_ALLEVENTS ) == 0 ) break;
            if (tmp_event.type != SDL_MOUSEMOTION) break;
            SDL_PeepEvents( &tmp_event, 1, SDL_GETEVENT, SDL_ALLEVENTS );
            event = tmp_event;
        }

        switch (event.type) {
          case SDL_MOUSEMOTION:
            mouseMoveEvent( (SDL_MouseMotionEvent*)&event );
            break;
            
          case SDL_MOUSEBUTTONDOWN:
            if ( !btndown_flag ) break;
          case SDL_MOUSEBUTTONUP:
            ret = mousePressEvent( (SDL_MouseButtonEvent*)&event );
            if (ret) return true;
            break;

          case SDL_JOYBUTTONDOWN:
            event.key.type = SDL_KEYDOWN;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;
            
          case SDL_KEYDOWN:
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            ret = keyDownEvent( (SDL_KeyboardEvent*)&event );
            if ( btndown_flag )
                ret |= keyPressEvent( (SDL_KeyboardEvent*)&event );
            if (ret) return true;
            break;

          case SDL_JOYBUTTONUP:
            event.key.type = SDL_KEYUP;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;
            
          case SDL_KEYUP:
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyUpEvent( (SDL_KeyboardEvent*)&event );
            ret = keyPressEvent( (SDL_KeyboardEvent*)&event );
            if (ret) return true;
            break;

          case SDL_JOYAXISMOTION:
          {
              SDL_KeyboardEvent ke = transJoystickAxis(event.jaxis);
              if (ke.keysym.sym != SDLK_UNKNOWN){
                  if (ke.type == SDL_KEYDOWN){
                      keyDownEvent( &ke );
                      if (btndown_flag)
                          keyPressEvent( &ke );
                  }
                  else if (ke.type == SDL_KEYUP){
                      keyUpEvent( &ke );
                      keyPressEvent( &ke );
                  }
              }
              break;
          }

          case ONS_TIMER_EVENT:
            timerEvent();
            break;

          case ONS_MUSIC_EVENT:
          case ONS_CDAUDIO_EVENT:
          case ONS_MIDI_EVENT:
            flushEventSub( event );
            break;

          case ONS_CHUNK_EVENT:
            flushEventSub( event );
            //printf("ONS_CHUNK_EVENT %d: %x %d %x\n", event.user.code, wave_sample[0], automode_flag, event_mode);
            if ( event.user.code != 0 ||
                 !(event_mode & WAIT_VOICE_MODE) ) break;

            event_mode &= ~WAIT_VOICE_MODE;

          case ONS_BREAK_EVENT:
            if (event_mode & WAIT_VOICE_MODE && wave_sample[0]){
                timerEvent();
                break;
            }

            if (automode_flag || autoclick_time>0)
                current_button_state.button = 0;
            else if ( usewheel_flag )
                current_button_state.button = -5;
            else
                current_button_state.button = -2;

            if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) && 
                ( clickstr_state == CLICK_WAIT || 
                  clickstr_state == CLICK_NEWPAGE ) ){
                playClickVoice(); 
                stopAnimation( clickstr_state ); 
            }

            return false;
            
          case SDL_ACTIVEEVENT:
            if ( !event.active.gain ) break;
          case SDL_VIDEOEXPOSE:
#ifdef ANDROID
            screen_surface = SDL_SetVideoMode( screen_device_width, screen_device_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG );
            // draw buttons
            if (screen_surface->w > screen_width){
                int w = screen_surface->w - screen_width;
                int h = screen_height/3;

                AnimationInfo anim;
                anim.allocImage(w, h);

                SDL_Rect rect_src={0, 0, w, h};
                SDL_Rect rect_dst={screen_width, 0, w, h};

                FontInfo f_info = sentence_font;
                f_info.color[0] = f_info.color[1] = f_info.color[2] = 0x80;
                f_info.top_xy[0] = (w-f_info.font_size_xy[0])/2;
                if (f_info.top_xy[0] < 0) f_info.top_xy[0] = 0;
                f_info.top_xy[1] = (h-f_info.font_size_xy[1])/2;
                if (f_info.top_xy[1] < 0) f_info.top_xy[1] = 0;

                anim.fill(0, 0, 0, 0);
                f_info.clear();
                drawChar( (char*)"Esc", &f_info, false, false, NULL, &anim );
                SDL_BlitSurface( anim.image_surface, &rect_src, screen_surface, &rect_dst );

                anim.fill(0, 0, 0, 0);
                f_info.clear();
                drawChar( (char*)"S", &f_info, false, false, NULL, &anim );
                rect_dst.y += h;
                SDL_BlitSurface( anim.image_surface, &rect_src, screen_surface, &rect_dst );

                anim.fill(0, 0, 0, 0);
                f_info.clear();
                drawChar( (char*)"O", &f_info, false, false, NULL, &anim );
                rect_dst.y += h;
                SDL_BlitSurface( anim.image_surface, &rect_src, screen_surface, &rect_dst );

                SDL_UpdateRect( screen_surface, screen_width, 0, w, screen_height);
            }
            
            repaintCommand();
#else
            SDL_UpdateRect( screen_surface, 0, 0, screen_width, screen_height );
#endif
            break;

          case SDL_QUIT:
            endCommand();
            break;
            
          default:
            break;
        }
    }
}
