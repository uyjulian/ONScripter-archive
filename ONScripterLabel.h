/* -*- C++ -*-
 * 
 *  ONScripterLabel.h - Execution block analyzer of ONScripter
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

#ifndef __ONSCRIPTER_LABEL_H__
#define __ONSCRIPTER_LABEL_H__

#include "ScriptParser.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#if defined(MP3_MAD)
#include "MadWrapper.h"
#else
#include <smpeg.h>
#endif

#define ONS_VERSION "beta"

#define DEFAULT_SURFACE_FLAG (SDL_SWSURFACE)
//#define DEFAULT_SURFACE_FLAG (SDL_HWSURFACE)

#define DEFAULT_BLIT_FLAG (0)
//#define DEFAULT_BLIT_FLAG (SDL_RLEACCEL)

#define MAX_SPRITE_NUM 256
#define MAX_PARAM_NUM 100
#define CUSTOM_EFFECT_NO 100

#define DEFAULT_WM_TITLE "ONScripter"
#define DEFAULT_WM_ICON  "ONScripter"

#define DEFAULT_WAVE_CHANNEL 1

class ONScripterLabel : public ScriptParser
{
public:
    ONScripterLabel( bool cdaudio_flag, char *default_font, char *default_registry, char *default_archive_path, bool edit_flag );
    ~ONScripterLabel();

    bool skip_flag;
    bool draw_one_page_flag;
    bool key_pressed_flag;
    
    int  eventLoop();

    /* ---------------------------------------- */
    /* Commands */
    int wavestopCommand();
    int waveCommand();
    int waittimerCommand();
    int waitCommand();
    int vspCommand();
    int voicevolCommand();
    int trapCommand();
    int textspeedCommand();
    int textonCommand();
    int textoffCommand();
    int textclearCommand();
    int texecCommand();
    int tablegotoCommand();
    int systemcallCommand();
    int stopCommand();
    int spstrCommand();
    int spbtnCommand();
    int skipoffCommand();
    int sevolCommand();
    int setwindowCommand();
    int setcursorCommand();
    int selectCommand();
    int savetimeCommand();
    int saveonCommand();
    int saveoffCommand();
    int savegameCommand();
    int resettimerCommand();
    int resetCommand();
    int repaintCommand();
    int rndCommand();
    int rmodeCommand();
    int quakeCommand();
    int puttextCommand();
    int prnumclearCommand();
    int prnumCommand();
    int printCommand();
    int playstopCommand();
    int playonceCommand();
    int playCommand();
    int negaCommand();
    int mspCommand();
    int mp3volCommand();
    int mp3Command();
    int monocroCommand();
    int menu_windowCommand();
    int menu_fullCommand();
    int lspCommand();
    int lookbackflushCommand();
    int locateCommand();
    int loadgameCommand();
    int ldCommand();
    int jumpfCommand();
    int jumpbCommand();
    int ispageCommand();
    int isdownCommand();
    int getversionCommand();
    int gettimerCommand();
    int getregCommand();
    int getmouseposCommand();
    int getcursorposCommand();
    int getcselnumCommand();
    int gameCommand();
    int exbtnCommand();
    int erasetextwindowCommand();
    int endCommand();
    int dwavestopCommand();
    int dwaveCommand();
    int delayCommand();
    int cspCommand();
    int cselgotoCommand();
    int cselbtnCommand();
    int clickCommand();
    int clCommand();
    int cellCommand();
    int captionCommand();
    int btnwait2Command();
    int btnwaitCommand();
    int btntimeCommand();
    int btndownCommand();
    int btndefCommand();
    int btnCommand();
    int brCommand();
    int bltCommand();
    int bgCommand();
    int barclearCommand();
    int barCommand();
    int autoclickCommand();
    int allspresumeCommand();
    int allsphideCommand();
    int amspCommand();
    
protected:
    /* ---------------------------------------- */
    /* Event related variables */
    typedef enum{ NOT_EDIT_MODE=0,
                      EDIT_SELECT_MODE=1,
                      EDIT_VARIABLE_INDEX_MODE=2,
                      EDIT_VARIABLE_NUM_MODE=3,
                      EDIT_MP3_VOLUME_MODE=4,
                      EDIT_VOICE_VOLUME_MODE=5,
                      EDIT_SE_VOLUME_MODE=6
                      } VARIABLE_EDIT_MODE;
    
    int variable_edit_mode;
    int variable_edit_index;
    int variable_edit_num;
    int variable_edit_sign;
    
    void variableEditMode( SDL_KeyboardEvent *event );
    void keyPressEvent( SDL_KeyboardEvent *event );
    void mousePressEvent( SDL_MouseButtonEvent *event );
    void mouseMoveEvent( SDL_MouseMotionEvent *event );
    void timerEvent();
    void startTimer( int count );
    void advancePhase( int count=MINIMUM_TIMER_RESOLUTION );
    void trapHandler();
    int SetVideoMode();
    
private:
    typedef enum{ NORMAL_DISPLAY_MODE=0, TEXT_DISPLAY_MODE=1 } DISPLAY_MODE;
    typedef enum{ IDLE_EVENT_MODE = 0,
                      EFFECT_EVENT_MODE = 1,
                      WAIT_BUTTON_MODE  = 2, // For select and btnwait.
                      WAIT_INPUT_MODE   = (4|8),  // For select and text wait. It allows the right click menu.
                      WAIT_SLEEP_MODE   = 16,
                      WAIT_ANIMATION_MODE  = 32,
                      WAIT_TEXTBTN_MODE = 64
                      } EVENT_MODE;
    typedef enum{
        COLOR_EFFECT_IMAGE            = 0,
            DIRECT_EFFECT_IMAGE       = 1,
            BG_EFFECT_IMAGE           = 2,
            TACHI_EFFECT_IMAGE        = 3
            } EFFECT_IMAGE;

    /* ---------------------------------------- */
    /* Global definitions */
    bool edit_flag;
    
    long internal_timer;
    long autoclick_timer;
    long remaining_time;

    FILE *tmp_save_fp;
    bool saveon_flag;
    bool shelter_soveon_flag; // used by csel
    int yesno_caller;
    int yesno_selected_file_no;

    bool monocro_flag, monocro_flag_new;
    uchar3 monocro_color, monocro_color_new;
    uchar3 monocro_color_lut[256];
    bool need_refresh_flag;
    int  nega_mode;

    bool trap_flag;
    char *trap_dist;
    char *wm_title_string;
    char *wm_icon_string;
    char wm_edit_string[256];
    bool fullscreen_mode;

    Uint32 rmask, gmask, bmask, amask;

    long btntime_value;
    long internal_button_timer;
    long btnwait_time;
    bool btndown_flag;

    /* ---------------------------------------- */
    /* Script related variables */
    typedef enum{ REFRESH_NORMAL_MODE = 0,
                      REFRESH_SAYA_MODE = 1,
                      REFRESH_SHADOW_MODE = 2,
                      REFRESH_CURSOR_MODE = 4
                      } REFRESH_MODE;
    
    int display_mode;
    int event_mode;
    SDL_Surface *background_surface; // Backgroud image
    SDL_Surface *accumulation_surface; // Text window + Sprite + Tachi image + background
    SDL_Surface *select_surface; // Select_image + Tachi image + background
    SDL_Surface *text_surface; // Text + Select_image + Tachi image + background
    SDL_Surface *screen_surface; // Text + Select_image + Tachi image + background
    SDL_Surface *effect_dst_surface; // Intermediate source buffer for effect
    SDL_Surface *effect_src_surface; // Intermediate destnation buffer for effect
    SDL_Surface *shelter_select_surface; // Intermediate buffer to store accumulation_surface when entering system menu
    SDL_Surface *shelter_text_surface; // Intermediate buffer to store text_surface when entering system menu

    /* ---------------------------------------- */
    /* Button related variables */
    AnimationInfo btndef_info;

    struct ButtonState{
        int x, y, button;
        bool down_flag;
    } current_button_state, volatile_button_state, last_mouse_state, shelter_mouse_state;

    typedef enum{ NORMAL_BUTTON = 0,
                      SPRITE_BUTTON = 1,
                      EX_SPRITE_BUTTON = 2,
                      CUSTOM_SELECT_BUTTON = 3
                      } BUTTON_TYPE;

    struct ButtonLink{
        struct ButtonLink *next;
        BUTTON_TYPE button_type;
        int no;
        int sprite_no;
        char *exbtn_ctl;
        SDL_Rect select_rect;
        SDL_Rect image_rect;
        SDL_Surface *image_surface;

        ButtonLink(){
            next = NULL;
            exbtn_ctl = NULL;
            image_surface = NULL;
        };
        ~ButtonLink(){
            if ( image_surface ) SDL_FreeSurface( image_surface );
            if ( exbtn_ctl ) delete[] exbtn_ctl;
        };
    } root_button_link, *last_button_link, current_button_link, *shelter_button_link, exbtn_d_button_link;

    int current_over_button;

    void deleteButtonLink();
    void refreshMouseOverButton();
    int refreshSprite( SDL_Surface *surface, int sprite_no, bool active_flag, int cell_no, bool draw_flag, bool change_flag );
    int decodeExbtnControl( SDL_Surface *surface, char *ctl_str, bool draw_flag, bool change_flag );
    void drawExbtn( char *ctl_str );
    
    /* ---------------------------------------- */
    /* Background related variables */
    AnimationInfo bg_info;
    EFFECT_IMAGE bg_effect_image;

    /* ---------------------------------------- */
    /* Tachi-e related variables */
    /* 0 ... left, 1 ... center, 2 ... right */
    AnimationInfo tachi_info[3];

    /* ---------------------------------------- */
    /* Sprite related variables */
    AnimationInfo sprite_info[MAX_SPRITE_NUM];
    bool all_sprite_hide_flag;
    
    /* ---------------------------------------- */
    /* Parameter related variables */
    AnimationInfo *bar_info[MAX_PARAM_NUM], *prnum_info[MAX_PARAM_NUM];

    /* ---------------------------------------- */
    /* Cursor related variables */
    typedef enum{ CURSOR_WAIT_NO = 0,
                      CURSOR_NEWPAGE_NO = 1
                      } CURSOR_NO;
    AnimationInfo cursor_info[2];

    int proceedAnimation();
    int estimateNextDuration( AnimationInfo *anim, SDL_Rect *rect, int minimum );
    void resetRemainingTime( int t );
    void stopAnimation( int click );
    void loadCursor( int no, char *str, int x, int y, bool abs_flag = false );
    
    /* ---------------------------------------- */
    /* Lookback related variables */
    AnimationInfo lookback_info[4];
    
    /* ---------------------------------------- */
    /* Registry related variables */
    char *registry_file;
    
    /* ---------------------------------------- */
    /* Text related variables */
    AnimationInfo sentence_font_info;
    char *font_file;
    int erase_text_window_mode;
    bool text_on_flag;

    void drawChar( char* text, FontInfo *info, bool flush_flag, SDL_Surface *surface, bool buffering_flag = true, SDL_Rect *clip=NULL );
    void drawString( char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect = NULL, bool buffering_flag = false, SDL_Rect *clip=NULL );
    void restoreTextBuffer( SDL_Surface *surface = NULL );
    int clickWait( char *out_text );
    int clickNewPage( char *out_text );
    int textCommand();
    
    /* ---------------------------------------- */
    /* Effect related variables */
    char *effect_command;
    int effect_counter; // counter in each effect
    int effect_timer_resolution;
    int effect_start_time;
    int effect_start_time_old;
    SDL_Surface *effect_mask_surface;
    bool first_mouse_over_flag;
    
    void makeEffectStr( char **buf, char *dst_buf );
    int  setEffect( int effect_no, char *buf );
    int  doEffect( int effect_no, AnimationInfo *anim, int effect_image );

    /* ---------------------------------------- */
    /* Select related variables */
    typedef enum{ SELECT_GOTO_MODE=0, SELECT_GOSUB_MODE=1, SELECT_NUM_MODE=2, SELECT_CSEL_MODE=3 } SELECT_MODE;
    struct SelectLink{
        struct SelectLink *next;
        char *text;
        char *label;

        SelectLink(){
            next = NULL;
            text = label = NULL;
        };
        ~SelectLink(){
            if ( text )  delete[] text;
            if ( label ) delete[] label;
        };
    } root_select_link, *shelter_select_link;
    struct LinkLabelInfo select_label_info;
    int shortcut_mouse_line;

    void deleteSelectLink();
    struct ONScripterLabel::ButtonLink *getSelectableSentence( char *buffer, FontInfo *info, bool flush_flag = true, bool nofile_flag = false );
    
    /* ---------------------------------------- */
    /* Sound related variables */
    SDL_AudioSpec audio_format;
    bool audio_open_flag;
    bool cdaudio_flag;
    
    SDL_CD *cdrom_info;
    int current_cd_track;
    bool music_play_once_flag;
    char *music_file_name;
    unsigned char *mp3_buffer;
    SMPEG *mp3_sample;
    Mix_Music *midi_info;
    
    Mix_Chunk *wave_sample[MIX_CHANNELS];
    bool wave_play_once_flag;

    int playMIDIFile();
    int playMIDI();
    int playMP3( int cd_no );
    int playCDAudio( int cd_no );
    int playWave( char *file_name, bool loop_flag, int channel );
    void stopBGM( bool continue_flag );
    void playClickVoice();
    
    /* ---------------------------------------- */
    /* Text event related variables */
    bool text_char_flag;
    TTF_Font *text_font;
    bool new_line_skip_flag;
    int text_speed_no;

    void shadowTextDisplay( SDL_Surface *dst_surface, SDL_Surface *src_surface, SDL_Rect *clip=NULL, FontInfo *info=NULL );
    void clearCurrentTextBuffer();
    void newPage( bool next_flag );
    
    void deleteLabelLink();
    void flush( SDL_Rect *rect=NULL );
    void flush( int x, int y, int w, int h );
    void executeLabel();
    int parseLine();
    void parseTaggedString( AnimationInfo *anim );
    void setupAnimationInfo( AnimationInfo *anim );
    int doClipping( SDL_Rect *dst, SDL_Rect *clip, SDL_Rect *clipped=NULL );
    void alphaBlend( SDL_Surface *dst_surface, SDL_Rect dst_rect,
                     SDL_Surface *src1_surface, int x1, int y1,
                     SDL_Surface *src2_surface, int x2, int y2,
                     SDL_Surface *mask_surface, int x3,
                     int trans_mode, unsigned char mask_value = 255, unsigned int effect_value=0, SDL_Rect *clip=NULL, uchar3 *direct_color=NULL );
    int enterTextDisplayMode();
    int resizeSurface( SDL_Surface *src, SDL_Rect *src_rect, SDL_Surface *dst, SDL_Rect *dst_rect );
    SDL_Surface *loadImage( char *file_name );
    void drawTaggedSurface( SDL_Surface *dst_surface, AnimationInfo *anim, SDL_Rect *clip );
    void makeNegaSurface( SDL_Surface *surface, SDL_Rect *dst_rect=NULL );
    void makeMonochromeSurface( SDL_Surface *surface, SDL_Rect *dst_rect=NULL, FontInfo *info=NULL );
    void refreshSurfaceParameters();
    void refreshSurface( SDL_Surface *surface, SDL_Rect *clip=NULL, int refresh_mode = REFRESH_NORMAL_MODE );
    void mouseOverCheck( int x, int y );
    
    /* ---------------------------------------- */
    /* System call related method */
    bool system_menu_enter_flag;
    int  system_menu_mode;

    int shelter_event_mode;
    struct TextBuffer *shelter_text_buffer;
    
    void searchSaveFiles( int no = -1 );
    int loadSaveFile( int no );
    int saveSaveFile( int no );
    void setupLookbackButton();

    void enterSystemCall();
    void leaveSystemCall( bool restore_flag = true );
    void executeSystemCall();
    void executeSystemMenu();
    void executeSystemSkip();
    void executeSystemReset();
    void executeWindowErase();
    void executeSystemLoad();
    void executeSystemSave();
    void executeSystemYesNo();
    void executeSystemLookback();
};

#endif // __ONSCRIPTER_LABEL_H__
