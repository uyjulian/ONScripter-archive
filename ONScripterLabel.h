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
#include <smpeg.h>
#include <SDL_mixer.h>

#define FONT_NAME "default.ttf"

#define DEFAULT_SURFACE_FLAG (SDL_SWSURFACE)
//#define DEFAULT_SURFACE_FLAG (SDL_HWSURFACE)

#define DEFAULT_BLIT_FLAG (0)
//#define DEFAULT_BLIT_FLAG (SDL_RLEACCEL)

#define MAX_SPRITE_NUM 256

class ONScripterLabel : public ScriptParser
{
public:
    ONScripterLabel();
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
    int textclearCommand();
    int systemcallCommand();
    int stopCommand();
    int spstrCommand();
    int spbtnCommand();
    int sevolCommand();
    int setwindowCommand();
    int setcursorCommand();
    int selectCommand();
    int savegameCommand();
    int resettimerCommand();
    int resetCommand();
    int rndCommand();
    int rmodeCommand();
    int puttextCommand();
    int printCommand();
    int playstopCommand();
    int playonceCommand();
    int playCommand();
    int mspCommand();
    int mp3volCommand();
    int mp3Command();
    int monocroCommand();
    int lspCommand();
    int lookbackflushCommand();
    int locateCommand();
    int loadgameCommand();
    int ldCommand();
    int jumpfCommand();
    int jumpbCommand();
    int getversionCommand();
    int gettimerCommand();
    int gameCommand();
    int exbtnCommand();
    int endCommand();
    int dwavestopCommand();
    int dwaveCommand();
    int delayCommand();
    int cspCommand();
    int clickCommand();
    int clCommand();
    int cellCommand();
    int captionCommand();
    int btnwait2Command();
    int btnwaitCommand();
    int btndefCommand();
    int btnCommand();
    int brCommand();
    int bgCommand();
    int bltCommand();
    int autoclickCommand();
    int amspCommand();
    
protected:
    void keyPressEvent( SDL_KeyboardEvent *event );
    void mousePressEvent( SDL_MouseButtonEvent *event );
    void mouseMoveEvent( SDL_MouseMotionEvent *event );
    void timerEvent();
    void startTimer( int count );
    int SetVideoMode();
    
private:
    typedef enum{ NORMAL_DISPLAY_MODE=0, TEXT_DISPLAY_MODE=1 } DISPLAY_MODE;
    typedef enum{ IDLE_EVENT_MODE = 0,
                      EFFECT_EVENT_MODE = 1,
                      WAIT_BUTTON_MODE  = 2, // For select and btnwait.
                      WAIT_INPUT_MODE   = (4|8),  // For select and text wait. It allows the right click menu.
                      WAIT_SLEEP_MODE   = 16,
                      WAIT_ANIMATION_MODE  = 32
                      } EVENT_MODE;
    typedef enum{
        COLOR_EFFECT_IMAGE            = 0,
            DIRECT_EFFECT_IMAGE       = 1,
            BG_EFFECT_IMAGE           = 2,
            TACHI_EFFECT_IMAGE        = 3
            } EFFECT_IMAGE;

    struct TaggedInfo{
        int trans_mode;
        char direct_color[8];
        int pallet_number;
        uchar3 color;
        SDL_Rect pos; // pose and size of the current cell

        int num_of_cells;
        int current_cell;
        int direction;
        int *duration_list;
        uchar3 *color_list;
        int loop_mode;
        
        char *file_name;

        TaggedInfo(){
            current_cell = 0;
            num_of_cells = 0;
            direction = 1;
            duration_list = NULL;
            color_list = NULL;
            file_name = NULL;
        }
        void remove(){
            if ( duration_list ){
                delete[] duration_list;
                duration_list = NULL;
            }
            if ( color_list ){
                delete[] color_list;
                color_list = NULL;
            }
            if ( file_name ){
                delete[] file_name;
                file_name = NULL;
            }
            num_of_cells = 0;
        }
        ~TaggedInfo(){
            remove();
        }
    };

    /* ---------------------------------------- */
    /* Global definitions */
    long internal_timer;
    long autoclick_timer;

    bool monocro_flag;
    uchar3 monocro_color;
    uchar3 monocro_color_lut[256];

    bool trap_flag;
    char *trap_dist;

    Uint32 rmask, gmask, bmask, amask;
    
    /* ---------------------------------------- */
    /* Script related variables */
    int display_mode;
    int event_mode;
    SDL_Surface *btndef_surface;
    SDL_Surface *background_surface; // Backgroud image
    SDL_Surface *accumulation_surface; // Tachi image + background
    SDL_Surface *select_surface; // Select_image + Tachi image + background
    SDL_Surface *text_surface; // Text + Select_image + Tachi image + background
    SDL_Surface *screen_surface; // Text + Select_image + Tachi image + background
    SDL_Surface *effect_dst_surface; // Intermediate source buffer for effect
    SDL_Surface *effect_src_surface; // Intermediate destnation buffer for effect
    SDL_Surface *shelter_select_surface; // Intermediate buffer to store accumulation_surface when entering system menu
    SDL_Surface *shelter_text_surface; // Intermediate buffer to store text_surface when entering system menu

    /* ---------------------------------------- */
    /* Text related variables */
    struct TaggedInfo sentence_font_tag;

    void drawChar( char* text, struct FontInfo *info, bool flush_flag = true, SDL_Surface *surface = NULL, bool buffering_flag = true );
    void drawString( char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect = NULL );
    void restoreTextBuffer();
    int clickWait( char *out_text );
    int clickNewPage( char *out_text );
    int textCommand( char *text );
    
    /* ---------------------------------------- */
    /* Button related variables */
    struct ButtonState{
        int x, y, button;
    } current_button_state, volatile_button_state, last_mouse_state, shelter_mouse_state;

    typedef enum{ NORMAL_BUTTON = 0,
                      SPRITE_BUTTON = 1,
                      EX_SPRITE_BUTTON = 2
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
    } root_button_link, *last_button_link, current_button_link, *shelter_button_link, exbtn_d_button_link;

    int current_over_button;

    void deleteButtonLink();
    void refreshMouseOverButton();
    int refreshSprite( SDL_Surface *surface, int sprite_no, bool active_flag, int cell_no, bool draw_flag );
    int decodeExbtnControl( SDL_Surface *surface, char *ctl_str, bool draw_flag );
    void drawExbtn( SDL_Surface *surface, char *ctl_str );
    
    /* ---------------------------------------- */
    /* Effect related variables */
    struct DelayedInfo{
        struct DelayedInfo *next;
        char *command;
    } *start_delayed_effect_info;
    int effect_counter; // counter in each effect
    int effect_timer_resolution;
    int effect_start_time;
    int effect_start_time_old;
    bool first_mouse_over_flag;
    
    int readEffect( char **buf, struct EffectLink *effect );
    void makeEffectStr( char **buf, char *dst_buf );
    int  setEffect( int immediate_flag, char *buf );
    int doEffect( int effect_no, struct TaggedInfo *tag, int effect_image );

    /* ---------------------------------------- */
    /* Select related variables */
    struct SelectLink{
        struct SelectLink *next;
        char *text;
        char *label;
    } root_select_link, *last_select_link;
    struct LinkLabelInfo select_label_info;
    int shortcut_mouse_line;

    void deleteSelectLink();
    struct ONScripterLabel::ButtonLink *getSelectableSentence( char *buffer, struct FontInfo *info, bool flush_flag = true, bool nofile_flag = false );
    
    /* ---------------------------------------- */
    /* Lookback related variables */
    struct TaggedInfo lookback_image_tag[4];
    SDL_Surface *lookback_image_surface[4];
    
    /* ---------------------------------------- */
    /* Animation related variables */
    struct AnimationInfo{
        bool valid;
        bool abs_flag;
        SDL_Rect pos;
        int trans;
        struct TaggedInfo tag;
        char *image_name;
        SDL_Surface *image_surface;
        SDL_Surface *preserve_surface;

        AnimationInfo(){
            valid = false;
            image_name = NULL;
            image_surface = NULL;
            preserve_surface = NULL;
            trans = 255;
        }
        void deleteImageName(){
            if ( image_name ) delete[] image_name;
            image_name = NULL;
        }
        void setImageName( char *name ){
            deleteImageName();
            image_name = new char[ strlen(name) + 1 ];
            memcpy( image_name, name, strlen(name) + 1 );
        }
        void deleteImageSurface(){
            if ( image_surface ) SDL_FreeSurface( image_surface );
            image_surface = NULL;
        }
    };

    /* ---------------------------------------- */
    /* Tachi-e related variables */
    /* 0 ... left, 1 ... center, 2 ... right */
    struct AnimationInfo tachi_info[3];

    /* ---------------------------------------- */
    /* Sprite related variables */
    struct AnimationInfo sprite_info[MAX_SPRITE_NUM];
    
    /* ---------------------------------------- */
    /* Cursor related variables */
    typedef enum{ CURSOR_WAIT_NO = 0,
                      CURSOR_NEWPAGE_NO = 1
                      } CURSOR_NO;
    struct AnimationInfo cursor_info[2];

    void proceedAnimation( AnimationInfo *anim );
    void showAnimation( AnimationInfo *anim );
    void loadCursor( int no, char *str, int x, int y, bool abs_flag = false );
    void startCursor( int click );
    void endCursor( int click );
    
    /* ---------------------------------------- */
    /* Sound related variables */
    SDL_AudioSpec audio_format;
    bool audio_open_flag;

    int current_cd_track;
    bool mp3_play_once_flag;
    char *mp3_file_name;
    unsigned char *mp3_buffer;
    SMPEG_Info mp3_info;
    SMPEG *mp3_sample;
    
    Mix_Chunk *wave_sample[MIX_CHANNELS];
    bool wave_play_once_flag;

    int playMP3( int cd_no );
    int playWave( char *file_name, bool loop_flag, int channel );
    
    /* ---------------------------------------- */
    /* Text event related variables */
    bool text_char_flag;
    TTF_Font *text_font;
    bool new_line_skip_flag;
    int text_speed_no;
    int default_text_speed[3];

    void shadowTextDisplay();
    void clearCurrentTextBuffer();
    void enterNewPage();
    
    void deleteLabelLink();
    void flush( SDL_Rect *rect=NULL );
    void flush( int x, int y, int w, int h );
    void executeLabel();
    int parseLine();
    void parseTaggedString( char *buffer, struct TaggedInfo *tag );
    void setupAnimationInfo( struct AnimationInfo *anim );
    void alphaBlend( SDL_Surface *dst_surface, int x, int y,
                     SDL_Surface *src1_surface, int x1, int y1, int wx, int wy,
                     SDL_Surface *src2_surface, int x2, int y2,
                     int x3, int y3, int mask_value );
    int enterTextDisplayMode();
    SDL_Surface *loadPixmap( struct TaggedInfo *tag );
    void drawTaggedSurface( SDL_Surface *dst_surface, SDL_Rect *pos, SDL_Rect *clip,
                           SDL_Surface *src_surface, TaggedInfo *tagged_info );
    void makeMonochromeSurface( SDL_Surface *surface, SDL_Rect *dst_rect=NULL, bool one_color_flag = true );
    void refreshAccumulationSurface( SDL_Surface *surface, SDL_Rect *rect=NULL );
    void mouseOverCheck( int x, int y );
    
    /* ---------------------------------------- */
    /* System call related method */
    bool system_menu_enter_flag;
    int  system_menu_mode;

    struct TaggedInfo bg_image_tag;
    EFFECT_IMAGE bg_effect_image;
    int shelter_event_mode;
    uchar3 shelter_sentence_color;
    struct TextBuffer *shelter_text_buffer;
    
    void searchSaveFiles();
    int loadSaveFile( int no );
    int saveSaveFile( int no );
    void setupLookbackButton();

    void leaveSystemCall( bool restore_flag = true );
    void executeSystemCall();
    void executeSystemMenu();
    void executeSystemSkip();
    void executeSystemReset();
    void executeWindowErase();
    void executeSystemLoad();
    void executeSystemSave();
    void executeSystemLookback();
};

#endif // __ONSCRIPTER_LABEL_H__
