/* -*- C++ -*-
 * 
 *  ONScripterLabel_text.cpp - Text parser of ONScripter
 *
 *  Copyright (c) 2001-2005 Ogapee. All rights reserved.
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

extern unsigned short convSJIS2UTF16( unsigned short in );

#define IS_KINSOKU(x)	\
        ( *(x) == (char)0x81 && *((x)+1) == (char)0x41 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x42 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x48 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x49 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x76 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x78 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x5b )

#define IS_ROTATION_REQUIRED(x)	\
        ( !IS_TWO_BYTE(*(x)) || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x50 || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x51 || \
          *(x) == (char)0x81 && *((x)+1) >= 0x5b && *((x)+1) <= 0x5d || \
          *(x) == (char)0x81 && *((x)+1) >= 0x60 && *((x)+1) <= 0x64 || \
          *(x) == (char)0x81 && *((x)+1) >= 0x69 && *((x)+1) <= 0x7a || \
          *(x) == (char)0x81 && *((x)+1) == (char)0x80 )

#define IS_TRANSLATION_REQUIRED(x)	\
        ( *(x) == (char)0x81 && *((x)+1) >= 0x41 && *((x)+1) <= 0x44 )

void ONScripterLabel::drawGlyph( SDL_Surface *dst_surface, FontInfo *info, SDL_Color &color, char* text, int xy[2], bool shadow_flag, SDL_Surface *cache_surface, SDL_Rect *clip, SDL_Rect &dst_rect )
{
    unsigned short unicode;
    if (IS_TWO_BYTE(text[0])){
        unsigned index = ((unsigned char*)text)[0];
        index = index << 8 | ((unsigned char*)text)[1];
        unicode = convSJIS2UTF16( index );
    }
    else{
        if ((text[0] & 0xe0) == 0xa0 || (text[0] & 0xe0) == 0xc0) unicode = ((unsigned char*)text)[0] - 0xa0 + 0xff60;
        else unicode = text[0];
    }

    int minx, maxx, miny, maxy, advanced;
    TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, unicode,
                      &minx, &maxx, &miny, &maxy, &advanced );
    
    SDL_Surface *tmp_surface0 = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );
    SDL_Surface *tmp_surface = SDL_ConvertSurface(tmp_surface0, text_surface->format, DEFAULT_SURFACE_FLAG);
    SDL_FreeSurface( tmp_surface0 );

    if ( info->getTateyokoMode() == FontInfo::TATE_MODE && IS_ROTATION_REQUIRED(text) ){
        tmp_surface = rotateSurface90CW(tmp_surface);
        int t;
        t = miny; miny = minx; minx = t;
        t = maxy; maxy = maxx; maxx = t;
    }

    //printf("min %d %d %d %d %d %d\n", minx, maxx, miny, maxy, advanced,TTF_FontAscent((TTF_Font*)info->ttf_font)  );
    dst_rect.x = xy[0] + minx;
    dst_rect.y = xy[1] + TTF_FontAscent((TTF_Font*)info->ttf_font) - maxy;
    if ( info->getTateyokoMode() == FontInfo::TATE_MODE && IS_TRANSLATION_REQUIRED(text) ){
        dst_rect.x += info->font_size_xy[0]/2;
        dst_rect.y -= info->font_size_xy[0]/2;
    }

    if ( shadow_flag ){
        dst_rect.x += shade_distance[0];
        dst_rect.y += shade_distance[1];
    }

    if ( tmp_surface ){
        dst_rect.w = tmp_surface->w;
        dst_rect.h = tmp_surface->h;

        SDL_Rect src_rect;
        src_rect.x = src_rect.y = 0;
        SDL_Surface *tmp2_surface = tmp_surface;
        if ( cache_surface ){
            alphaBlend( cache_surface, dst_rect, cache_surface,
                        tmp_surface, 0, 0,
                        NULL, AnimationInfo::TRANS_ALPHA_MULTIPLE, 256, clip );
            src_rect.x = dst_rect.x;
            src_rect.y = dst_rect.y;
            tmp2_surface = cache_surface;
            if (cache_surface == text_surface)
                loadSubTexture(cache_surface, text_id, &dst_rect);
        }
        if (dst_surface)
            alphaBlend( dst_surface, dst_rect, dst_surface,
                        tmp2_surface, src_rect.x, src_rect.y,
                        NULL, AnimationInfo::TRANS_ALPHA_MULTIPLE, 256, clip );
        
        SDL_FreeSurface( tmp_surface );
    }
}

void ONScripterLabel::drawChar( char* text, FontInfo *info, bool flush_flag, bool lookback_flag, SDL_Surface *surface, SDL_Surface *cache_surface, SDL_Rect *clip )
{
    //printf("draw %x-%x[%s] %d, %d\n", text[0], text[1], text, info->xy[0], info->xy[1] );
    
    if ( info->ttf_font == NULL ){
        if ( info->openFont( font_file, screen_ratio1, screen_ratio2 ) == NULL ){
            fprintf( stderr, "can't open font file: %s\n", font_file );
            quit();
            exit(-1);
        }
    }

    if ( info->isEndOfLine() ){
        info->newLine();
        for (int i=0 ; i<indent_offset ; i++)
            sentence_font.advanceCharInHankaku(2);

        if ( lookback_flag ){
            current_text_buffer->addBuffer( 0x0a );
            for (int i=0 ; i<indent_offset ; i++){
                current_text_buffer->addBuffer(((char*)"�@")[0]);
                current_text_buffer->addBuffer(((char*)"�@")[1]);
            }
        }
    }

    int xy[2];
    xy[0] = info->x() * screen_ratio1 / screen_ratio2;
    xy[1] = info->y() * screen_ratio1 / screen_ratio2;
    
    SDL_Color color;
    SDL_Rect dst_rect;
    if ( info->is_shadow ){
        color.r = color.g = color.b = 0;
        drawGlyph(surface, info, color, text, xy, true, cache_surface, clip, dst_rect);
    }
    color.r = info->color[0];
    color.g = info->color[1];
    color.b = info->color[2];
    drawGlyph( surface, info, color, text, xy, false, cache_surface, clip, dst_rect );

    if ( surface == accumulation_surface &&
         !flush_flag &&
         (!clip || AnimationInfo::doClipping( &dst_rect, clip ) == 0) ){
        dirty_rect.add( dst_rect );
    }
    else if ( flush_flag ){
        info->addShadeArea(dst_rect, shade_distance);
        flushDirect( dst_rect, refresh_shadow_text_mode );
    }

    /* ---------------------------------------- */
    /* Update text buffer */
    if (IS_TWO_BYTE(text[0]))
        info->advanceCharInHankaku(2);
    else
        info->advanceCharInHankaku(1);
    
    if ( lookback_flag ){
        current_text_buffer->addBuffer( text[0] );
        if (IS_TWO_BYTE(text[0]))
            current_text_buffer->addBuffer( text[1] );
    }
}

void ONScripterLabel::drawDoubleChars( char* text, FontInfo *info, bool flush_flag, bool lookback_flag, SDL_Surface *surface, SDL_Surface *cache_surface, SDL_Rect *clip )
{
    char text2[3]= {text[0], '\0', '\0'};
    
    if ( IS_TWO_BYTE(text[0]) ){
        drawChar( text, info, flush_flag, lookback_flag, surface, cache_surface, clip );
    }
    else if (text[1]){
        drawChar( text2, info, flush_flag, lookback_flag, surface, cache_surface, clip );
        text2[0] = text[1];
        drawChar( text2, info, flush_flag, lookback_flag, surface, cache_surface, clip );
    }
    else{
        drawChar( text2, info, flush_flag, lookback_flag, surface, cache_surface, clip );
    }
}

void ONScripterLabel::drawString( const char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect, SDL_Surface *cache_surface )
{
    int i;

    int start_xy[2];
    start_xy[0] = info->xy[0];
    start_xy[1] = info->xy[1];

    /* ---------------------------------------- */
    /* Draw selected characters */
    uchar3 org_color;
    for ( i=0 ; i<3 ; i++ ) org_color[i] = info->color[i];
    for ( i=0 ; i<3 ; i++ ) info->color[i] = color[i];

    bool skip_whitespace_flag = true;
    char text[3] = { '\0', '\0', '\0' };
    while( *str ){
        while (*str == ' ' && skip_whitespace_flag) str++;

#ifdef ENABLE_1BYTE_CHAR
        if ( *str == '`' ){
            str++;
            skip_whitespace_flag = false;
            continue;
        }
#endif            
        if ( IS_TWO_BYTE(*str) ){
            /* Kinsoku process */
            if (info->isEndOfLine(1) && IS_KINSOKU( str+2 )){
                info->newLine();
                for (int i=0 ; i<indent_offset ; i++){
                    sentence_font.advanceCharInHankaku(2);
                }
            }
            text[0] = *str++;
            text[1] = *str++;
            drawChar( text, info, false, false, surface, cache_surface );
        }
        else if (*str == 0x0a){
            info->newLine();
            str++;
        }
        else{
            text[0] = *str++;
            text[1] = '\0';
            drawChar( text, info, false, false, surface, cache_surface );
            if (*str && *str != 0x0a){
                text[0] = *str++;
                drawChar( text, info, false, false, surface, cache_surface );
            }
        }
    }
    for ( i=0 ; i<3 ; i++ ) info->color[i] = org_color[i];

    /* ---------------------------------------- */
    /* Calculate the area of selection */
    SDL_Rect clipped_rect = info->calcUpdatedArea(start_xy, screen_ratio1, screen_ratio2);
    info->addShadeArea(clipped_rect, shade_distance);
    
    if ( flush_flag ) flush( refresh_shadow_text_mode, &clipped_rect );
    
    if ( rect ) *rect = clipped_rect;
}

void ONScripterLabel::refreshText( SDL_Surface *surface, SDL_Rect *clip, int refresh_mode )
{
    if (!(refresh_mode & REFRESH_TEXT_MODE)) return;
    
    SDL_Rect rect = {0, 0, screen_width, screen_height};
    if ( clip ) rect = *clip;
    
    if (refresh_mode & REFRESH_OPENGL_MODE){
        drawTexture( text_id, (Rect&)rect, (Rect&)rect );
    }
    else{
        alphaBlend( surface, rect, surface,
                    text_surface, rect.x, rect.y,
                    NULL, AnimationInfo::TRANS_ALPHA_MULTIPLE );
    }
}

void ONScripterLabel::restoreTextBuffer()
{
    SDL_FillRect( text_surface, NULL, SDL_MapRGBA( text_surface->format, 0, 0, 0, 0 ) );
    loadSubTexture( text_surface, text_id );

    char out_text[3] = { '\0','\0','\0' };
    FontInfo f_info = sentence_font;
    f_info.clear();
    for ( int i=0 ; i<current_text_buffer->buffer2_count ; i++ ){
        if ( current_text_buffer->buffer2[i] == 0x0a ){
            f_info.newLine();
        }
        else{
            out_text[0] = current_text_buffer->buffer2[i];
#ifndef FORCE_1BYTE_CHAR            
            if (out_text[0] == '('){
                startRuby(current_text_buffer->buffer2 + i + 1, f_info);
                continue;
            }
            else if (out_text[0] == '/' && ruby_struct.stage == RubyStruct::BODY ){
                f_info.addMargin(ruby_struct.margin);
                i = ruby_struct.ruby_end - current_text_buffer->buffer2 - 1;
                if (*ruby_struct.ruby_end == ')'){
                    endRuby(false, false, NULL);
                    i++;
                }
                continue;
            }
#endif
            if ( IS_TWO_BYTE(out_text[0]) ){
                out_text[1] = current_text_buffer->buffer2[i+1];
            }
            else{
                out_text[1] = '\0';
                drawChar( out_text, &f_info, false, false, NULL, text_surface );
                
                if (i+1 == current_text_buffer->buffer2_count) break;
                out_text[0] = current_text_buffer->buffer2[i+1];
                if (out_text[0] == 0x0a) continue;
            }
            i++;
            drawChar( out_text, &f_info, false, false, NULL, text_surface );
        }
    }

    Rect rect = {0, 0, screen_width, screen_height};
    drawTexture( text_id, rect, rect );
}

int ONScripterLabel::enterTextDisplayMode()
{
    if (saveon_flag && internal_saveon_flag) saveSaveFile( -1 );
    internal_saveon_flag = false;
    
    if ( !(display_mode & TEXT_DISPLAY_MODE) ){
        if ( event_mode & EFFECT_EVENT_MODE ){
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode = TEXT_DISPLAY_MODE;
                text_on_flag = true;
                return RET_CONTINUE | RET_NOREAD;
            }
            return RET_WAIT | RET_REREAD;
        }
        else{
            next_display_mode = TEXT_DISPLAY_MODE;
            refreshSurface( effect_dst_surface, NULL, refresh_shadow_text_mode );
            dirty_rect.clear();
            dirty_rect.add( sentence_font_info.pos );

            return setEffect( window_effect.effect );
        }
    }
    
    return RET_NOMATCH;
}

int ONScripterLabel::leaveTextDisplayMode()
{
    if ( display_mode & TEXT_DISPLAY_MODE &&
         erase_text_window_mode != 0 ){

        if ( event_mode & EFFECT_EVENT_MODE ){
            if ( doEffect( WINDOW_EFFECT, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
                display_mode = NORMAL_DISPLAY_MODE;
                return RET_CONTINUE | RET_NOREAD;
            }
            return RET_WAIT | RET_REREAD;
        }
        else{
            next_display_mode = NORMAL_DISPLAY_MODE;
            refreshSurface( effect_dst_surface, NULL, REFRESH_NORMAL_MODE );
            dirty_rect.add( sentence_font_info.pos );
            
            return setEffect( window_effect.effect );
        }
    }
    
    return RET_NOMATCH;
}

int ONScripterLabel::clickWait( char *out_text )
{
    if ( (skip_flag || draw_one_page_flag || ctrl_pressed_status) && !textgosub_label ){
        clickstr_state = CLICK_NONE;
        if ( out_text ){
            drawDoubleChars( out_text, &sentence_font, false, true, accumulation_surface, text_surface );
            if (out_text[1]) string_buffer_offset++;
            string_buffer_offset++;
        }
        else{ // called on '@'
            flush(refresh_shadow_text_mode);
            string_buffer_offset++;
        }
        num_chars_in_sentence = 0;
        // ... below is ugly work-around ...
        if ( script_h.getStringBuffer()[string_buffer_offset] == '\0' )
            return RET_CONTINUE;

        return RET_CONTINUE | RET_NOREAD;
    }
    else{
        clickstr_state = CLICK_WAIT;
        key_pressed_flag = false;
        if ( out_text ){
            drawDoubleChars( out_text, &sentence_font, true, true, accumulation_surface, text_surface );
            num_chars_in_sentence++;
        }
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text && out_text[1] ) string_buffer_offset++;
            string_buffer_offset++;

            textgosub_clickstr_state = CLICK_WAIT;
            if (script_h.getNext()[0] == 0x0a)
                textgosub_clickstr_state |= CLICK_EOL;
            gosubReal( textgosub_label, script_h.getNext() );
            return RET_CONTINUE;
        }
        if ( automode_flag ){
            event_mode = WAIT_INPUT_MODE | WAIT_VOICE_MODE;
            if ( automode_time < 0 )
                startTimer( -automode_time * num_chars_in_sentence );
            else
                startTimer( automode_time );
        }
        else if ( autoclick_time > 0 ){
            event_mode = WAIT_SLEEP_MODE;
            startTimer( autoclick_time );
        }
        else{
            event_mode = WAIT_INPUT_MODE | WAIT_TIMER_MODE;
            advancePhase();
        }
        draw_cursor_flag = true;
        num_chars_in_sentence = 0;
        return RET_WAIT | RET_NOREAD;
    }
}

int ONScripterLabel::clickNewPage( char *out_text )
{
    clickstr_state = CLICK_NEWPAGE;
    if ( out_text ){
        drawDoubleChars( out_text, &sentence_font, true, true, accumulation_surface, text_surface );
        num_chars_in_sentence++;
    }
    if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ) flush( refresh_shadow_text_mode );
    
    if ( (skip_flag || ctrl_pressed_status) && !textgosub_label  ){
        event_mode = WAIT_SLEEP_MODE;
        advancePhase();
        num_chars_in_sentence = 0;
    }
    else{
        key_pressed_flag = false;
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text && out_text[1] ) string_buffer_offset++;
            string_buffer_offset++;

            textgosub_clickstr_state = CLICK_NEWPAGE;
            gosubReal( textgosub_label, script_h.getNext() );
            return RET_CONTINUE;
        }
        if ( automode_flag ){
            event_mode = WAIT_INPUT_MODE | WAIT_VOICE_MODE;
            if ( automode_time < 0 )
                startTimer( -automode_time * num_chars_in_sentence );
            else
                startTimer( automode_time );
        }
        else if ( autoclick_time > 0 ){
            event_mode = WAIT_SLEEP_MODE;
            startTimer( autoclick_time );
        }
        else /*if ( cursor_info[ CURSOR_NEWPAGE_NO ].num_of_cells > 0 )*/{
            event_mode = WAIT_INPUT_MODE | WAIT_TIMER_MODE;
            advancePhase();
        }
        draw_cursor_flag = true;
        num_chars_in_sentence = 0;
    }
    return RET_WAIT | RET_NOREAD;
}

void ONScripterLabel::startRuby(char *buf, FontInfo &info)
{
    ruby_struct.stage = RubyStruct::BODY;
    ruby_font = info;
    ruby_font.ttf_font = NULL;
    if ( ruby_struct.font_size_xy[0] != -1 )
        ruby_font.font_size_xy[0] = ruby_struct.font_size_xy[0];
    else
        ruby_font.font_size_xy[0] = info.font_size_xy[0]/2;
    if ( ruby_struct.font_size_xy[1] != -1 )
        ruby_font.font_size_xy[1] = ruby_struct.font_size_xy[1];
    else
        ruby_font.font_size_xy[1] = info.font_size_xy[1]/2;
                
    ruby_struct.body_count = 0;
    ruby_struct.ruby_count = 0;

    while(1){
        if ( *buf == '/' ){
            ruby_struct.stage = RubyStruct::RUBY;
            ruby_struct.ruby_start = buf+1;
        }
        else if ( *buf == ')' || *buf == '\0' ){
            break;
        }
        else{
            if ( ruby_struct.stage == RubyStruct::BODY )
                ruby_struct.body_count++;
            else if ( ruby_struct.stage == RubyStruct::RUBY )
                ruby_struct.ruby_count++;
        }
        buf++;
    }
    ruby_struct.ruby_end = buf;
    ruby_struct.stage = RubyStruct::BODY;
    ruby_struct.margin = ruby_font.initRuby(info, ruby_struct.body_count/2, ruby_struct.ruby_count/2);
}

void ONScripterLabel::endRuby(bool flush_flag, bool lookback_flag, SDL_Surface *surface)
{
    char out_text[3]= {'\0', '\0', '\0'};
    if ( rubyon_flag ){
        ruby_font.clear();
        char *buf = ruby_struct.ruby_start;
        while( buf < ruby_struct.ruby_end ){
            out_text[0] = *buf;
            if ( IS_TWO_BYTE(*buf) ){
                out_text[1] = *(buf+1);
                drawChar( out_text, &ruby_font, flush_flag, lookback_flag, surface, text_surface );
                buf++;
            }
            else{
                out_text[1] = '\0';
                drawChar( out_text, &ruby_font, flush_flag,  lookback_flag, surface, text_surface );
                if ( *(buf+1) ){
                    out_text[1] = *(buf+1);
                    drawChar( out_text, &ruby_font, flush_flag,  lookback_flag, surface, text_surface );
                    buf++;
                }
            }
            buf++;
        }
    }
    ruby_struct.stage = RubyStruct::NONE;
}

int ONScripterLabel::textCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    if (pretextgosub_label && 
        (line_enter_flag == false ||
         (string_buffer_offset == 0 &&
          script_h.getStringBuffer()[string_buffer_offset] == '['))){
        line_enter_flag = true;
        gosubReal( pretextgosub_label, script_h.getCurrent() );
        return RET_CONTINUE;
    }

    //printf("textCommand %c %d %d\n", script_h.getStringBuffer()[ string_buffer_offset ], string_buffer_offset, event_mode);
    char out_text[3]= {'\0', '\0', '\0'};

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE) ){
        draw_cursor_flag = false;
        if ( clickstr_state == CLICK_WAIT ){
            event_mode = IDLE_EVENT_MODE;
            if (script_h.checkClickstr(script_h.getStringBuffer() + string_buffer_offset) != 1) string_buffer_offset++;
            string_buffer_offset++;
            clickstr_state = CLICK_NONE;
            //return RET_CONTINUE;
        }
        else if ( clickstr_state == CLICK_NEWPAGE ){
            event_mode = IDLE_EVENT_MODE;
            if (script_h.checkClickstr(script_h.getStringBuffer() + string_buffer_offset) != 1) string_buffer_offset++;
            string_buffer_offset++;
            newPage( true );
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE | RET_NOREAD;
        }
        else if ( IS_TWO_BYTE(script_h.getStringBuffer()[ string_buffer_offset ]) ){
            string_buffer_offset += 2;
        }
        else if ( script_h.getStringBuffer()[ string_buffer_offset ] == '!' ){
            string_buffer_offset++;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'w' || script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
                string_buffer_offset++;
                while ( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
                        script_h.getStringBuffer()[ string_buffer_offset ] <= '9' )
                    string_buffer_offset++;
            }
        }
        else if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
                  !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) ){
            string_buffer_offset += 2;
        }
        else
            string_buffer_offset++;

        event_mode = IDLE_EVENT_MODE;
    }

    
    if (script_h.getStringBuffer()[string_buffer_offset] == '\0' ||
        script_h.getStringBuffer()[string_buffer_offset] == 0x0a ){
        indent_offset = 0;
        line_enter_flag = false;
        return RET_CONTINUE;
    }

    new_line_skip_flag = false;
    
    //printf("*** textCommand %d (%d,%d)\n", string_buffer_offset, sentence_font.xy[0], sentence_font.xy[1]);

    while( (!(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) &&
            script_h.getStringBuffer()[ string_buffer_offset ] == ' ') ||
           script_h.getStringBuffer()[ string_buffer_offset ] == '\t' ) string_buffer_offset ++;

    char ch = script_h.getStringBuffer()[string_buffer_offset];
    if ( IS_TWO_BYTE(ch) ){ // Shift jis
        /* ---------------------------------------- */
        /* Kinsoku process */
        if (IS_KINSOKU( script_h.getStringBuffer() + string_buffer_offset + 2)){
            int i = 2;
            while (!sentence_font.isEndOfLine(i/2) &&
                   IS_KINSOKU( script_h.getStringBuffer() + string_buffer_offset + i + 2)){
                i += 2;
            }

            if (sentence_font.isEndOfLine(i/2)){
                current_text_buffer->addBuffer( 0x0a );
                sentence_font.newLine();
                for (int i=0 ; i<indent_offset ; i++){
                    current_text_buffer->addBuffer(((char*)"�@")[0]);
                    current_text_buffer->addBuffer(((char*)"�@")[1]);
                    sentence_font.advanceCharInHankaku(2);
                }
            }
        }
        
        out_text[0] = script_h.getStringBuffer()[string_buffer_offset];
        out_text[1] = script_h.getStringBuffer()[string_buffer_offset+1];
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            if (script_h.checkClickstr(&script_h.getStringBuffer()[string_buffer_offset]) > 0){
                if (sentence_font.getRemainingLine() <= clickstr_line)
                    return clickNewPage( out_text );
                else
                    return clickWait( out_text );
            }
            else{
                clickstr_state = CLICK_NONE;
            }
        }
        
        if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ){
            drawChar( out_text, &sentence_font, false, true, accumulation_surface, text_surface );
            num_chars_in_sentence++;
                
            string_buffer_offset += 2;
            return RET_CONTINUE | RET_NOREAD;
        }
        else{
            drawChar( out_text, &sentence_font, true, true, accumulation_surface, text_surface );
            num_chars_in_sentence++;
            event_mode = WAIT_SLEEP_MODE;
            if ( sentence_font.wait_time == -1 )
                advancePhase( default_text_speed[text_speed_no] );
            else
                advancePhase( sentence_font.wait_time );
            return RET_WAIT | RET_NOREAD;
        }
    }
    else if ( ch == '@' ){ // wait for click
        return clickWait( NULL );
    }
    else if ( ch == '\\' ){ // new page
        return clickNewPage( NULL );
    }
    else if ( ch == '_' ){ // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '!' && !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) ){
        string_buffer_offset++;
        if ( script_h.getStringBuffer()[ string_buffer_offset ] == 's' ){
            string_buffer_offset++;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
                sentence_font.wait_time = -1;
                string_buffer_offset++;
            }
            else{
                int t = 0;
                while( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
                       script_h.getStringBuffer()[ string_buffer_offset ] <= '9' ){
                    t = t*10 + script_h.getStringBuffer()[ string_buffer_offset ] - '0';
                    string_buffer_offset++;
                }
                sentence_font.wait_time = t;
            }
        }
        else if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'w' ||
                  script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
            bool flag = false;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ) flag = true;
            string_buffer_offset++;
            int tmp_string_buffer_offset = string_buffer_offset;
            int t = 0;
            while( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
                   script_h.getStringBuffer()[ string_buffer_offset ] <= '9' ){
                t = t*10 + script_h.getStringBuffer()[ string_buffer_offset ] - '0';
                string_buffer_offset++;
            }
            if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ){
                return RET_CONTINUE | RET_NOREAD;
            }
            else{
                event_mode = WAIT_SLEEP_MODE;
                if ( flag ) event_mode |= WAIT_INPUT_MODE;
                key_pressed_flag = false;
                startTimer( t );
                string_buffer_offset = tmp_string_buffer_offset - 2;
                return RET_WAIT | RET_NOREAD;
            }
        }
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '#' && !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) ){
        readColor( &sentence_font.color, script_h.getStringBuffer() + string_buffer_offset );
        readColor( &ruby_font.color, script_h.getStringBuffer() + string_buffer_offset );
        string_buffer_offset += 7;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '(' && !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR)){
        current_text_buffer->addBuffer('(');
        startRuby( script_h.getStringBuffer() + string_buffer_offset + 1, sentence_font );
        
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '/' && !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) ){
        if ( ruby_struct.stage == RubyStruct::BODY ){
            current_text_buffer->addBuffer('/');
            sentence_font.addMargin(ruby_struct.margin);
            string_buffer_offset = ruby_struct.ruby_end - script_h.getStringBuffer();
            if (*ruby_struct.ruby_end == ')'){
                if ( skip_flag || draw_one_page_flag || ctrl_pressed_status )
                    endRuby(false, true, accumulation_surface);
                else
                    endRuby(true, true, accumulation_surface);
                current_text_buffer->addBuffer(')');
                string_buffer_offset++;
            }

            return RET_CONTINUE | RET_NOREAD;
        }
        else{ // skip new line
            new_line_skip_flag = true;
            //string_buffer_offset++;
            return RET_CONTINUE; // skip the following eol
        }
    }
    else{
        out_text[0] = ch;
        
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            int matched_len = script_h.checkClickstr(script_h.getStringBuffer() + string_buffer_offset);

            if (matched_len > 0){
                if (matched_len == 2) out_text[1] = script_h.getStringBuffer()[ string_buffer_offset + 1 ];
                if (sentence_font.getRemainingLine() <= clickstr_line)
                    return clickNewPage( out_text );
                else
                    return clickWait( out_text );
            }
            else if (script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
                     script_h.checkClickstr(&script_h.getStringBuffer()[string_buffer_offset+1]) == 1){
                if ( script_h.getStringBuffer()[ string_buffer_offset + 2 ] &&
                     script_h.checkClickstr(&script_h.getStringBuffer()[string_buffer_offset+2]) > 0){
                    clickstr_state = CLICK_NONE;
                }
                else if (script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '@'){
                    return clickWait( out_text );
                }
                else if (script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '\\'){
                    return clickNewPage( out_text );
                }
                else{
                    out_text[1] = script_h.getStringBuffer()[ string_buffer_offset + 1 ];
                    if (sentence_font.getRemainingLine() <= clickstr_line)
                        return clickNewPage( out_text );
                    else
                        return clickWait( out_text );
                }
            }
            else{
                clickstr_state = CLICK_NONE;
            }
        }
        
        bool flush_flag = true;
        if ( skip_flag || draw_one_page_flag || ctrl_pressed_status )
            flush_flag = false;
        if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
             !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR))
            out_text[1] = script_h.getStringBuffer()[ string_buffer_offset + 1 ];

        drawDoubleChars( out_text, &sentence_font, flush_flag, true, accumulation_surface, text_surface );
        num_chars_in_sentence++;
        if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ){
            if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
                 !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR))
                string_buffer_offset++;
            string_buffer_offset++;
            return RET_CONTINUE | RET_NOREAD;
        }
        else{
            event_mode = WAIT_SLEEP_MODE;
            if ( sentence_font.wait_time == -1 )
                advancePhase( default_text_speed[text_speed_no] );
            else
                advancePhase( sentence_font.wait_time );
            return RET_WAIT | RET_NOREAD;
        }
    }

    return RET_NOMATCH;
}

