/* -*- C++ -*-
 * 
 *  ONScripterLabel_text.cpp - Text parser of ONScripter
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
        ( !(*(x) & 0x80) || \
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
    if ( text[0] & 0x80 ){
        unsigned index = ((unsigned char*)text)[0];
        index = index << 8 | ((unsigned char*)text)[1];
        unicode = convSJIS2UTF16( index );
    }
    else if ( text[1] ){
        unicode = text[1];
    }
    else{
        unicode = text[0];
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
        if ( lookback_flag )
            current_text_buffer->addBuffer( 0x0a );
    }

    int xy[2];
    xy[0] = info->x(!(text[0] & 0x80) && text[1]) * screen_ratio1 / screen_ratio2;
    xy[1] = info->y(!(text[0] & 0x80) && text[1]) * screen_ratio1 / screen_ratio2;
    
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
         (!clip || doClipping( &dst_rect, clip ) == 0) ){
        dirty_rect.add( dst_rect );
    }
    else if ( flush_flag ){
        info->addShadeArea(dst_rect, shade_distance);
        flushDirect( dst_rect, REFRESH_SHADOW_TEXT_MODE );
    }

    /* ---------------------------------------- */
    /* Update text buffer */
    info->advanceChar();
    if ( lookback_flag ){
        current_text_buffer->addBuffer( text[0] );
        if ( text[0] & 0x80 || text[1] )
            current_text_buffer->addBuffer( text[1] );
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

    char text[3] = { '\0', '\0', '\0' };
    while( *str ){
        if ( *str == ' ' ){
            str++;
            continue;
        }
        if ( *str & 0x80 ){
            /* Kinsoku process */
            if (info->isEndOfLine(1) && IS_KINSOKU( str+2 )) info->newLine();
            text[0] = *str++;
            text[1] = *str++;
            drawChar( text, info, false, false, surface, cache_surface );
        }
        else{
            text[0] = *str++;
            if ( !(text[0] & 0x80) ){
                text[1] = '\0';
                drawChar( text, info, false, false, surface, cache_surface );
                info->advanceChar(-1);
            }
            if ((text[0] & 0x80) || *str){
                text[1] = *str++;
                drawChar( text, info, false, false, surface, cache_surface );
            }
        }
    }
    for ( i=0 ; i<3 ; i++ ) info->color[i] = org_color[i];

    /* ---------------------------------------- */
    /* Calculate the area of selection */
    SDL_Rect clipped_rect = info->calcUpdatedArea(start_xy, screen_ratio1, screen_ratio2);
    info->addShadeArea(clipped_rect, shade_distance);
    
    if ( flush_flag ) flush( REFRESH_SHADOW_TEXT_MODE, &clipped_rect );
    
    if ( rect ) *rect = clipped_rect;
}

void ONScripterLabel::refreshText( SDL_Surface *surface, SDL_Rect *clip, int refresh_mode )
{
    if (!(refresh_mode & REFRESH_TEXT_MODE)) return;
    
    SDL_Rect rect = {0, 0, screen_width, screen_height};
    if ( clip ) rect = *clip;
    
    if (refresh_mode & REFRESH_OPENGL_MODE){
        drawTexture( text_id, rect, rect );
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
            if ( !(out_text[0] & 0x80) ){
                out_text[1] = '\0';
                drawChar( out_text, &f_info, false, false, NULL, text_surface );
                f_info.advanceChar(-1);
            }
            if (i+1 == current_text_buffer->buffer2_count) break;
            out_text[1] = current_text_buffer->buffer2[i+1];
            if (out_text[1] == 0x0a) continue;
            i++;
            drawChar( out_text, &f_info, false, false, NULL, text_surface );
        }
    }

    SDL_Rect rect = {0, 0, screen_width, screen_height};
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
            refreshSurface( effect_dst_surface, NULL, REFRESH_SHADOW_TEXT_MODE );
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
            drawChar( out_text, &sentence_font, false, true, accumulation_surface, text_surface );
            string_buffer_offset += 2;
        }
        else{ // called on '@'
            flush(REFRESH_SHADOW_TEXT_MODE);
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
        if ( sentence_font.wait_time == 0 ||
             ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ) flush(REFRESH_SHADOW_TEXT_MODE);
        if ( out_text ){
            drawChar( out_text, &sentence_font, true, true, accumulation_surface, text_surface );
            num_chars_in_sentence++;
        }
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text ) string_buffer_offset += 2;
            else            string_buffer_offset++;

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
        drawChar( out_text, &sentence_font, false, true, accumulation_surface, text_surface );
        num_chars_in_sentence++;
    }
    if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ||
         ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ||
         ctrl_pressed_status ) flush( REFRESH_SHADOW_TEXT_MODE );
    
    if ( (skip_flag || ctrl_pressed_status) && !textgosub_label  ){
        event_mode = WAIT_SLEEP_MODE;
        advancePhase();
        num_chars_in_sentence = 0;
    }
    else{
        key_pressed_flag = false;
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text ) string_buffer_offset += 2;
            else            string_buffer_offset++;

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

int ONScripterLabel::textCommand()
{
    int i, ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    //printf("textCommand %c %d %d\n", script_h.getStringBuffer()[ string_buffer_offset ], string_buffer_offset, event_mode);
    char out_text[3]= {'\0', '\0', '\0'};

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE) ){
        draw_cursor_flag = false;
        if ( clickstr_state == CLICK_WAIT ){
            event_mode = IDLE_EVENT_MODE;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] != '@' ) string_buffer_offset++;
            string_buffer_offset++;
            clickstr_state = CLICK_NONE;
            //return RET_CONTINUE;
        }
        else if ( clickstr_state == CLICK_NEWPAGE ){
            event_mode = IDLE_EVENT_MODE;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] != '\\' ) string_buffer_offset++;
            string_buffer_offset++;
            newPage( true );
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE | RET_NOREAD;
        }
        else if ( script_h.getStringBuffer()[ string_buffer_offset ] & 0x80 ){
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
                  !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR &&
                    (script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '@' ||
                     script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '\\'))){
            string_buffer_offset += 2;
        }
        else
            string_buffer_offset++;

        event_mode = IDLE_EVENT_MODE;
    }

    if ( script_h.getStringBuffer()[string_buffer_offset] == 0x0a ||
         script_h.getStringBuffer()[string_buffer_offset] == '\0' ) return RET_CONTINUE;

    new_line_skip_flag = false;
    
    //printf("*** textCommand %d (%d,%d)\n", string_buffer_offset, sentence_font.xy[0], sentence_font.xy[1]);

    while( (!(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) &&
            script_h.getStringBuffer()[ string_buffer_offset ] == ' ') ||
           script_h.getStringBuffer()[ string_buffer_offset ] == '\t' ) string_buffer_offset ++;

    char ch = script_h.getStringBuffer()[string_buffer_offset];
    if ( ch & 0x80 ){ // Shift jis
        /* ---------------------------------------- */
        /* Kinsoku process */
        if (IS_KINSOKU( script_h.getStringBuffer() + string_buffer_offset + 2)){
            int i = 2;
            while (!sentence_font.isEndOfLine(i/2) &&
                   IS_KINSOKU( script_h.getStringBuffer() + string_buffer_offset + i + 2)){
                i += 2;
            }

            if (sentence_font.isEndOfLine(i/2)){
                sentence_font.newLine();
                current_text_buffer->addBuffer( 0x0a );
            }
        }
        
        out_text[0] = script_h.getStringBuffer()[string_buffer_offset];
        out_text[1] = script_h.getStringBuffer()[string_buffer_offset+1];
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            if (script_h.checkClickstr(&script_h.getStringBuffer()[string_buffer_offset])){
                if ( sentence_font.xy[1] >= sentence_font.num_xy[1] - clickstr_line )
                    return clickNewPage( out_text );
                else
                    return clickWait( out_text );
            }
            else{
                clickstr_state = CLICK_NONE;
            }
        }
        
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ||
             ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ||
             ctrl_pressed_status ){
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
    else if ( ch == '(' ){
        int i=0;

        while( (ch = script_h.getStringBuffer()[string_buffer_offset+i]) != '\0' ){
            if ( ch == '(' ){
                ruby_struct.stage = RubyStruct::BODY;
                ruby_font = sentence_font;
                ruby_font.ttf_font = NULL;
                if ( ruby_struct.font_size_xy[0] != -1 )
                    ruby_font.font_size_xy[0] = ruby_struct.font_size_xy[0];
                else
                    ruby_font.font_size_xy[0] = sentence_font.font_size_xy[0]/2;
                if ( ruby_struct.font_size_xy[1] != -1 )
                    ruby_font.font_size_xy[1] = ruby_struct.font_size_xy[1];
                else
                    ruby_font.font_size_xy[1] = sentence_font.font_size_xy[1]/2;
                
                ruby_struct.body_count = 0;
                ruby_struct.ruby_count = 0;
                ruby_struct.ruby_end = 0;
            }
            else if ( ch == '/' ){
                ruby_struct.stage = RubyStruct::RUBY;
                ruby_struct.ruby_start = string_buffer_offset + i + 1;
            }
            else if ( ch == ')' ){
                ruby_struct.ruby_end = string_buffer_offset + i;
                break;
            }
            else{
                if ( ruby_struct.stage == RubyStruct::BODY )
                    ruby_struct.body_count++;
                else if ( ruby_struct.stage == RubyStruct::RUBY )
                    ruby_struct.ruby_count++;
            }
            i++;
        }
        ruby_struct.stage = RubyStruct::BODY;
        ruby_struct.margin = ruby_font.initRuby(sentence_font, ruby_struct.body_count/2, ruby_struct.ruby_count/2);
        
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == ')' ){
        if ( rubyon_flag ){
            bool flush_flag = true;
            if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ||
                 ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ||
                 ctrl_pressed_status )
                flush_flag = false;
        
            ruby_font.clear();
            for ( i=ruby_struct.ruby_start ; i<string_buffer_offset ; i++ ){
                out_text[0] = script_h.getStringBuffer()[i];
                if ( script_h.getStringBuffer()[i] & 0x80 ){
                    out_text[1] = script_h.getStringBuffer()[i+1];
                    drawChar( out_text, &ruby_font, flush_flag, false, accumulation_surface, text_surface );
                    i++;
                }
                else{
                    out_text[1] = '\0';
                    drawChar( out_text, &ruby_font, flush_flag, false, accumulation_surface, text_surface );
                    if ( script_h.getStringBuffer()[i+1] ){
                        out_text[1] = script_h.getStringBuffer()[i+1];
                        drawChar( out_text, &ruby_font, flush_flag, false, accumulation_surface, text_surface );
                        i++;
                    }
                }
            }
        }
        ruby_struct.stage = RubyStruct::NONE;
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '@' ){ // wait for click
        return clickWait( NULL );
    }
    else if ( ch == '/' ){
        if ( ruby_struct.stage == RubyStruct::BODY ){
            sentence_font.addMargin(ruby_struct.margin);
            if ( ruby_struct.ruby_end != 0 ){
                ruby_struct.stage = RubyStruct::RUBY;
                string_buffer_offset = ruby_struct.ruby_end;
            }
            else{
                ruby_struct.stage = RubyStruct::NONE;
                while( script_h.getStringBuffer()[string_buffer_offset] != '\0' )
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
    else if ( ch == '\\' ){ // new page
        return clickNewPage( NULL );
    }
    else if ( ch == '!' ){
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
    else if ( ch == '_' ){ // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if ( ch == '\0' ){ // End of line
        printf("end of text\n");
        return RET_CONTINUE;
    }
    else if ( ch == '#' ){
        readColor( &sentence_font.color, script_h.getStringBuffer() + string_buffer_offset );
        readColor( &ruby_font.color, script_h.getStringBuffer() + string_buffer_offset );
        string_buffer_offset += 7;
        return RET_CONTINUE | RET_NOREAD;
    }
    else{
        bool flush_flag = true;
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ||
             ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ||
             ctrl_pressed_status )
            flush_flag = false;
        out_text[0] = ch;
        if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
             !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR &&
               (script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '@' ||
                script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '\\'))){
            drawChar( out_text, &sentence_font, flush_flag, false, accumulation_surface, text_surface );
            sentence_font.advanceChar(-1);
            out_text[1] = script_h.getStringBuffer()[ string_buffer_offset + 1 ];
            drawChar( out_text, &sentence_font, flush_flag, true, accumulation_surface, text_surface );
        }
        else{
            drawChar( out_text, &sentence_font, flush_flag, true, accumulation_surface, text_surface );
            sentence_font.advanceChar(-1);
        }
        num_chars_in_sentence++;
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ||
             ( sentence_font.wait_time == -1 && default_text_speed[text_speed_no] == 0 ) ||
             ctrl_pressed_status ){
            if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] &&
                 !(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR &&
                   (script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '@' ||
                    script_h.getStringBuffer()[ string_buffer_offset + 1 ] == '\\'))){
                string_buffer_offset++;
            }
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
