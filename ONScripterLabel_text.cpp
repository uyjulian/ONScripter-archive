/* -*- C++ -*-
 * 
 *  ONScripterLabel_text.cpp - Text parser of ONScripter
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

extern unsigned short convSJIS2UTF16( unsigned short in );

#define IS_KINSOKU(x)	\
        ( ( *(x) & 0x80 ) && \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x41 ) || \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x42 ) || \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x48 ) || \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x49 ) || \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x76 ) || \
          ( *(x) == (char)0x81 && *((x)+1) == (char)0x5b ) )

inline void ONScripterLabel::drawGlyph( SDL_Surface *dst_surface, char *text, FontInfo *info, SDL_Color &color, unsigned short unicode, int xy[2], int minx, int maxy, int shadow_offset, bool flush_flag, SDL_Rect *clip )
{
    SDL_Rect dst_rect, src_rect, clipped_rect;
    SDL_Surface *tmp_surface;
    
    dst_rect.x = xy[0] + shadow_offset + minx;
    if ( !(text[0] & 0x80) && text[1] ) dst_rect.x += info->pitch_xy[0] / 2 * screen_ratio1 / screen_ratio2;
    dst_rect.y = xy[1] + TTF_FontAscent( (TTF_Font*)info->ttf_font ) + shadow_offset - maxy;
    src_rect.x = src_rect.y = 0;
    
    tmp_surface = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );

    if ( tmp_surface ){
        dst_rect.w = src_rect.w = tmp_surface->w;
        dst_rect.h = src_rect.h = tmp_surface->h;
        if ( dst_surface ){
            bool out_of_region = false;
            if ( clip ){
                if ( doClipping( &dst_rect, clip, &clipped_rect ) ){
                    out_of_region = true;
                }
                else{
                    src_rect.x += clipped_rect.x;
                    src_rect.y += clipped_rect.y;
                }
            }
            if ( !out_of_region )
                SDL_BlitSurface( tmp_surface, &src_rect, dst_surface, &dst_rect );
        }
        SDL_FreeSurface( tmp_surface );
    }

    if ( flush_flag ) flush( dst_rect.x, dst_rect.y, src_rect.w + 1, src_rect.h + 1 );
}

void ONScripterLabel::drawChar( char* text, FontInfo *info, bool flush_flag, SDL_Surface *surface, bool buffering_flag, SDL_Rect *clip )
{
    int xy[2];
    SDL_Color color;
    unsigned short index, unicode;
    int minx, maxx, miny, maxy, advanced;
    char *p_text;

    //printf("draw %x-%x[%s] %d, %d\n", text[0], text[1], text, info->xy[0], info->xy[1] );

    if ( info->ttf_font == NULL ){
#if 0 // defined(PDA)
        // reduce memory uaage
        sentence_font.closeFont();
        system_font.closeFont();
        menu_font.closeFont();
#endif        
        if ( info->openFont( font_file, screen_ratio1, screen_ratio2 ) == NULL ){
            fprintf( stderr, "can't open font file: %s\n", font_file );
            SDL_Quit();
            exit(-1);
        }
    }

    if ( text[0] & 0x80 ){
        index = ((unsigned char*)text)[0];
        index = index << 8 | ((unsigned char*)text)[1];
        unicode = convSJIS2UTF16( index );
        TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, unicode,
                          &minx, &maxx, &miny, &maxy, &advanced );
    }
    else if ( text[1] ){
        p_text = &text[1];
        unicode = text[1];
        TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, text[1],
                          &minx, &maxx, &miny, &maxy, &advanced );
    }
    else{
        p_text = &text[0];
        unicode = text[0];
        TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, text[0],
                          &minx, &maxx, &miny, &maxy, &advanced );
    }

    //printf("minx %d maxx %d miny %d maxy %d ace %d\n",minx, maxx, miny, maxy, advanced );
    
    if ( info->xy[0] >= info->num_xy[0] ){
        info->xy[0] = 0;
        info->xy[1]++;
    }
    xy[0] = info->x( tateyoko_mode ) * screen_ratio1 / screen_ratio2;
    xy[1] = info->y( tateyoko_mode ) * screen_ratio1 / screen_ratio2;
    
    if ( info->is_shadow ){
        color.r = color.g = color.b = 0;
        drawGlyph( surface, text, info, color, unicode, xy, minx, maxy, 1, false, clip );
    }
    
    color.r = info->color[0];
    color.g = info->color[1];
    color.b = info->color[2];
    drawGlyph( surface, text, info, color, unicode, xy, minx, maxy, 0, flush_flag, clip );

    /* ---------------------------------------- */
    /* Update text buffer */
    if ( !system_menu_enter_flag && buffering_flag ){
        if ( info->num_xy[0] > current_text_buffer->num_xy[0] ||
             info->num_xy[1] > current_text_buffer->num_xy[1] ||
             info->xy[0] >= current_text_buffer->num_xy[0] ||
             info->xy[1] >= current_text_buffer->num_xy[1] ){
            return;
        }
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 ] = text[0];
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 + 1 ] = text[1];
    }
    info->xy[0] ++;
    if ( !system_menu_enter_flag && buffering_flag ){
        current_text_buffer->xy[0] = info->xy[0];
        current_text_buffer->xy[1] = info->xy[1];
    }
}

void ONScripterLabel::drawString( const char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect, bool buffering_flag, SDL_Rect *clip )
{
    int i, tmp_xy[2];
    uchar3 org_color;
    char text[3] = { '\0', '\0', '\0' };
    SDL_Rect clipped_rect;

    tmp_xy[0] = info->xy[0];
    tmp_xy[1] = info->xy[1];

    /* ---------------------------------------- */
    /* Draw selected characters */
    for ( i=0 ; i<3 ; i++ ) org_color[i] = info->color[i];
    for ( i=0 ; i<3 ; i++ ) info->color[i] = color[i];

    while( *str ){
        if ( *str == ' ' ){
            str++;
            continue;
        }
        if ( *str & 0x80 ){
            /* Kinsoku process */
            if ( info->xy[0] + 1 == info->num_xy[0] &&
                 IS_KINSOKU( str+2 ) ) {
                info->xy[0] = 0;
                info->xy[1]++;
            }
            text[0] = *str++;
            text[1] = *str++;
            drawChar( text, info, false, surface, buffering_flag, clip );
        }
        else{
            text[0] = *str++;
            text[1] = '\0';
            drawChar( text, info, false, surface, buffering_flag, clip );
            info->xy[0]--;
            if ( *str ){
                text[1] = *str++;
                drawChar( text, info, false, surface, buffering_flag, clip );
            }
        }
    }
    for ( i=0 ; i<3 ; i++ ) info->color[i] = org_color[i];

    /* ---------------------------------------- */
    /* Calculate the area of selection */
    if ( tateyoko_mode == 1 ){
        clipped_rect.x = (info->top_xy[0] + (info->num_xy[1] - tmp_xy[1] - 1) * info->pitch_xy[0]) * screen_ratio1 / screen_ratio2;
        clipped_rect.w = (info->pitch_xy[0] * (info->xy[1] - tmp_xy[1] + 1)) * screen_ratio1 / screen_ratio2;
        if ( tmp_xy[1] == info->xy[1] ){
            clipped_rect.y = (info->top_xy[1] + tmp_xy[0] * info->pitch_xy[1]) * screen_ratio1 / screen_ratio2;
            clipped_rect.h = (info->pitch_xy[1] * (info->xy[0] - tmp_xy[0] + 1)) * screen_ratio1 / screen_ratio2;
        }
        else{
            clipped_rect.y = info->top_xy[1] * screen_ratio1 / screen_ratio2;
            clipped_rect.h = info->pitch_xy[1] * info->num_xy[0] * screen_ratio1 / screen_ratio2;
        }
    }
    else{
        if ( tmp_xy[1] == info->xy[1] ){
            clipped_rect.x = (info->top_xy[0] + tmp_xy[0] * info->pitch_xy[0]) * screen_ratio1 / screen_ratio2;
            clipped_rect.w = (info->pitch_xy[0] * (info->xy[0] - tmp_xy[0] + 1)) * screen_ratio1 / screen_ratio2;
        }
        else{
            clipped_rect.x = info->top_xy[0] * screen_ratio1 / screen_ratio2;
            clipped_rect.w = info->pitch_xy[0] * info->num_xy[0] * screen_ratio1 / screen_ratio2;
        }
        clipped_rect.y = (tmp_xy[1] * info->pitch_xy[1] + info->top_xy[1]) * screen_ratio1 / screen_ratio2;
        clipped_rect.h = (info->xy[1] - tmp_xy[1] + 1) * info->pitch_xy[1] * screen_ratio1 / screen_ratio2;
    }

    if ( flush_flag ) flush( &clipped_rect );
    
    if ( rect ) *rect = clipped_rect;
}

void ONScripterLabel::restoreTextBuffer( SDL_Surface *surface )
{
    int i;
    char out_text[3] = { '\0','\0','\0' };

    if ( !surface ) surface = text_surface;

    FontInfo f_info = sentence_font;
    f_info.xy[0] = 0;
    f_info.xy[1] = 0;
    for ( i=0 ; i<current_text_buffer->num_xy[1] * current_text_buffer->num_xy[0] ; i++ ){
        out_text[0] = current_text_buffer->buffer[ i * 2 ];
        if ( out_text[0] == 0x0 ){
            out_text[0] = ((char*)"�@")[0];
            out_text[1] = ((char*)"�@")[1];
        }
        else{
            if ( !(out_text[0] & 0x80) ){
                out_text[1] = '\0';
                drawChar( out_text, &f_info, false, surface, false );
                f_info.xy[0]--;
            }
            out_text[1] = current_text_buffer->buffer[ i * 2 + 1];
        }
        drawChar( out_text, &f_info, false, surface );
    }

    sentence_font.ttf_font = f_info.ttf_font;
}

int ONScripterLabel::clickWait( char *out_text )
{
    if ( skip_flag || draw_one_page_flag ){
        clickstr_state = CLICK_NONE;
        if ( out_text ){
            drawChar( out_text, &sentence_font, false, text_surface );
            string_buffer_offset += 2;
        }
        else{ // called on '@'
            flush();
            string_buffer_offset++;
        }
        // ... below is ugly work-around ...
        if ( script_h.getStringBuffer()[string_buffer_offset] == '\0' )
            return RET_CONTINUE;

        return RET_CONTINUE_NOREAD;
    }
    else{
        clickstr_state = CLICK_WAIT;
        key_pressed_flag = false;
        if ( sentence_font.wait_time == 0 ) flush();
        if ( out_text ) drawChar( out_text, &sentence_font, true, text_surface );
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text ) string_buffer_offset += 2;
            else            string_buffer_offset++;
            if ( script_h.getStringBuffer()[string_buffer_offset] == '\0' ){
                script_h.readToken();
                string_buffer_offset = 0;
            }
            gosubReal( textgosub_label, true, script_h.getCurrent() );
            return RET_JUMP;
        }
        event_mode = WAIT_INPUT_MODE;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else /*if ( cursor_info[ CURSOR_WAIT_NO ].num_of_cells > 0 )*/{
            event_mode |= WAIT_ANIMATION_MODE;
            advancePhase();
        }
        return RET_WAIT_NOREAD;
    }
}

int ONScripterLabel::clickNewPage( char *out_text )
{
    clickstr_state = CLICK_NEWPAGE;
    if ( out_text ) drawChar( out_text, &sentence_font, false, text_surface );
    if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ) flush();
    
    if ( skip_flag ){
        event_mode = WAIT_SLEEP_MODE;
        advancePhase();
    }
    else{
        key_pressed_flag = false;
        if ( textgosub_label ){
            saveoffCommand();
            if ( out_text ) string_buffer_offset += 2;
            else            string_buffer_offset++;
            if ( script_h.getStringBuffer()[string_buffer_offset] == '\0' ){
                script_h.readToken();
                string_buffer_offset = 0;
            }
            gosubReal( textgosub_label, false, script_h.getCurrent() );
            new_line_skip_flag = true;
            return RET_JUMP;
        }
        event_mode = WAIT_INPUT_MODE;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else /*if ( cursor_info[ CURSOR_NEWPAGE_NO ].num_of_cells > 0 )*/{
            event_mode |= WAIT_ANIMATION_MODE;
            advancePhase();
        }
    }
    return RET_WAIT_NOREAD;
}

int ONScripterLabel::textCommand()
{
    int i, j, ret = enterTextDisplayMode();// RET_WAIT_NOREAD );
    if ( ret != RET_NOMATCH ) return ret;
    
    char out_text[3]= {'\0', '\0', '\0'};

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE) ){
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
            new_line_skip_flag = true;
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE_NOREAD;
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
        else if ( script_h.getStringBuffer()[ string_buffer_offset+1 ] ){
            string_buffer_offset += 2;
        }
        else
            string_buffer_offset++;

        event_mode = IDLE_EVENT_MODE;
        //string_buffer_offset;
    }

    if ( script_h.getStringBuffer()[string_buffer_offset] == '\0' ) return RET_CONTINUE;
    new_line_skip_flag = false;
    
    //printf("*** textCommand %d (%d,%d) %s\n", string_buffer_offset, sentence_font.xy[0], sentence_font.xy[1], string_buffer + string_buffer_offset );
    
    while( script_h.getStringBuffer()[ string_buffer_offset ] == ' ' ||
           script_h.getStringBuffer()[ string_buffer_offset ] == '\t' ) string_buffer_offset ++;
    char ch = script_h.getStringBuffer()[string_buffer_offset];
    if ( ch & 0x80 ){ // Shift jis
        /* ---------------------------------------- */
        /* Kinsoku process */
        if ( sentence_font.xy[0] + 1 == sentence_font.num_xy[0] &&
             IS_KINSOKU( script_h.getStringBuffer() + string_buffer_offset + 2 ) ){
            sentence_font.xy[0] = 0;
            sentence_font.xy[1]++;
        }
        
        out_text[0] = script_h.getStringBuffer()[string_buffer_offset];
        out_text[1] = script_h.getStringBuffer()[string_buffer_offset+1];
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            clickstr_state = CLICK_NONE;
            for ( i=0 ; i<clickstr_num ; i++ ){
                if ( clickstr_list[i*2] == out_text[0] && clickstr_list[i*2+1] == out_text[1] ){
                    if ( sentence_font.xy[1] >= sentence_font.num_xy[1] - clickstr_line ){
                        if ( script_h.getStringBuffer()[string_buffer_offset+2] != '@' && script_h.getStringBuffer()[string_buffer_offset+2] != '\\' ){
                            clickstr_state = CLICK_NEWPAGE;
                        }
                    }
                    else{
                        if ( script_h.getStringBuffer()[string_buffer_offset+2] != '@' && script_h.getStringBuffer()[string_buffer_offset+2] != '\\' ){
                            clickstr_state = CLICK_WAIT;
                        }
                    }
                    for ( j=0 ; j<clickstr_num ; j++ ){
                        if ( clickstr_list[j*2] == script_h.getStringBuffer()[string_buffer_offset+2] &&
                             clickstr_list[j*2+1] == script_h.getStringBuffer()[string_buffer_offset+3] ){
                            clickstr_state = CLICK_NONE;
                        }
                    }
                    if ( script_h.getStringBuffer()[string_buffer_offset+2] == '!' ){
                        if ( script_h.getStringBuffer()[string_buffer_offset+3] == 'w' ||
                             script_h.getStringBuffer()[string_buffer_offset+3] == 'd' )
                            clickstr_state = CLICK_NONE;
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
            if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0 ){
                drawChar( out_text, &sentence_font, false, text_surface );
                string_buffer_offset += 2;
                return RET_CONTINUE_NOREAD;
            }
            else{
                drawChar( out_text, &sentence_font, true, text_surface );
                event_mode = WAIT_SLEEP_MODE;
                startTimer( sentence_font.wait_time );
                return RET_WAIT_NOREAD;
            }
        }
    }
    else if ( ch == '@' ){ // wait for click
        return clickWait( NULL );
    }
    else if ( ch == '/' ){ // skip new line
        new_line_skip_flag = true;
        string_buffer_offset++;
#if 0
        if ( script_h.isQuat() )
            return RET_CONTINUE;
        else
#endif
            return RET_CONTINUE_NOREAD;
    }
    else if ( ch == '\\' ){ // new page
        return clickNewPage( NULL );
    }
    else if ( ch == '!' ){
        string_buffer_offset++;
        if ( script_h.getStringBuffer()[ string_buffer_offset ] == 's' ){
            string_buffer_offset++;
            if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
                sentence_font.wait_time = default_text_speed[ text_speed_no ];
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
        else if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'w' || script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
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
            if ( skip_flag || draw_one_page_flag ){
                return RET_CONTINUE_NOREAD;
            }
            else{
                event_mode = WAIT_SLEEP_MODE;
                if ( flag ) event_mode |= WAIT_INPUT_MODE;
                key_pressed_flag = false;
                startTimer( t );
                string_buffer_offset = tmp_string_buffer_offset - 2;
                return RET_WAIT;
            }
        }
        return RET_CONTINUE_NOREAD;
    }
    else if ( ch == '_' ){ // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE_NOREAD;
    }
    else if ( ch == '\0' ){ // End of line
        printf("end of text\n");
        return RET_CONTINUE;
    }
    else if ( ch == '#' ){
        readColor( &sentence_font.color, script_h.getStringBuffer() + string_buffer_offset );
        string_buffer_offset += 7;
        return RET_CONTINUE_NOREAD;
    }
    else{
        bool flush_flag = true;
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0)
            flush_flag = false;
        out_text[0] = ch;
        drawChar( out_text, &sentence_font, flush_flag, text_surface );
        sentence_font.xy[0]--;
        if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] ){
            out_text[1] = script_h.getStringBuffer()[ string_buffer_offset + 1 ];
            drawChar( out_text, &sentence_font, flush_flag, text_surface );
        }
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0){
            if ( script_h.getStringBuffer()[ string_buffer_offset + 1 ] ) string_buffer_offset++;
            string_buffer_offset++;
            return RET_CONTINUE_NOREAD;
        }
        else{
            event_mode = WAIT_SLEEP_MODE;
            startTimer( sentence_font.wait_time );
            return RET_WAIT_NOREAD;
        }
    }

    return RET_NOMATCH;
}
