/* -*- C++ -*-
 * 
 *  ScriptParser.h - Define block parser of ONScripter
 *
 *  Copyright (c) 2001-2004 Ogapee. All rights reserved.
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

#ifndef __SCRIPT_PARSER_H__
#define __SCRIPT_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "ScriptHandler.h"
#include "NsaReader.h"
#include "DirectReader.h"
#include "AnimationInfo.h"
#include "FontInfo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef unsigned char uchar3[3];

class ScriptParser
{
public:
    ScriptParser( char *path, char *key_exe );
    ~ScriptParser();

    int open( char *path );
    int parseLine();

    FILE *fopen(const char *path, const char *mode);
    void saveGlovalData();

    /* Command */
    int windowbackCommand();
    int versionstrCommand();
    int usewheelCommand();
    int useescspcCommand();
    int underlineCommand();
    int transmodeCommand();
    int timeCommand();
    int textgosubCommand();
    int subCommand();
    int straliasCommand();
    int soundpressplginCommand();
    int skipCommand();
    int shadedistanceCommand();
    int selectvoiceCommand();
    int selectcolorCommand();
    int savenumberCommand();
    int savenameCommand();
    int rubyonCommand();
    int rubyoffCommand();
    int roffCommand();
    int rmenuCommand();
    int returnCommand();
    int pretextgosubCommand();
    int numaliasCommand();
    int nsadirCommand();
    int nsaCommand();
    int nextCommand();
    int mulCommand();
    int movCommand();
    int mode_sayaCommand();
    int mode_extCommand();
    int modCommand();
    int midCommand();
    int menusetwindowCommand();
    int menuselectvoiceCommand();
    int menuselectcolorCommand();
    int maxkaisoupageCommand();
    int lookbackcolorCommand();
    //int lookbackbuttonCommand();
    int linepageCommand();
    int lenCommand();
    int labellogCommand();
    int kidokuskipCommand();
    int kidokumodeCommand();
    int itoaCommand();
    int intlimitCommand();
    int incCommand();
    int ifCommand();
    int humanzCommand();
    int gotoCommand();
    int gosubCommand();
    int globalonCommand();
    int getparamCommand();
    //int gameCommand();
    int forCommand();
    int filelogCommand();
    int effectcutCommand();
    int effectblankCommand();
    int effectCommand();
    int divCommand();
    int dimCommand();
    int defvoicevolCommand();
    int defsubCommand();
    int defsevolCommand();
    int defmp3volCommand();
    int defaultspeedCommand();
    int defaultfontCommand();
    int decCommand();
    int dateCommand();
    int cmpCommand();
    int clickvoiceCommand();
    int clickstrCommand();
    int breakCommand();
    int atoiCommand();
    int arcCommand();
    int addCommand();
    
protected:
    struct UserFuncLUT{
        struct UserFuncLUT *next;
        char *command;
        UserFuncLUT(){
            next = NULL;
            command = NULL;
        };
        ~UserFuncLUT(){
            if (command) delete[] command;
        };
    } root_user_func, *last_user_func;

    struct NestInfo{
        enum { LABEL = 0,
               FOR   = 1 };
        struct NestInfo *previous, *next;
        int  nest_mode;
        char *next_script; // used in gosub and for
        int  var_no, to, step; // used in for

        NestInfo(){
            previous = next = NULL;
            nest_mode = LABEL;
        };
    } last_tilde;

    enum { SYSTEM_NULL        = 0,
           SYSTEM_SKIP        = 1,
           SYSTEM_RESET       = 0x10, // used in logical operation
           SYSTEM_SAVE        = 3,
           SYSTEM_LOAD        = 4,
           SYSTEM_LOOKBACK    = 5,
           SYSTEM_WINDOWERASE = 6,
           SYSTEM_MENU        = 7,
           SYSTEM_YESNO       = 8,
           SYSTEM_END         = 0x20 // used in logical operation
    };
    enum { RET_NOMATCH   = 0,
           RET_SKIP_LINE = 1,
           RET_CONTINUE  = 2,
           RET_WAIT      = 4,
           RET_NOREAD    = 8,
           RET_REREAD    = 16
    };
    enum { CLICK_NONE    = 0,
           CLICK_WAIT    = 1,
           CLICK_NEWPAGE = 2,
           CLICK_IGNORE  = 3,
           CLICK_EOL     = 4
    };
    enum{ NORMAL_MODE, DEFINE_MODE };
    int current_mode;
    int debug_level;

    char *archive_path;
    char *nsa_path;
    bool globalon_flag;
    bool labellog_flag;
    bool filelog_flag;
    bool kidokuskip_flag;
    bool kidokumode_flag;

    int z_order;
    bool rmode_flag;
    bool windowback_flag;
    bool usewheel_flag;
    bool useescspc_flag;
    bool mode_saya_flag;
    bool mode_ext_flag;
    bool force_button_shortcut_flag;
    bool rubyon_flag;
    
    int string_buffer_offset;

    NestInfo root_nest_info, *last_nest_info;
    ScriptHandler::LabelInfo current_label_info;
    int current_line;

    /* ---------------------------------------- */
    /* Global definitions */
    int screen_ratio1, screen_ratio2;
    int screen_width, screen_height;
    int screen_texture_width, screen_texture_height;
    int screen_bpp;
    char *version_str;
    int underline_value;

    void setStr( char **dst, const char *src, int num=-1 );
    
    void gosubReal( const char *label, char *next_script );
    void setCurrentLabel( const char *label );

    /* ---------------------------------------- */
    /* Effect related variables */
    enum{ WINDOW_EFFECT = -1, TMP_EFFECT = -2 };
    struct EffectLink{
        struct EffectLink *next;
        int num;
        int effect;
        int duration;
        AnimationInfo anim;

        EffectLink(){
            next = NULL;
            effect = 10;
            duration = 0;
        };
    };
    
    EffectLink root_effect_link, *last_effect_link, window_effect, tmp_effect;
    
    int effect_blank;
    bool effect_cut_flag;

    EffectLink *getEffect( int effect_no );
    int readEffect( EffectLink *effect );

    /* ---------------------------------------- */
    /* Lookback related variables */
    //char *lookback_image_name[4];
    int lookback_sp[2];
    uchar3 lookback_color;
    
    /* ---------------------------------------- */
    /* Select related variables */
    uchar3 select_on_color;
    uchar3 select_off_color;
    
    /* ---------------------------------------- */
    /* For loop related variables */
    bool break_flag;
    
    /* ---------------------------------------- */
    /* Transmode related variables */
    int trans_mode;
    
    /* ---------------------------------------- */
    /* Save/Load related variables */
    struct SaveFileInfo{
        bool valid;
        int  month, day, hour, minute;
        char sjis_no[5];
        char sjis_month[5];
        char sjis_day[5];
        char sjis_hour[5];
        char sjis_minute[5];
    };
    unsigned int num_save_file;
    char *save_menu_name;
    char *load_menu_name;
    char *save_item_name;

    /* ---------------------------------------- */
    /* Text related variables */
    char *default_env_font;
    int default_text_speed[3];
    struct TextBuffer{
        struct TextBuffer *next, *previous;
        char *buffer2;
        int num_xy[2];
        int buffer2_count;

        TextBuffer(){
            buffer2 = NULL;
            buffer2_count = 0;
        }
        int addBuffer( char ch ){
            if ( buffer2_count >= (num_xy[0]*2+1)*(num_xy[1]+1) ) return -1;
            buffer2[buffer2_count++] = ch;
            return 0;
        };
    } *text_buffer, *start_text_buffer, *current_text_buffer; // ring buffer
    int  max_text_buffer;
    int  clickstr_line;
    int  clickstr_state;
    int  num_chars_in_sentence;
    
    /* ---------------------------------------- */
    /* Sound related variables */
    int mp3_volume;
    int voice_volume;
    int se_volume;

    enum { CLICKVOICE_NORMAL  = 0,
           CLICKVOICE_NEWPAGE = 1,
           CLICKVOICE_NUM     = 2
    };
    char *clickvoice_file_name[CLICKVOICE_NUM];

    enum { SELECTVOICE_OPEN   = 0,
           SELECTVOICE_OVER   = 1,
           SELECTVOICE_SELECT = 2,
           SELECTVOICE_NUM    = 3
    };
    char *selectvoice_file_name[SELECTVOICE_NUM];

    enum { MENUSELECTVOICE_OPEN   = 0,
           MENUSELECTVOICE_CANCEL = 1,
           MENUSELECTVOICE_OVER   = 2,
           MENUSELECTVOICE_CLICK  = 3,
           MENUSELECTVOICE_WARN   = 4,
           MENUSELECTVOICE_YES    = 5,
           MENUSELECTVOICE_NO     = 6,
           MENUSELECTVOICE_NUM    = 7
    };
    char *menuselectvoice_file_name[MENUSELECTVOICE_NUM];
     
    /* ---------------------------------------- */
    /* Font related variables */
    FontInfo *current_font, sentence_font, menu_font, ruby_font;
    struct RubyStruct{
        enum { NONE,
               BODY,
               RUBY };
        int stage;
        int body_count;
        int ruby_start;
        int ruby_end;
        int ruby_count;
        int margin;

        int font_size_xy[2];
        char *font_name;

        RubyStruct(){
            stage = NONE;
            font_size_xy[0] = 0;
            font_size_xy[1] = 0;
            font_name = NULL;
        };
        ~RubyStruct(){
            if ( font_name ) delete[] font_name;
        };
    } ruby_struct;
    int shade_distance[2];

    /* ---------------------------------------- */
    /* Menu related variables */
    struct MenuLink{
        struct MenuLink *next;
        char *label;
        int system_call_no;

        MenuLink(){
            next  = NULL;
            label = NULL;
        };
    } root_menu_link;
    unsigned int  menu_link_num, menu_link_width;

    int getSystemCallNo( const char *buffer );
    unsigned char convHexToDec( char ch );
    void readColor( uchar3 *color, const char *buf );
    
    void errorAndExit( const char *str, const char *reason=NULL );

    void saveInt( FILE *fp, int var );
    void loadInt( FILE *fp, int *var );
    void saveStr( FILE *fp, char *str );
    void loadStr( FILE *fp, char **str );
    void saveVariables( FILE *fp, int from, int to );
    void loadVariables( FILE *fp, int from, int to );

    /* ---------------------------------------- */
    /* System customize related variables */
    char *textgosub_label;
    char *pretextgosub_label;

protected:
    ScriptHandler script_h;
    
private:
    unsigned char *key_table;

    void createKeyTable( const char *key_exe );
};

#endif // __SCRIPT_PARSER_H__
