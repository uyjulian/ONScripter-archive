/* -*- C++ -*-
 * 
 *  ONScripter_animation.cpp - Methods to manipulate AnimationInfo
 *
 *  Copyright (c) 2001-2003 Ogapee. All rights reserved.
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

int ONScripterLabel::proceedAnimation()
{
    int i, minimum_duration = -1;
    AnimationInfo *anim;
    
    for ( i=0 ; i<3 ; i++ ){
        anim = &tachi_info[i];
        if ( anim->visible && anim->is_animatable ){
            minimum_duration = estimateNextDuration( anim, anim->pos, minimum_duration );
        }
    }

    for ( i=MAX_SPRITE_NUM-1 ; i>=0 ; i-- ){
        anim = &sprite_info[i];
        if ( anim->visible && anim->is_animatable ){
            minimum_duration = estimateNextDuration( anim, anim->pos, minimum_duration );
        }
    }

    if ( !textgosub_label &&
         ( clickstr_state == CLICK_WAIT ||
           clickstr_state == CLICK_NEWPAGE ) ){
        
        if      ( clickstr_state == CLICK_WAIT )
            anim = &cursor_info[CURSOR_WAIT_NO];
        else if ( clickstr_state == CLICK_NEWPAGE )
            anim = &cursor_info[CURSOR_NEWPAGE_NO];

        if ( anim->visible && anim->is_animatable ){
            SDL_Rect dst_rect = anim->pos;
            if ( !anim->abs_flag ){
                dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
                dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
            }

            minimum_duration = estimateNextDuration( anim, dst_rect, minimum_duration );
        }
    }
    flush();

    if ( minimum_duration == -1 ) minimum_duration = 0;

    return minimum_duration;
}

int ONScripterLabel::estimateNextDuration( AnimationInfo *anim, SDL_Rect &rect, int minimum )
{
    if ( anim->remaining_time == 0 ){
        if ( minimum == -1 ||
             minimum > anim->duration_list[ anim->current_cell ] )
            minimum = anim->duration_list[ anim->current_cell ];
        if ( anim->proceedAnimation() )
            refreshSurface( accumulation_surface, &rect, REFRESH_SHADOW_MODE | REFRESH_CURSOR_MODE );
    }
    else{
        if ( minimum == -1 ||
             minimum > anim->remaining_time )
            minimum = anim->remaining_time;
    }

    return minimum;
}

void ONScripterLabel::resetRemainingTime( int t )
{
    int i;
    AnimationInfo *anim;
    
    for ( i=0 ; i<3 ; i++ ){
        anim = &tachi_info[i];
        if ( anim->visible && anim->is_animatable){
            anim->remaining_time -= t;
        }
    }
        
    for ( i=MAX_SPRITE_NUM-1 ; i>=0 ; i-- ){
        anim = &sprite_info[i];
        if ( anim->visible && anim->is_animatable ){
            anim->remaining_time -= t;
        }
    }

    if ( !textgosub_label &&
         ( clickstr_state == CLICK_WAIT ||
           clickstr_state == CLICK_NEWPAGE ) ){
        if ( clickstr_state == CLICK_WAIT )
            anim = &cursor_info[CURSOR_WAIT_NO];
        else if ( clickstr_state == CLICK_NEWPAGE )
            anim = &cursor_info[CURSOR_NEWPAGE_NO];
        
        if ( anim->visible && anim->is_animatable ){
            anim->remaining_time -= t;
        }
    }
}

void ONScripterLabel::setupAnimationInfo( AnimationInfo *anim )
{
    anim->deleteSurface();
    anim->abs_flag = true;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){

        FontInfo f_info = sentence_font;
        f_info.xy[0] = f_info.xy[1] = 0;
        f_info.top_xy[0] = anim->pos.x * screen_ratio2 / screen_ratio1;
        f_info.top_xy[1] = anim->pos.y * screen_ratio2 / screen_ratio1;
        f_info.num_xy[0] = strlen( anim->file_name );
        f_info.num_xy[1] = 1;

        if ( anim->font_size_xy[0] >= 0 ){
            f_info.font_size_xy[0] = f_info.pitch_xy[0] = anim->font_size_xy[0];
            f_info.font_size_xy[1] = f_info.pitch_xy[1] = anim->font_size_xy[1];
            if ( anim->font_pitch >= 0 )
                f_info.pitch_xy[0] = anim->font_pitch;
            f_info.ttf_font = NULL;
        }

        drawString( anim->file_name, anim->color_list[ anim->current_cell ], &f_info, false, NULL, &anim->pos );
        anim->image_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, anim->pos.w*anim->num_of_cells, anim->pos.h, 32, rmask, gmask, bmask, amask );
        f_info.top_xy[0] = f_info.top_xy[1] = 0;
        for ( int i=0 ; i<anim->num_of_cells ; i++ ){
            f_info.xy[0] = f_info.xy[1] = 0;
            drawString( anim->file_name, anim->color_list[ i ], &f_info, false, anim->image_surface );
            f_info.top_xy[0] += anim->pos.w * screen_ratio2 / screen_ratio1;
        }
    }
    else{
        anim->image_surface = loadImage( anim->file_name );

        if ( anim->image_surface ){
            anim->pos.w = anim->image_surface->w / anim->num_of_cells;
            anim->pos.h = anim->image_surface->h;
            if ( anim->trans_mode == AnimationInfo::TRANS_ALPHA ){
                anim->pos.w /= 2;
                anim->alpha_offset = anim->pos.w;
            }
            else{
                anim->alpha_offset = 0;
            }

            if ( anim->trans_mode == AnimationInfo::TRANS_MASK )
                anim->mask_surface = loadImage( anim->mask_file_name );
        }
    }
}

void ONScripterLabel::parseTaggedString( AnimationInfo *anim )
{
    anim->removeTag();
    
    int i;
    char *buffer = anim->image_name;
    anim->num_of_cells = 1;
    anim->trans_mode = trans_mode;

    if ( buffer[0] == ':' ){
        buffer++;
        if ( buffer[0] == 'a' ){
            anim->trans_mode = AnimationInfo::TRANS_ALPHA;
            buffer++;
        }
        else if ( buffer[0] == 'l' ){
            anim->trans_mode = AnimationInfo::TRANS_TOPLEFT;
            buffer++;
        }
        else if ( buffer[0] == 'r' ){
            anim->trans_mode = AnimationInfo::TRANS_TOPRIGHT;
            buffer++;
        }
        else if ( buffer[0] == 'c' ){
            anim->trans_mode = AnimationInfo::TRANS_COPY;
            buffer++;
        }
        else if ( buffer[0] == 's' ){
            anim->trans_mode = AnimationInfo::TRANS_STRING;
            buffer++;
            anim->num_of_cells = 0;
            if ( buffer[0] == '/' ){
                buffer++;
                script_h.pushCurrent( buffer );
                anim->font_size_xy[0] = script_h.readInt();
                anim->font_size_xy[1] = script_h.readInt();
                anim->font_pitch = script_h.readInt() + anim->font_size_xy[0];
                if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
                    script_h.readInt(); // 0 ... normal, 1 ... no anti-aliasing, 2 ... Fukuro
                }
                buffer = script_h.getNext();
                script_h.popCurrent();
                buffer++;
            }
            else{
                anim->font_size_xy[0] = sentence_font.font_size_xy[0];
                anim->font_size_xy[1] = sentence_font.font_size_xy[1];
                anim->font_pitch = sentence_font.pitch_xy[0];
            }
            i=0;
            while( buffer[i] == '#' ){
                anim->num_of_cells++;
                i += 7;
            }
            anim->color_list = new uchar3[ anim->num_of_cells ];
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                readColor( &anim->color_list[i], buffer );
                buffer += 7;
            }
        }
        else if ( buffer[0] == 'm' ){
            anim->trans_mode = AnimationInfo::TRANS_MASK;
            buffer++;
            script_h.pushCurrent( buffer );
            script_h.readToken();
            setStr( &anim->mask_file_name, script_h.getStringBuffer() );
            buffer = script_h.getNext();
            script_h.popCurrent();
        }
        else if ( buffer[0] == '#' ){
            anim->trans_mode = AnimationInfo::TRANS_DIRECT;
            readColor( &anim->direct_color, buffer );
            buffer += 7;
        }
        else if ( buffer[0] == '!' ){
            anim->trans_mode = AnimationInfo::TRANS_PALLET;
            buffer++;
            anim->pallet_number = getNumberFromBuffer( (const char**)&buffer );
        }
        else{
            buffer++; // skip an illegal trans_mode
        }
    }

    if ( buffer[0] == '/' ){
        buffer++;
        anim->num_of_cells = getNumberFromBuffer( (const char**)&buffer );
        buffer++;
        if ( anim->num_of_cells == 0 ){
            fprintf( stderr, "ONScripterLabel::parseTaggedString  The number of cells is 0\n");
            return;
        }

        anim->duration_list = new int[ anim->num_of_cells ];

        if ( *buffer == '<' ){
            buffer++;
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                anim->duration_list[i] = getNumberFromBuffer( (const char**)&buffer );
                buffer++;
            }
            buffer++; // skip '>'
        }
        else{
            anim->duration_list[0] = getNumberFromBuffer( (const char**)&buffer );
            for ( i=1 ; i<anim->num_of_cells ; i++ )
                anim->duration_list[i] = anim->duration_list[0];
            buffer++;
        }
        
        anim->loop_mode = *buffer++ - '0'; // 3...no animation
        if ( anim->loop_mode != 3 ) anim->is_animatable = true;
    }

    if ( buffer[0] == ';' ) buffer++;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING && buffer[0] == '$' ){
        script_h.pushCurrent( buffer );
        setStr( &anim->file_name, script_h.readStr() );
        script_h.popCurrent();
    }
    else{
        setStr( &anim->file_name, buffer );
    }
}

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, AnimationInfo *anim, SDL_Rect *clip )
{
    int cell = anim->current_cell;
    if ( cell >= anim->num_of_cells )
        cell = anim->num_of_cells - 1;
    
    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){
        alphaBlend( dst_surface, anim->pos,
                    dst_surface, anim->pos.x, anim->pos.y,
                    anim->image_surface, anim->image_surface->w / anim->num_of_cells * cell, 0, 
                    NULL, anim->image_surface->w / anim->num_of_cells * cell, AnimationInfo::TRANS_ALPHA_PRESERVE, 255, clip );
        return;
    }
    else if ( !anim->image_surface ) return;

    int offset = (anim->pos.w + anim->alpha_offset) * cell;

    SDL_Rect dst_rect = anim->pos;
    if ( !anim->abs_flag ){
        dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    alphaBlend( dst_surface, dst_rect,
                dst_surface, dst_rect.x, dst_rect.y,
                anim->image_surface, offset, 0,
                anim->mask_surface, offset + anim->alpha_offset,
                anim->trans_mode, anim->trans, clip, &anim->direct_color );
}

void ONScripterLabel::stopAnimation( int click )
{
    int no;

    if ( !(event_mode & WAIT_TIMER_MODE) ) return;
    
    event_mode &= ~WAIT_TIMER_MODE;
    remaining_time = -1;
    if ( textgosub_label ) return;

    if      ( click == CLICK_WAIT )    no = CURSOR_WAIT_NO;
    else if ( click == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
    else return;
    
    SDL_Rect dst_rect = cursor_info[ no ].pos;

    if ( !cursor_info[ no ].abs_flag ){
        dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    
    refreshSurface( accumulation_surface, &dst_rect, REFRESH_SHADOW_MODE );

    flush( &dst_rect, false, true );
}
