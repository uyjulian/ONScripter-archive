/* -*- C++ -*-
 * 
 *  ONScripter_animation.cpp - Methods to manipulate AnimationInfo
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

void ONScripterLabel::showAnimation( AnimationInfo *anim )
{
    if ( anim->duration_list ){
        if ( anim->image_surface ){
            SDL_Rect src_rect, dst_rect;
            src_rect.x = anim->image_surface->w * anim->current_cell / anim->num_of_cells;
            src_rect.y = 0;
            src_rect.w = anim->pos.w;
            src_rect.h = anim->pos.h;
            if ( anim->abs_flag ) {
                dst_rect.x = anim->pos.x;
                dst_rect.y = anim->pos.y;
            }
            else{
                dst_rect.x = sentence_font.xy[0] * sentence_font.pitch_xy[0] + sentence_font.top_xy[0] + anim->pos.x;
                dst_rect.y = sentence_font.xy[1] * sentence_font.pitch_xy[1] + sentence_font.top_xy[1] + anim->pos.y;
            }
            alphaBlend( text_surface, dst_rect.x, dst_rect.y,
                        anim->preserve_surface, 0, 0, anim->pos.w, anim->pos.h,
                        anim->image_surface, src_rect.x, src_rect.y,
                        anim->alpha_offset, 0, -anim->trans_mode );
            flush( dst_rect.x, dst_rect.y, src_rect.w, src_rect.h );
        }

        anim->proceedAnimation();
    }
}

void ONScripterLabel::setupAnimationInfo( AnimationInfo *anim )
{
    anim->deleteSurface();
    anim->abs_flag = true;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){

        FontInfo f_info = sentence_font;
        f_info.xy[0] = f_info.xy[1] = 0;
        f_info.top_xy[0] = anim->pos.x;
        f_info.top_xy[1] = anim->pos.y;

        drawString( anim->file_name, anim->color_list[ anim->current_cell ], &f_info, true, NULL, &anim->pos );
        sentence_font.font_valid_flag = f_info.font_valid_flag;
        sentence_font.ttf_font = f_info.ttf_font;
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
            if ( anim->preserve_surface ) SDL_FreeSurface( anim->preserve_surface );
            anim->preserve_surface = SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG, anim->pos.w, anim->pos.h, 32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( anim->preserve_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
        }
    }
}

void ONScripterLabel::parseTaggedString( AnimationInfo *anim )
{
    //printf(" parseTaggeString %s\n", buffer);
    anim->removeTag();
    
    int i;
    char *buffer = anim->image_name;
    anim->num_of_cells = 1;
    anim->trans_mode = trans_mode;

    if ( buffer[0] == ':' ){
        buffer++;
        if ( buffer[0] == 'a' ){ // alpha blend
            anim->trans_mode = AnimationInfo::TRANS_ALPHA;
            buffer++;
        }
        else if ( buffer[0] == 'l' ){ // top left
            anim->trans_mode = AnimationInfo::TRANS_TOPLEFT;
            buffer++;
        }
        else if ( buffer[0] == 'r' ){ // top right
            anim->trans_mode = AnimationInfo::TRANS_TOPRIGHT;
            buffer++;
        }
        else if ( buffer[0] == 'c' ){ // copy
            anim->trans_mode = AnimationInfo::TRANS_COPY;
            buffer++;
        }
        else if ( buffer[0] == 's' ){ // string
            anim->trans_mode = AnimationInfo::TRANS_STRING;
            buffer++;
            i=0;
            anim->num_of_cells = 0;
            while( buffer[i] == '#' ){
                anim->num_of_cells++;
                i += 7;
            }
            anim->color_list = new uchar3[ anim->num_of_cells ];
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                readColor( &anim->color_list[i], buffer + 1 );
                buffer += 7;
            }
        }
        else if ( buffer[0] == 'm' ){ // mask
            anim->trans_mode = AnimationInfo::TRANS_MASK;
            buffer++;
            readStr( &buffer, tmp_string_buffer );
            setStr( &anim->mask_file_name, tmp_string_buffer );
        }
        else if ( buffer[0] == '#' ){ // direct color
            anim->trans_mode = AnimationInfo::TRANS_DIRECT;
            memcpy( anim->direct_color, buffer, 7 );
            anim->direct_color[7] = '\0';
            buffer += 7;
        }
        else if ( buffer[0] == '!' ){ // pallet number
            anim->trans_mode = AnimationInfo::TRANS_PALLET;
            buffer++;
            anim->pallet_number = 0;
            while ( *buffer >= '0' && *buffer <= '9' )
                anim->pallet_number = anim->pallet_number * 10 + *buffer++ - '0';
        }
    }

    if ( buffer[0] == '/' ){
        buffer++;
        anim->num_of_cells = 0;
        while ( *buffer >= '0' && *buffer <= '9' )
            anim->num_of_cells = anim->num_of_cells * 10 + *buffer++ - '0';
        buffer++;
        if ( anim->num_of_cells == 0 ){
            fprintf( stderr, "ONScripterLabel::parseTaggedString  The number of cells is 0\n");
            return;
        }

        anim->duration_list = new int[ anim->num_of_cells ];

        if ( *buffer == '<' ){
            buffer++;
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                anim->duration_list[i] = 0;
                while ( *buffer >= '0' && *buffer <= '9' )
                    anim->duration_list[i] = anim->duration_list[i] * 10 + *buffer++ - '0';
                buffer++;
            }
            buffer++; // skip '>'
        }
        else{
            anim->duration_list[0] = 0;
            while ( *buffer >= '0' && *buffer <= '9' )
                anim->duration_list[0] = anim->duration_list[0] * 10 + *buffer++ - '0';
            for ( i=1 ; i<anim->num_of_cells ; i++ )
                anim->duration_list[i] = anim->duration_list[0];
            buffer++;
        }
        
        anim->loop_mode = *buffer++ - '0'; // 3...no animation
    }

    if ( buffer[0] == ';' ) buffer++;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING && buffer[0] == '$' ){
        buffer++;
        setStr( &anim->file_name, str_variables[ readInt( &buffer ) ] );
    }
    else{
        setStr( &anim->file_name, buffer );
    }
}

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, AnimationInfo *anim, SDL_Rect *clip )
{
    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){

        if ( clip ){
            if ( anim->pos.x + anim->pos.w <= clip->x ||
                 clip->x + clip->w <= anim->pos.x ||
                 anim->pos.y + anim->pos.h <= clip->y ||
                 clip->y + clip->h <= anim->pos.y )
                return;
        }
        FontInfo f_info = sentence_font;
        f_info.xy[0] = f_info.xy[1] = 0;
        f_info.top_xy[0] = anim->pos.x;
        f_info.top_xy[1] = anim->pos.y;

        drawString( anim->file_name, anim->color_list[ anim->current_cell ], &f_info, false, dst_surface, NULL );
        sentence_font.font_valid_flag = f_info.font_valid_flag;
        sentence_font.ttf_font = f_info.ttf_font;
        return;
    }
    else if ( !anim->image_surface ) return;

    int offset = (anim->pos.w + anim->alpha_offset) * anim->current_cell;

    if ( anim->trans_mode == AnimationInfo::TRANS_ALPHA ||
         anim->trans_mode == AnimationInfo::TRANS_TOPLEFT ||
         anim->trans_mode == AnimationInfo::TRANS_TOPRIGHT ){

        alphaBlend( dst_surface, anim->pos.x, anim->pos.y,
                    dst_surface, anim->pos.x, anim->pos.y, anim->pos.w, anim->pos.h,
                    anim->image_surface, offset, 0,
                    offset + anim->alpha_offset, 0, -anim->trans_mode, 0, clip );
    }
    else if ( anim->trans_mode == AnimationInfo::TRANS_COPY ){
        SDL_Rect src_rect;
        
        src_rect.x = offset;
        src_rect.y = 0;
        src_rect.w = anim->pos.w;
        src_rect.h = anim->pos.h;
        SDL_BlitSurface( anim->image_surface, &src_rect, dst_surface, &anim->pos );
    }
}
