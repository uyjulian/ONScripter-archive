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

void ONScripterLabel::drawChar( char* text, struct FontInfo *info, bool flush_flag, SDL_Surface *surface, bool buffering_flag )
{
    int xy[2];
    SDL_Rect rect;
    SDL_Surface *tmp_surface = NULL;
    SDL_Color color;
    unsigned short index, unicode;
    int minx, maxx, miny, maxy, advanced;

    //printf("draw %x-%x[%s] %d, %d\n", text[0], text[1], text, system_menu_enter_flag, buffering_flag );

    //if ( !surface ) surface = text_surface;
    
    if ( !info->font_valid_flag && info->ttf_font ){
        TTF_CloseFont( (TTF_Font*)info->ttf_font );
        info->ttf_font = NULL;
    }
    if ( info->ttf_font == NULL ){
        info->ttf_font = TTF_OpenFont( font_name, info->font_size );
        if ( !info->ttf_font ){
            fprintf( stderr, "can't open font file: %s\n", font_name );
            SDL_Quit();
            exit(-1);
        }
        info->font_valid_flag = true;
    }

    if ( text[0] & 0x80 ){
        index = ((unsigned char*)text)[0];
        index = index << 8 | ((unsigned char*)text)[1];
        unicode = convSJIS2UTF16( index );
    }
    else{
        unicode = text[0];
    }

    TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, unicode,
                      &minx, &maxx, &miny, &maxy, &advanced );
    //printf("minx %d maxx %d miny %d maxy %d ace %d\n",minx, maxx, miny, maxy, advanced );
    
    if ( info->xy[0] >= info->num_xy[0] ){
        info->xy[0] = 0;
        info->xy[1]++;
    }
    xy[0] = info->xy[0] * info->pitch_xy[0] + info->top_xy[0];
    xy[1] = info->xy[1] * info->pitch_xy[1] + info->top_xy[1];
    
    rect.x = xy[0] + 1 + minx;
    rect.y = xy[1] + TTF_FontAscent( (TTF_Font*)info->ttf_font ) - maxy;
    if ( info->display_shadow ){
        color.r = color.g = color.b = 0;
        tmp_surface = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );
        rect.w = tmp_surface->w;
        rect.h = tmp_surface->h;
        if ( surface )
            SDL_BlitSurface( tmp_surface, NULL, surface, &rect );
        SDL_FreeSurface( tmp_surface );
    }

    color.r = info->color[0];
    color.g = info->color[1];
    color.b = info->color[2];

    tmp_surface = TTF_RenderGlyph_Blended( (TTF_Font*)info->ttf_font, unicode, color );
    rect.x--;
    rect.w = tmp_surface->w;
    rect.h = tmp_surface->h;
    if ( surface )
        SDL_BlitSurface( tmp_surface, NULL, surface, &rect );
    if ( tmp_surface ) SDL_FreeSurface( tmp_surface );
    
    if ( flush_flag ) flush( rect.x, rect.y, rect.w + 1, rect.h );

    /* ---------------------------------------- */
    /* Update text buffer */
    if ( !system_menu_enter_flag && buffering_flag ){
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 ] = text[0];
        current_text_buffer->buffer[ (info->xy[1] * info->num_xy[0] + info->xy[0]) * 2 + 1 ] = text[1];
    }
    info->xy[0] ++;
    if ( !system_menu_enter_flag && buffering_flag ){
        current_text_buffer->xy[0] = info->xy[0];
        current_text_buffer->xy[1] = info->xy[1];
    }
}

void ONScripterLabel::drawString( char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect, bool buffering_flag )
{
    int i, current_text_xy[2];
    uchar3 org_color;
    char text[3] = { '\0', '\0', '\0' };

    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];

    /* ---------------------------------------- */
    /* Draw selected characters */
    for ( i=0 ; i<3 ; i++ ) org_color[i] = info->color[i];
    for ( i=0 ; i<3 ; i++ ) info->color[i] = color[i];

    while( *str ){
        if ( *str & 0x80 ){
            /* Kinsoku process */
            if ( info->xy[0] + 1 == info->num_xy[0] &&
                 IS_KINSOKU( str+2 ) ) {
                info->xy[0] = 0;
                info->xy[1]++;
            }
            text[0] = *str++;
            text[1] = *str++;
        }
        else{
            text[0] = *str++;
            text[1] = '\0';
        }
        drawChar( text, info, flush_flag, surface, buffering_flag );
    }
    for ( i=0 ; i<3 ; i++ ) info->color[i] = org_color[i];

    /* ---------------------------------------- */
    /* Calculate the area of the selection */
    if ( rect ){
        if ( current_text_xy[1] == info->xy[1] ){
            rect->x = info->top_xy[0] + current_text_xy[0] * info->pitch_xy[0];
            rect->w = info->pitch_xy[0] * (info->xy[0] - current_text_xy[0] + 1);
        }
        else{
            rect->x = info->top_xy[0];
            rect->w = info->pitch_xy[0] * info->num_xy[0];
        }
        rect->y = current_text_xy[1] * info->pitch_xy[1] + info->top_xy[1];// - info->pitch_xy[1] + 3;
        rect->h = (info->xy[1] - current_text_xy[1] + 1) * info->pitch_xy[1];
    }
}

void ONScripterLabel::restoreTextBuffer( SDL_Surface *surface )
{
    int i, end;
    int xy[2];
    char out_text[3] = { '\0','\0','\0' };

    if ( !surface ) surface = text_surface;

    xy[0] = sentence_font.xy[0];
    xy[1] = sentence_font.xy[1];
    sentence_font.xy[0] = 0;
    sentence_font.xy[1] = 0;
    end = current_text_buffer->xy[1] * current_text_buffer->num_xy[0] + current_text_buffer->xy[0];
    for ( i=0 ; i<current_text_buffer->num_xy[1] * current_text_buffer->num_xy[0] ; i++ ){
        if ( sentence_font.xy[1] * current_text_buffer->num_xy[0] + sentence_font.xy[0] >= end ) break;
        out_text[0] = current_text_buffer->buffer[ i * 2 ];
        out_text[1] = current_text_buffer->buffer[ i * 2 + 1];
        drawChar( out_text, &sentence_font, false, surface );
    }
    sentence_font.xy[0] = xy[0];
    sentence_font.xy[1] = xy[1];
    if ( xy[0] == 0 ) text_char_flag = false;
    else              text_char_flag = true;
}

int ONScripterLabel::clickWait( char *out_text )
{
    if ( skip_flag || draw_one_page_flag ){
        clickstr_state = CLICK_NONE;
        if ( out_text ){
            drawChar( out_text, &sentence_font, false, text_surface );
            string_buffer_offset += 2;
        }
        else{
            flush();
            string_buffer_offset++;
        }
        return RET_CONTINUE;
    }
    else{
        clickstr_state = CLICK_WAIT;
        if ( out_text ) drawChar( out_text, &sentence_font, true, text_surface );
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else if ( cursor_info[ CURSOR_WAIT_NO ].tag.num_of_cells > 0 ){
            startCursor( CLICK_WAIT );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
        return RET_WAIT;
    }
}

int ONScripterLabel::clickNewPage( char *out_text )
{
    clickstr_state = CLICK_NEWPAGE;
    if ( out_text ) drawChar( out_text, &sentence_font, false, text_surface );
    if ( skip_flag || draw_one_page_flag ) flush();
    
    if ( skip_flag ){
        event_mode = WAIT_SLEEP_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }
    else{
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        if ( autoclick_timer > 0 ){
            event_mode |= WAIT_SLEEP_MODE;
            startTimer( autoclick_timer );
        }
        else if ( cursor_info[ CURSOR_NEWPAGE_NO ].tag.num_of_cells > 0 ){
            startCursor( CLICK_NEWPAGE );
            startTimer( MINIMUM_TIMER_RESOLUTION );
        }
    }
    return RET_WAIT;
}

int ONScripterLabel::textCommand( char *text )
{
    int i, j, ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;
    
    char out_text[20];
    char *p_string_buffer;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE) ){
        if ( clickstr_state == CLICK_WAIT ){
            event_mode = IDLE_EVENT_MODE;
            if ( string_buffer[ string_buffer_offset ] != '@' ) current_link_label_info->offset = ++string_buffer_offset;
            current_link_label_info->offset = ++string_buffer_offset;
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE;
        }
        else if ( clickstr_state == CLICK_NEWPAGE ){
            event_mode = IDLE_EVENT_MODE;
            if ( string_buffer[ string_buffer_offset ] != '\\' ) current_link_label_info->offset = ++string_buffer_offset;
            current_link_label_info->offset = ++string_buffer_offset;
            newPage( true );
            new_line_skip_flag = true;
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE;
        }
        else if ( string_buffer[ string_buffer_offset ] & 0x80 ){
            string_buffer_offset += 2;
        }
        else if ( string_buffer[ string_buffer_offset ] == '!' ){
            string_buffer_offset++;
            if ( string_buffer[ string_buffer_offset ] == 'w' || string_buffer[ string_buffer_offset ] == 'd' ){
                string_buffer_offset++;
                p_string_buffer = &string_buffer[ string_buffer_offset ];
                readInt( &p_string_buffer );
                string_buffer_offset = p_string_buffer - string_buffer;
            }
        }
        else{
            string_buffer_offset++;
        }

        event_mode = IDLE_EVENT_MODE;
        current_link_label_info->offset = string_buffer_offset;
    }

    if ( string_buffer[ string_buffer_offset ] == '\0' ) return RET_CONTINUE;
    new_line_skip_flag = false;
    
    //printf("*** textCommand %d %d(%d) %s\n", string_buffer_offset, sentence_font.xy[0], sentence_font.pitch_xy[0], string_buffer + string_buffer_offset );
    
    while( string_buffer[ string_buffer_offset ] == ' ' || string_buffer[ string_buffer_offset ] == '\t' ) string_buffer ++;
    char ch = string_buffer[ string_buffer_offset ];
    if ( ch & 0x80 ){ // Shift jis
        text_char_flag = true;
        /* ---------------------------------------- */
        /* Kinsoku process */
        if ( sentence_font.xy[0] + 1 == sentence_font.num_xy[0] &&
             IS_KINSOKU( string_buffer + string_buffer_offset + 2 ) ){
            sentence_font.xy[0] = 0;
            sentence_font.xy[1]++;
        }
        
        out_text[0] = string_buffer[ string_buffer_offset ];
        out_text[1] = string_buffer[ string_buffer_offset + 1 ];
        out_text[2] = '\0';
        if ( clickstr_state == CLICK_IGNORE ){
            clickstr_state = CLICK_NONE;
        }
        else{
            clickstr_state = CLICK_NONE;
            for ( i=0 ; i<clickstr_num ; i++ ){
                if ( clickstr_list[i*2] == out_text[0] && clickstr_list[i*2+1] == out_text[1] ){
                    if ( sentence_font.xy[1] >= sentence_font.num_xy[1] - clickstr_line ){
                        if ( string_buffer[ string_buffer_offset + 2 ] != '@' && string_buffer[ string_buffer_offset + 2 ] != '\\' ){
                            clickstr_state = CLICK_NEWPAGE;
                        }
                    }
                    else{
                        if ( string_buffer[ string_buffer_offset + 2 ] != '@' && string_buffer[ string_buffer_offset + 2 ] != '\\' ){
                            clickstr_state = CLICK_WAIT;
                        }
                    }
                    for ( j=0 ; j<clickstr_num ; j++ ){
                        if ( clickstr_list[j*2] == string_buffer[ string_buffer_offset + 2 ] &&
                             clickstr_list[j*2+1] == string_buffer[ string_buffer_offset + 3 ] ){
                            clickstr_state = CLICK_NONE;
                        }
                    }
                    if ( string_buffer[ string_buffer_offset + 2 ] == '!' ){
                        if ( string_buffer[ string_buffer_offset + 3 ] == 'w' ||
                             string_buffer[ string_buffer_offset + 3 ] == 'd' )
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
                return RET_CONTINUE;
            }
            else{
                drawChar( out_text, &sentence_font, true, text_surface );
                event_mode = WAIT_SLEEP_MODE;
                startTimer( sentence_font.wait_time );
                return RET_WAIT;
            }
        }
    }
    else if ( ch == '@' ){ // wait for click
        return clickWait( NULL );
    }
    else if ( ch == '/' ){ // skip new line
        new_line_skip_flag = true;
        string_buffer_offset++;
        return RET_CONTINUE;
    }
    else if ( ch == '\\' ){ // new page
        return clickNewPage( NULL );
    }
    else if ( ch == '!' ){
        string_buffer_offset++;
        if ( string_buffer[ string_buffer_offset ] == 's' ){
            string_buffer_offset++;
            if ( string_buffer[ string_buffer_offset ] == 'd' ){
                sentence_font.wait_time = default_text_speed[ text_speed_no ];
                string_buffer_offset++;
            }
            else{
                p_string_buffer = &string_buffer[ string_buffer_offset ];
                sentence_font.wait_time = readInt( &p_string_buffer );
                string_buffer_offset = p_string_buffer - string_buffer;
            }
        }
        else if ( string_buffer[ string_buffer_offset ] == 'w' || string_buffer[ string_buffer_offset ] == 'd' ){
            bool flag = false;
            if ( string_buffer[ string_buffer_offset ] == 'd' ) flag = true;
            string_buffer_offset++;
            p_string_buffer = &string_buffer[ string_buffer_offset ];
            int t = readInt( &p_string_buffer );
            if ( skip_flag || draw_one_page_flag ){
                string_buffer_offset = p_string_buffer - string_buffer;
                return RET_CONTINUE;
            }
            else{
                event_mode = WAIT_SLEEP_MODE;
                if ( flag ) event_mode |= WAIT_INPUT_MODE;
                key_pressed_flag = false;
                startTimer( t );
                string_buffer_offset -= 2;
                return RET_WAIT;
            }
        }
        return RET_CONTINUE;
    }
    else if ( ch == '_' ){ // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE;
    }
    else if ( ch == '\0' ){ // End of line
        printf("end of text\n");
        return RET_CONTINUE;
    }
    else if ( ch == '#' ){
        readColor( &sentence_font.color, string_buffer + string_buffer_offset + 1 );
        string_buffer_offset += 7;
        return RET_CONTINUE;
    }
    else{
        printf("unrecognized text %x\n",ch);
        text_char_flag = true;
        out_text[0] = ch;
        out_text[1] = '\0';
        if ( skip_flag || draw_one_page_flag || sentence_font.wait_time == 0){
            drawChar( out_text, &sentence_font, false, text_surface );
            string_buffer_offset++;
            return RET_CONTINUE;
        }
        else{
            drawChar( out_text, &sentence_font, true, text_surface );
            event_mode = WAIT_SLEEP_MODE;
            startTimer( sentence_font.wait_time );
            printf("dispatch timer %d\n",sentence_font.wait_time);
            return RET_WAIT;
        }
    }

    return RET_NOMATCH;
}
