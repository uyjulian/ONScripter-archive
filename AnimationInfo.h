/* -*- C++ -*-
 * 
 *  AnimationInfo.h - General image storage class of ONScripter
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

#ifndef __ANIMATION_INFO_H__
#define __ANIMATION_INFO_H__

#include <SDL.h>
#include <string.h>

typedef unsigned char uchar3[3];

class AnimationInfo{
public:
    typedef enum{
            TRANS_ALPHA = 1,
            TRANS_TOPLEFT = 2,
            TRANS_COPY = 3,
            TRANS_STRING = 4,
            TRANS_DIRECT = 5,
            TRANS_PALLET = 6,
            TRANS_TOPRIGHT = 7,
            TRANS_MASK = 8,
            TRANS_FADE_MASK = 9,
            TRANS_CROSSFADE_MASK = 10
            } TRANS_MODE;

    /* Variables from TaggedInfo */
    int trans_mode;
    char direct_color[8];
    int pallet_number;
    uchar3 color;
    SDL_Rect pos; // pose and size of the current cell

    int num_of_cells;
    int current_cell;
    int direction;
    int *duration_list;
    uchar3 *color_list;
    int loop_mode;
        
    char *file_name;
    char *mask_file_name;

    /* Variables from AnimationInfo */
    bool valid;
    bool abs_flag;
    //SDL_Rect pos;
    int alpha_offset;
    int trans;
    //struct TaggedInfo tag;
    char *image_name;
    SDL_Surface *image_surface;
    SDL_Surface *preserve_surface;
    SDL_Surface *mask_surface;

    AnimationInfo();
    ~AnimationInfo();
    void deleteImageName();
    void setImageName( char *name );
    void deleteSurface();
    void remove();
    void removeTag();

    void proceedAnimation();
};

#endif // __ANIMATION_INFO_H__
