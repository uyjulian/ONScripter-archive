/* -*- C++ -*-
 * 
 *  ScriptParser.h - Define block analyzer of ONScripter
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

#ifndef __SCRIPT_PARSER_H__
#define __SCRIPT_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "NsaReader.h"
#include "DirectReader.h"

#define ONSCRITER_VERSION 192

#define MINIMUM_TIMER_RESOLUTION 10
#define EFFECT_TIMER_RESOLUTION 100

#define VARIABLE_RANGE 4096
#define MAX_SAVE_FILE 20

#define MENU_TOP_X
#define MENU_TOP_Y

#define MAX_TEXT_BUFFER 10

#define WIDTH  640
#define HEIGHT 480

class ScriptParser
{
public:
    struct LabelInfo{
        char *name;
        char *start_address;
        int  num_of_lines;
        bool access_flag;
    };
    struct LinkLabelInfo{
        struct LinkLabelInfo *previous, *next;
        struct LabelInfo label_info;
        int current_line;
        int offset;
        char *current_script;
    };
    
    struct NameAlias{
        struct NameAlias *next;
        char *alias;
        int  num;
    };
    struct StringAlias{
        struct StringAlias *next;
        char *alias;
        char *str;
    };
    struct EffectLink{
        struct EffectLink *next;
        int num;
        int effect;
        int duration;
        char *image;
    };
    struct MenuLink{
        struct MenuLink *next;
        char *label;
        int system_call_no;
    };
    typedef enum{ SYSTEM_NULL = 0, SYSTEM_SKIP = 1, SYSTEM_RESET = 2, SYSTEM_SAVE = 3, SYSTEM_LOAD = 4, SYSTEM_LOOKBACK = 5, SYSTEM_WINDOWERASE = 6, SYSTEM_MENU = 7  } SYSTEM_CALLS;

    struct FontInfo{
        void *ttf_font;
        bool font_valid_flag;
        unsigned char color[3];
        int font_size;
        int top_xy[2]; // Top left origin
        int num_xy[2]; // Row and column of the text windows
        int xy[2]; // Current position
        int pitch_xy[2]; // Width and height of a character
        int wait_time;
        bool display_bold;
        bool display_shadow;
        bool display_transparency;
        char window_color[8];
        int  window_color_mask[3];
        char *window_image;
        int window_rect[4]; // Top left and bottom right of the text window
    };

    struct TextBuffer{
        struct TextBuffer *next;
        char *buffer;
        int num_xy[2];
        int xy[2];
    };
    
    typedef enum{ RET_COMMENT = 0, RET_NOMATCH = 1, RET_CONTINUE = 2, RET_CONTINUE_NONEXT = 3, RET_WAIT = 4, RET_WAIT_NEXT = 5, RET_JUMP = 6 } COMMAND_RETURN;
    typedef enum{ CLICK_NONE = 0, CLICK_WAIT = 1, CLICK_NEWPAGE = 2, CLICK_IGNORE = 3 } CLICKSTR_STATE;

    ScriptParser();
    ~ScriptParser();

    int open();
    int readLine( char **buf, bool raw_flag = false );
    int getNumLabelAccessed();
    bool getLabelAccessFlag( const char *label );
    struct LabelInfo lookupLabel( const char* label );
    struct LabelInfo lookupLabelNext( const char* label );
    int parseLine();

    bool readToken( char **src_buf, char *dst_buf, bool comma_check_flag = true );
    int readInt( char **src_buf, char *dst_buf, bool comma_check_flag = true );
    bool readStr( char **src_buf, char *dst_buf, bool comma_check_flag = true );
    void skipToken();
    void saveGlovalData();
    void saveFileLog();
    void saveLabelLog();
    
    const char* getVersion();

    /* Command */
    int versionstrCommand();
    int underlineCommand();
    int timeCommand();
    int subCommand();
    int straliasCommand();
    int skipCommand();
    int savenumberCommand();
    int savenameCommand();
    int rmenuCommand();
    int returnCommand();
    int numaliasCommand();
    int mulCommand();
    int movCommand();
    int modCommand();
    int menusetwindowCommand();
    int labellogCommand();
    int itoaCommand();
    int incCommand();
    int ifCommand();
    int humanzCommand();
    int gotoCommand();
    int gosubCommand();
    int globalonCommand();
    int gameCommand();
    int filelogCommand();
    int effectblankCommand();
    int effectCommand();
    int divCommand();
    int decCommand();
    int dateCommand();
    int cmpCommand();
    int clickstrCommand();
    int atoiCommand();
    int arcCommand();
    int addCommand();
    
protected:
    typedef enum{ NORMAL_MODE, DEFINE_MODE } MODE;
    MODE current_mode;

    bool globalon_flag;
    bool filelog_flag;
    bool labellog_flag;
    int num_of_label_accessed;
    int stack_depth;

    bool jumpf_flag;
    LinkLabelInfo last_tilde;
    int z_order;

    BaseReader *cBR;

    int string_buffer_length, string_buffer_offset;//, string_buffer_offset_command;
    char *string_buffer, *tmp_string_buffer;

    struct LinkLabelInfo root_link_label_info, *current_link_label_info;
    struct EffectLink getEffect( int effect_no );

    /* ---------------------------------------- */
    /* Global definitions */
    char *version_str;
    int underline_value;
    
    /* ---------------------------------------- */
    /* Number variables and string variables */
    int num_variables[ VARIABLE_RANGE ];
    char *str_variables[ VARIABLE_RANGE ];

    int effect_blank;

    void gosubReal( char *label );

    typedef enum{ WINDOW_EFFECT = -1, PRINT_EFFECT = -2, TMP_EFFECT = -3  } EFFECT_MODE;
    struct EffectLink window_effect, print_effect, tmp_effect;

    /* ---------------------------------------- */
    /* Save/Load related variables */
    struct SaveFileInfo{
        bool valid;
        char no[5];
        char month[5];
        char day[5];
        char hour[5];
        char minute[5];
    } save_file_info[MAX_SAVE_FILE];
    unsigned int  num_save_file;
    char *save_menu_name;
    char *load_menu_name;
    char *save_item_name;

    /* ---------------------------------------- */
    /* Text related variables */
    struct TextBuffer text_buffer[ MAX_TEXT_BUFFER ], *current_text_buffer; // ring buffer
    bool text_line_flag;
    int  clickstr_num;
    char *clickstr_list;
    int  clickstr_line;
    int  clickstr_state;
    
    /* ---------------------------------------- */
    /* Menu related variables */
    struct FontInfo system_font;
    struct FontInfo menu_font;
    struct MenuLink root_menu_link, *last_menu_link;
    unsigned int  menu_link_num, menu_link_width;

    int getSystemCallNo( char *buffer );
    void getSJISFromInteger( char *buffer, int no, bool add_space_flag=true );
    void addStringBuffer( char ch, int string_counter );
    unsigned char convHexToDec( char ch );

    void errorAndExit( char *str );

    void saveInt( FILE *fp, int var );
    void loadInt( FILE *fp, int *var );
    void saveStr( FILE *fp, char *str );
    void loadStr( FILE *fp, char **str );
    void saveVariables( FILE *fp, int from, int to );
    void loadVariables( FILE *fp, int from, int to );
    
private:
    char *script_buffer;
    long script_buffer_length;
    int num_of_labels;
    struct LabelInfo *label_info;

    struct NameAlias root_name_alias, *last_name_alias;
    struct StringAlias root_str_alias, *last_str_alias;
    struct EffectLink root_effect_link, *last_effect_link;

    int parseNumAlias( char **buf );

    int readScript();
    int labelScript();
    int findLabel( const char* label );
};

#endif // __SCRIPT_PARSER_H__
