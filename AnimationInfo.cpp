/* -*- C++ -*-
 * 
 *  AnimationInfo.cpp - General image storage class of ONScripter
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

#include "AnimationInfo.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//#define ENABLE_BILINEAR

AnimationInfo::AnimationInfo(){
    /* Variables from TaggedInfo */
    current_cell = 0;
    num_of_cells = 0;
    direction = 1;
    duration_list = NULL;
    color_list = NULL;
    file_name = NULL;
    mask_file_name = NULL;
    is_animatable = false;
    pos.x = pos.y = 0;

    /* Variables from AnimationInfo */
    visible = false;
    image_name = NULL;
    image_surface = NULL;
    mask_surface = NULL;
    trans = 256;

    font_size_xy[0] = font_size_xy[1] = -1;
    font_pitch = -1;
    remaining_time = 0;
}

AnimationInfo::~AnimationInfo(){
    remove();
}

void AnimationInfo::deleteImageName(){
    if ( image_name ) delete[] image_name;
    image_name = NULL;
}

void AnimationInfo::setImageName( const char *name ){
    deleteImageName();
    image_name = new char[ strlen(name) + 1 ];
    strcpy( image_name, name );
}

void AnimationInfo::deleteSurface(){
    if ( image_surface ) SDL_FreeSurface( image_surface );
    image_surface = NULL;
    if ( mask_surface ) SDL_FreeSurface( mask_surface );
    mask_surface = NULL;
}

void AnimationInfo::remove(){
    trans = 256;
    deleteImageName();
    deleteSurface();
    removeTag();
}

void AnimationInfo::removeTag(){
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
    if ( mask_file_name ){
        delete[] mask_file_name;
        mask_file_name = NULL;
    }
    current_cell = 0;
    num_of_cells = 0;
    remaining_time = 0;
    is_animatable = false;

    color[0] = color[1] = color[2] = 0;
}

// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
bool AnimationInfo::proceedAnimation()
{
    bool is_changed = false;
    
    if ( loop_mode != 3 && num_of_cells > 1 ){
        current_cell += direction;
        is_changed = true;
    }

    if ( current_cell < 0 ){ // loop_mode must be 2
        current_cell = 1;
        direction = 1;
    }
    else if ( current_cell >= num_of_cells ){
        if ( loop_mode == 0 ){
            current_cell = 0;
        }
        else if ( loop_mode == 1 ){
            current_cell = num_of_cells - 1;
            is_changed = false;
        }
        else{
            current_cell = num_of_cells - 2;
            direction = -1;
        }
    }

    remaining_time = duration_list[ current_cell ];

    return is_changed;
}

void AnimationInfo::setCell(int cell)
{
    if (cell < 0) cell = 0;
    else if (cell >= num_of_cells) cell = num_of_cells - 1;

    current_cell = cell;
}

void AnimationInfo::blendOnSurface( SDL_Surface *dst_surface, int dst_x, int dst_y,
                                    SDL_Surface *src_surface, int src_x, int src_y,
                                    SDL_Rect *clip,
                                    int alpha, int scale_x, int scale_y, int rot,
                                    bool do_interpolation )
{
    if ( image_surface == NULL ) return;
    if ( scale_x == 0 || scale_y == 0 ) return;
    
    int i, x, y;
    SDL_Rect dst_rect;
    dst_rect.x = dst_x;
    dst_rect.y = dst_y;
    dst_rect.w = pos.w;
    dst_rect.h = pos.h;

    // for integer arithmetic operation
    int cos_i = 256, sin_i = 0;
    if (rot != 0){
        cos_i = (int)(256.0 * cos(-M_PI*rot/180));
        sin_i = (int)(256.0 * sin(-M_PI*rot/180));
    }

    // project corner point and calculate bounding box
    int dst_corner_xy[4][2];
    int dst_center_xy[2] = {dst_rect.x + dst_rect.w/2, dst_rect.y + dst_rect.h/2};
    int min_xy[2]={dst_surface->w-1, dst_surface->h-1}, max_xy[2]={0,0};
    for (i=0 ; i<4 ; i++){
        int c_x = ((((i+1)%4)/2)*2-1)*dst_rect.w/2 - (((i+1)%4)/2) * (1-dst_rect.w%2);
        int c_y = ((i/2)*2-1)*dst_rect.h/2 - (i/2) * (1-dst_rect.h%2);
        dst_corner_xy[i][0] = (cos_i * scale_x * c_x - sin_i * scale_y * c_y) / (100*256) + dst_center_xy[0];
        dst_corner_xy[i][1] = (sin_i * scale_x * c_x + cos_i * scale_y * c_y) / (100*256) + dst_center_xy[1];

        if (min_xy[0] > dst_corner_xy[i][0]) min_xy[0] = dst_corner_xy[i][0];
        if (max_xy[0] < dst_corner_xy[i][0]) max_xy[0] = dst_corner_xy[i][0];
        if (min_xy[1] > dst_corner_xy[i][1]) min_xy[1] = dst_corner_xy[i][1];
        if (max_xy[1] < dst_corner_xy[i][1]) max_xy[1] = dst_corner_xy[i][1];
    }

    // clip bounding box
    if (max_xy[0] < 0) return;
    if (max_xy[0] >= dst_surface->w) max_xy[0] = dst_surface->w - 1;
    if (min_xy[0] >= dst_surface->w) return;
    if (min_xy[0] < 0) min_xy[0] = 0;
    if (max_xy[1] < 0) return;
    if (max_xy[1] >= dst_surface->h) max_xy[1] = dst_surface->h - 1;
    if (min_xy[1] >= dst_surface->h) return;
    if (min_xy[1] < 0) min_xy[1] = 0;

    // extra clipping
    if ( clip ){
        if (max_xy[0] < clip->x) return;
        else if (max_xy[0] >= clip->x + clip->w) max_xy[0] = clip->x + clip->w - 1;
        if (min_xy[0] >= clip->x + clip->w) return;
        else if (min_xy[0] < clip->x) min_xy[0] = clip->x;
        if (max_xy[1] < clip->y) return;
        else if (max_xy[1] >= clip->y + clip->h) max_xy[1] = clip->y + clip->h - 1;
        if (min_xy[1] >= clip->y + clip->h) return;
        else if (min_xy[1] < clip->y) min_xy[1] = clip->y;
    }

    // lock surface
    SDL_LockSurface( dst_surface );
    if ( src_surface != dst_surface ) SDL_LockSurface( src_surface );
    SDL_LockSurface( image_surface );

    SDL_Surface *tmp_mask_surface;
    int mask_offset;
    if ( mask_surface ){
        SDL_LockSurface( mask_surface );
        tmp_mask_surface = mask_surface;
        mask_offset = 0;
    }
    else{
        tmp_mask_surface = image_surface;
        mask_offset = pos.w;
    }

    // check constant alpha key
    Uint32 ref_color=0;
    if ( trans_mode == TRANS_TOPLEFT ){
        ref_color = *((Uint32*)image_surface->pixels);
    }
    else if ( trans_mode == TRANS_TOPRIGHT ){
        ref_color = *((Uint32*)image_surface->pixels + image_surface->w - 1);
    }
    else if ( trans_mode == TRANS_DIRECT ) {
        ref_color = direct_color[0] << image_surface->format->Rshift |
            direct_color[1] << image_surface->format->Gshift |
            direct_color[2] << image_surface->format->Bshift;
    }
    ref_color &= 0xffffff;

    // set pixel by inverse-projection with raster scanning
    for (y=min_xy[1] ; y<= max_xy[1] ; y++){
        // calculate start and end point for each raster scanning
        int raster_min = min_xy[0], raster_max = max_xy[0];
        if (rot != 0){
            for (i=0 ; i<4 ; i++){
                if (dst_corner_xy[i][1] == dst_corner_xy[(i+1)%4][1]) continue;
                x = (dst_corner_xy[(i+1)%4][0] - dst_corner_xy[i][0])*(y-dst_corner_xy[i][1])/(dst_corner_xy[(i+1)%4][1] - dst_corner_xy[i][1]) + dst_corner_xy[i][0];
                if (dst_corner_xy[(i+1)%4][1] - dst_corner_xy[i][1] > 0){
                    if (raster_max > x) raster_max = x;
                }
                else{
                    if (raster_min < x) raster_min = x;
                }
            }
        }

        Uint32 *dst_buffer = (Uint32 *)dst_surface->pixels + dst_surface->w * y + raster_min;
        Uint32 *src_buffer = (Uint32 *)src_surface->pixels + src_surface->w * (y-dst_rect.y+src_y) + raster_min - dst_rect.x + src_x;

        // inverse-projection
        for (x=raster_min ; x<=raster_max ; x++, dst_buffer++, src_buffer++){
#ifdef ENABLE_BILINEAR            
            int x2 = ( cos_i * (x-dst_center_xy[0]) + sin_i * (y-dst_center_xy[1])) * 100 / scale_x + (dst_rect.w/2)*256;
            int y2 = (-sin_i * (x-dst_center_xy[0]) + cos_i * (y-dst_center_xy[1])) * 100 / scale_y + (dst_rect.h/2)*256;
            int dx = x2 % 256;
            int dy = y2 % 256;
            
            x2 = x2 / 256;
            y2 = y2 / 256;
            
#else
            int x2 = ( cos_i * (x-dst_center_xy[0]) + sin_i * (y-dst_center_xy[1])) * 100 / (scale_x*256) + dst_rect.w/2;
            int y2 = (-sin_i * (x-dst_center_xy[0]) + cos_i * (y-dst_center_xy[1])) * 100 / (scale_y*256) + dst_rect.h/2;
#endif            
            if (x2 < 0) x2 = 0;
            else if (x2 >= image_surface->w) x2 = image_surface->w-1;
            x2 += image_surface->w*current_cell/num_of_cells;
            
            // one last line is omitted to accelerate the interpolation code
            if (y2 < 0) y2 = 0;
            else if (y2 >= image_surface->h-1) y2 = image_surface->h-2;
            
            Uint32 *sp_buffer  = (Uint32 *)image_surface->pixels + image_surface->w * y2 + x2;
            Uint32 pixel, mask_rb, mask_g;
#ifdef ENABLE_BILINEAR            
            Uint32 mask_rb2, mask_g2;
            if (do_interpolation){
                // bi-linear interpolation
                mask_rb = (((*sp_buffer & 0xff00ff) * (256-dx) + (*(sp_buffer+1) & 0xff00ff) * dx) >> 8) & 0xff00ff;
                mask_g  = (((*sp_buffer & 0x00ff00) * (256-dx) + (*(sp_buffer+1) & 0x00ff00) * dx) >> 8) & 0x00ff00;

                sp_buffer += image_surface->w;
                mask_rb2 = (((*sp_buffer & 0xff00ff) * (256-dx) + (*(sp_buffer+1) & 0xff00ff) * dx) >> 8) & 0xff00ff;
                mask_g2  = (((*sp_buffer & 0x00ff00) * (256-dx) + (*(sp_buffer+1) & 0x00ff00) * dx) >> 8) & 0x00ff00;

                pixel = (((mask_rb * (256-dy) + mask_rb2 * dy) >> 8) & 0xff00ff) |
                    (((mask_g  * (256-dy) + mask_g2  * dy) >> 8) & 0x00ff00);
            }
            else
#endif                
            {
                pixel = *sp_buffer;
            }

            Uint32 mask, mask1, mask2;
            if ( trans_mode == TRANS_ALPHA || trans_mode == TRANS_MASK ){
                mask = ~*((Uint32 *)tmp_mask_surface->pixels + tmp_mask_surface->w * (y2%tmp_mask_surface->h) + (x2+mask_offset)%tmp_mask_surface->w) & 0xff;
                mask2 = (mask * alpha) >> 8;
            }
            else if ( trans_mode == TRANS_TOPLEFT ||
                      trans_mode == TRANS_TOPRIGHT ||
                      trans_mode == TRANS_DIRECT ){
                if ( (pixel & 0xffffff) == ref_color )
                    mask2 = 0;
                else
                    mask2 = alpha;
            }
            else if ( trans_mode == TRANS_STRING ){
                mask = *((Uint32 *)tmp_mask_surface->pixels + tmp_mask_surface->w * (y2%tmp_mask_surface->h) + (x2+mask_offset)%tmp_mask_surface->w) & 0xff000000;
                mask >>= 24;
                mask2 = (mask * alpha) >> 8;
            }
            else{ // TRANS_COPY
                mask2 = alpha;
            }
            mask1 = 256 - mask2;
            
            mask_rb = (((*src_buffer & 0xff00ff) * mask1 +
                        (pixel & 0xff00ff) * mask2) >> 8) & 0xff00ff; // red and blue pixel
            mask_g = (((*src_buffer & 0x00ff00) * mask1 +
                       (pixel & 0x00ff00) * mask2) >> 8) & 0x00ff00; // green pixel

            *dst_buffer = mask_rb | mask_g;
        }
    }
    
    // unlock surface
    if ( mask_surface ) SDL_UnlockSurface( mask_surface );
    SDL_UnlockSurface( image_surface );
    if ( src_surface != dst_surface ) SDL_UnlockSurface( src_surface );
    SDL_UnlockSurface( dst_surface );
}
