/* -*- C++ -*-
 * 
 *  AnimationInfo.cpp - General image storage class of ONScripter
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

#include "AnimationInfo.h"

AnimationInfo::AnimationInfo(){
    /* Variables from TaggedInfo */
    current_cell = 0;
    num_of_cells = 0;
    direction = 1;
    duration_list = NULL;
    color_list = NULL;
    file_name = NULL;
    mask_file_name = NULL;

    /* Variables from AnimationInfo */
    valid = false;
    image_name = NULL;
    image_surface = NULL;
    preserve_surface = NULL;
    mask_surface = NULL;
    trans = 255;
}

AnimationInfo::~AnimationInfo(){
    removeTag();
    remove();
}

void AnimationInfo::deleteImageName(){
    if ( image_name ) delete[] image_name;
    image_name = NULL;
}

void AnimationInfo::setImageName( char *name ){
    deleteImageName();
    image_name = new char[ strlen(name) + 1 ];
    memcpy( image_name, name, strlen(name) + 1 );
}

void AnimationInfo::deleteSurface(){
    if ( image_surface ) SDL_FreeSurface( image_surface );
    image_surface = NULL;
    if ( preserve_surface ) SDL_FreeSurface( preserve_surface );
    preserve_surface = NULL;
    if ( mask_surface ) SDL_FreeSurface( mask_surface );
    mask_surface = NULL;
}

void AnimationInfo::remove(){
    valid = false;
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
    num_of_cells = 0;
}

// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
void AnimationInfo::proceedAnimation()
{
    if ( loop_mode != 3 && num_of_cells > 1 )
        current_cell += direction;

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
        }
        else{
            current_cell = num_of_cells - 2;
            direction = -1;
        }
    }
}

