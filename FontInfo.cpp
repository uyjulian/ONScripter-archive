/* -*- C++ -*-
 * 
 *  FontInfo.cpp - Font information storage class of ONScripter
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

#include "FontInfo.h"
#include <stdio.h>
#include <SDL_ttf.h>

static struct FontContainer{
    FontContainer *next;
    int size;
    TTF_Font *font;

    FontContainer(){
        size = 0;
        next = NULL;
        font = NULL;
    };
} root_font_container;

FontInfo::FontInfo()
{
    ttf_font = NULL;

    reset();
    resetSelectColor();
}

void FontInfo::reset()
{
    clear();

    color[0] = color[1] = color[2] = 0xff;
    is_bold = true;
    is_shadow = true;
    is_transparent = true;
}

void FontInfo::resetSelectColor()
{
    on_color[0]     = on_color[1]     = on_color[2]     = 0xff;
    off_color[0]    = off_color[1]    = off_color[2]    = 0x80;
    nofile_color[0] = nofile_color[1] = nofile_color[2] = 0x80;
}

void *FontInfo::openFont( char *font_file, int ratio1, int ratio2 )
{
    int font_size;
    if ( font_size_xy[0] < font_size_xy[1] )
        font_size = font_size_xy[0];
    else
        font_size = font_size_xy[1];

    FontContainer *fc = &root_font_container;
    while( fc->next ){
        if ( fc->next->size == font_size ) break;
        fc = fc->next;
    }
    if ( !fc->next ){
        fc->next = new FontContainer();
        fc->next->size = font_size;
        FILE *fp = fopen( font_file, "r" );
        if ( fp == NULL ) return NULL;
        fclose( fp );
        fc->next->font = TTF_OpenFont( font_file, font_size * ratio1 / ratio2 );
    }

    ttf_font = (void*)fc->next->font;
    
    return fc->next->font;
}

void FontInfo::setTateyokoMode( int tateyoko_mode )
{
    this->tateyoko_mode = tateyoko_mode;
    clear();
}

int FontInfo::getTateyokoMode()
{
    return tateyoko_mode;
}

int FontInfo::x(bool do_hankaku_shift)
{
    int ret = xy[0] * pitch_xy[0] + top_xy[0] + offset_xy[0];
    if (do_hankaku_shift && tateyoko_mode==YOKO_MODE) ret += pitch_xy[0] / 2;
        
    return ret;
}

int FontInfo::y(bool do_hankaku_shift)
{
    int ret = xy[1] * pitch_xy[1] + top_xy[1] + offset_xy[1];
    if (do_hankaku_shift && tateyoko_mode==TATE_MODE) ret += pitch_xy[1] / 2;
    
    return ret;
}

void FontInfo::setXY( int x, int y )
{
    if ( x != -1 ) xy[0] = x;
    if ( y != -1 ) xy[1] = y;
}

void FontInfo::clear()
{
    if (tateyoko_mode == YOKO_MODE)
        setXY(0, 0);
    else
        setXY(num_xy[0]-1, 0);
    offset_xy[0] = offset_xy[1] = 0;
}

void FontInfo::newLine()
{
    if (tateyoko_mode == YOKO_MODE){
        xy[0] = 0;
        xy[1]++;
    }
    else{
        xy[0]--;
        xy[1] = 0;
    }
    offset_xy[0] = offset_xy[1] = 0;
}

void FontInfo::setLineArea(int num)
{
    num_xy[tateyoko_mode] = num;
    num_xy[1-tateyoko_mode] = 1;
}

bool FontInfo::isEndOfLine(int margin)
{
    if (xy[tateyoko_mode] + margin >= num_xy[tateyoko_mode]) return true;

    return false;
}

bool FontInfo::isLineEmpty()
{
    if (xy[tateyoko_mode] == 0) return true;

    return false;
}

void FontInfo::advanceChar(int offset)
{
    xy[tateyoko_mode] += offset;
}

void FontInfo::addMargin(int offset)
{
    offset_xy[tateyoko_mode] += offset;
}

SDL_Rect FontInfo::calcUpdatedArea(int start_xy[2], int ratio1, int ratio2)
{
    SDL_Rect rect;
    
    if (tateyoko_mode == YOKO_MODE){
        if (start_xy[1] == xy[1]){
            rect.x = top_xy[0] + pitch_xy[0] * start_xy[0];
            rect.w = pitch_xy[0] * (xy[0] - start_xy[0] + 1);
        }
        else{
            rect.x = top_xy[0];
            rect.w = pitch_xy[0] * num_xy[0];
        }
        rect.y = top_xy[1] + start_xy[1] * pitch_xy[1];
        rect.h = pitch_xy[1] * (xy[1] - start_xy[1] + 1);
    }
    else{
        rect.x = top_xy[0] + pitch_xy[0] * xy[0];
        rect.w = pitch_xy[0] * (start_xy[0] - xy[0] + 1);
        if (start_xy[1] == xy[1]){
            rect.y = top_xy[1] + pitch_xy[1] * start_xy[1];
            rect.h = pitch_xy[1] * (xy[1] - start_xy[1] + 1);
        }
        else{
            rect.y = top_xy[1];
            rect.h = pitch_xy[1] * num_xy[1];
        }
    }

    rect.x = rect.x * ratio1 / ratio2;
    rect.y = rect.y * ratio1 / ratio2;
    rect.w = rect.w * ratio1 / ratio2;
    rect.h = rect.h * ratio1 / ratio2;
    
    return rect;
}

void FontInfo::addShadeArea(SDL_Rect &rect, int shade_distance[2])
{
    if (is_shadow){
        if (shade_distance[0]>0)
            rect.w += shade_distance[0];
        else{
            rect.x += shade_distance[0];
            rect.w -= shade_distance[0];
        }
        if (shade_distance[1]>0)
            rect.h += shade_distance[1];
        else{
            rect.y += shade_distance[1];
            rect.h -= shade_distance[1];
        }
    }
}

int FontInfo::initRuby(FontInfo &body_info, int body_count, int ruby_count)
{
    top_xy[0] = body_info.x();
    top_xy[1] = body_info.y();
    pitch_xy[0] = font_size_xy[0];
    pitch_xy[1] = font_size_xy[1];

    int margin=0;
    
    if (tateyoko_mode == YOKO_MODE){
        top_xy[1] -= font_size_xy[1];
        num_xy[0] = ruby_count;
        num_xy[1] = 1;
    }
    else{
        top_xy[0] += body_info.font_size_xy[0];
        num_xy[0] = 1;
        num_xy[1] = ruby_count;
    }
    
    if (ruby_count*font_size_xy[tateyoko_mode] >= body_count*body_info.pitch_xy[tateyoko_mode]){
        margin = (ruby_count*font_size_xy[tateyoko_mode] - body_count*body_info.pitch_xy[tateyoko_mode] + 1)/2;
    }
    else{
        int offset = (body_count*body_info.pitch_xy[tateyoko_mode] - ruby_count*font_size_xy[tateyoko_mode] + ruby_count) / (ruby_count*2);
        top_xy[tateyoko_mode] += offset;
        pitch_xy[tateyoko_mode] += offset*2;
    }
    body_info.offset_xy[tateyoko_mode] += margin;
    
    clear();

    return margin;
}
