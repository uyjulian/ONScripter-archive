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
#include <SDL_sound.h>
#include <SDL_mixer.h>

#define FONT_NAME "default.ttf"

#define DEFAULT_SURFACE_FLAG (SDL_SWSURFACE)
//#define DEFAULT_SURFACE_FLAG (SDL_HWSURFACE)

#define DEFAULT_BLIT_FLAG (0)
//#define DEFAULT_BLIT_FLAG (SDL_RLEACCEL)

#define MAX_SPRITE_NUM 50

class ONScripterLabel : public ScriptParser
{
public:
    ONScripterLabel();
    ~ONScripterLabel();

    bool skip_flag;
    bool draw_one_page_flag;
    bool new_page_flag;
    bool key_pressed_flag;
    
    int  eventLoop();

    /* ---------------------------------------- */
    /* Commands */
    int clickWait( char *out_text );
    int clickNewPage( char *out_text );
    int textCommand( char *text );
    
    int wavestopCommand();
    int waveCommand();
    int waittimerCommand();
    int waitCommand();
    int vspCommand();
    int textclearCommand();
    int systemcallCommand();
    int stopCommand();
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
    int endCommand();
    int dwavestopCommand();
    int dwaveCommand();
    int delayCommand();
    int cspCommand();
    int clickCommand();
    int clCommand();
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
    void startTimer( Uint32 count );
    int SetVideoMode(int w, int h);
    
private:
    typedef enum{ NORMAL_DISPLAY_MODE=0, TEXT_DISPLAY_MODE=1 } DISPLAY_MODE;
    typedef enum{ IDLE_EVENT_MODE = 0,
                      EFFECT_EVENT_MODE = 1,
                      WAIT_BUTTON_MODE  = 2, // For select and btnwait.
                      WAIT_MOUSE_MODE   = 4,  // For select and text wait. It allows the right click menu.
                      WAIT_KEY_MODE     = 8,
                      WAIT_SLEEP_MODE   = 16,
                      WAIT_CURSOR_MODE  = 32
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

        int num_of_cells;
        int *duration_list;
        uchar3 *color_list;
        int loop_mode;
        
        char *file_name;

        TaggedInfo(){
            num_of_cells = 0;
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

    long internal_timer;
    long autoclick_timer;

    bool monocro_flag;
    uchar3 monocro_color;
    uchar3 monocro_color_lut[256];

    bool rmode_flag;

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
    /* Button related variables */
    struct ButtonState{
        int x, y, button;
    } current_button_state, volatile_button_state, last_mouse_state, shelter_mouse_state;
    
    struct ButtonLink{
        struct ButtonLink *next;
        int no;
        SDL_Rect select_rect;
        SDL_Rect image_rect;
        SDL_Surface *image_surface;
    } root_button_link, *last_button_link, current_over_button_link, *shelter_button_link;

    int current_over_button;

    void deleteButtonLink();
    void refreshMouseOverButton();
    
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
    
    void makeEffectStr( char *buf, int no, int duration, char *image );
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
    /* Tachi related variables */
    /* 0 ... left, 1 ... center, 2 ... right */
    int tachi_image_x[3], tachi_image_width[3];
    char *tachi_image_name[3];
    SDL_Surface *tachi_image_surface[3];
    
    /* ---------------------------------------- */
    /* Sprite related variables */
    struct SpriteInfo{
        bool valid;
        int x, y;
        int trans;
        struct TaggedInfo tag;
        char *name;
        SDL_Surface *image_surface;
    } sprite_info[MAX_SPRITE_NUM];
    
    /* ---------------------------------------- */
    /* Cursor related variables */
    struct CursorInfo{
        int xy[2];
        int w, h;
        int count;
        int direction;
        struct TaggedInfo tag;
        char *image_name;
        SDL_Surface *image_surface;
        SDL_Surface *preserve_surface;
    } cursor_info[2];

    void loadCursor( int no, char *str, int x, int y );
    void startCursor( int click );
    void endCursor( int click );
    
    /* ---------------------------------------- */
    /* Sound related variables */
    bool audio_open_flag;
    Sound_Sample *mp3_sample;
    int current_cd_track;
    bool mp3_play_once_flag;
    char *mp3_file_name;
    unsigned char *mp3_buffer;

    Mix_Chunk *wave_sample;
    bool wave_play_once_flag;

    int playMP3( int cd_no );
    int playWave( char *file_name, bool loop_flag );
    
    /* ---------------------------------------- */
    /* Text event related variables */
    bool text_char_flag;
    TTF_Font *text_font;
    bool new_line_skip_flag;
    int text_speed_no;
    int default_text_speed[3];

    void drawChar( char* text, struct FontInfo *info, bool flush_flag = true, SDL_Surface *surface = NULL );
    void clearCurrentTextBuffer();
    void enterNewPage();
    
    void deleteLabelLink();
    void flush( int x=-1, int y=-1, int wx=-1, int wy=-1 );
    void executeLabel();
    int parseLine();
    void parseTaggedString( char *buffer, struct TaggedInfo *tag );
    void alphaBlend( SDL_Surface *dst_surface, int x, int y,
                     SDL_Surface *src1_surface, int x1, int y1, int wx, int wy,
                     SDL_Surface *src2_surface, int x2, int y2,
                     int x3, int y3, int mask_value );
    int enterTextDisplayMode();
    SDL_Surface *loadPixmap( struct TaggedInfo *tag );
    void drawTaggedSurface( SDL_Surface *dst_surface, int x, int y, int w, int h,
                           SDL_Surface *src_surface, TaggedInfo *tagged_info );
    void makeMonochromeSurface( SDL_Surface *surface, SDL_Rect *dst_rect=NULL );
    void refreshAccumulationSruface( SDL_Surface *surface );
    void restoreTextBuffer();
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
