/* -*- C++ -*-
 * 
 *  ONScripterLabel_effect.cpp - Effect executer of ONScripter
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

#define EFFECT_STRIPE_WIDTH 16
#define EFFECT_STRIPE_CURTAIN_WIDTH 24

int ONScripterLabel::setEffect( int immediate_flag, char *buf )
{
    //printf("setEffect %d\n",immediate_flag);
    DelayedInfo *p_effect = new DelayedInfo();
    p_effect->next = NULL;
    p_effect->command = buf;

    if ( !start_delayed_effect_info ) start_delayed_effect_info = p_effect;
    else{
        DelayedInfo *p_delayed_effect_info = start_delayed_effect_info;
        while( p_delayed_effect_info->next ) p_delayed_effect_info = p_delayed_effect_info->next;
        p_delayed_effect_info->next = p_effect;
    }

    if ( immediate_flag == RET_WAIT || immediate_flag == RET_WAIT_NEXT ){
        effect_counter = 0;
        event_mode = EFFECT_EVENT_MODE;
        startTimer( MINIMUM_TIMER_RESOLUTION );
    }

    return immediate_flag;
}

int ONScripterLabel::doEffect( int effect_no, AnimationInfo *anim, int effect_image )
{
    effect_start_time = SDL_GetTicks();
    if ( effect_counter == 0 ) effect_start_time_old = effect_start_time - 1;
    //printf("effect_counter %d timer between %d %d\n",effect_counter,effect_start_time,effect_start_time_old);
    effect_timer_resolution = effect_start_time - effect_start_time_old;
    effect_start_time_old = effect_start_time;
    
    //printf("doEffect %d %d\n", effect_no, effect_counter );
    int i;
    int width, width2;
    int height, height2;
    SDL_Rect src_rect, dst_rect;

    if ( effect_counter == 0 ){
        if ( effect_image != DIRECT_EFFECT_IMAGE )
            SDL_BlitSurface( accumulation_surface, NULL, effect_src_surface, NULL );

        switch( effect_image ){
          case DIRECT_EFFECT_IMAGE:
            break;
            
          case COLOR_EFFECT_IMAGE:
            SDL_FillRect( background_surface, NULL, SDL_MapRGB( effect_dst_surface->format, anim->tag.color[0], anim->tag.color[1], anim->tag.color[2]) );
            refreshAccumulationSurface( effect_dst_surface );
            break;

          case BG_EFFECT_IMAGE:
            if ( anim->image_surface ){
                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.w = anim->image_surface->w;
                src_rect.h = anim->image_surface->h;
                dst_rect.x = (screen_width - anim->image_surface->w) / 2;
                dst_rect.y = (screen_height - anim->image_surface->h) / 2;

                SDL_BlitSurface( anim->image_surface, &src_rect, background_surface, &dst_rect );
            }
            refreshAccumulationSurface( effect_dst_surface );
            break;
            
          case TACHI_EFFECT_IMAGE:
            refreshAccumulationSurface( effect_dst_surface );
            break;
        }
    }
    
    /* ---------------------------------------- */
    /* Execute effect */
    struct EffectLink effect = getEffect( effect_no );
    //printf("Effect number %d\n", effect.effect );
    switch ( effect.effect ){
      case 0: // Instant display
      case 1: // Instant display
        SDL_BlitSurface( effect_dst_surface, NULL, text_surface, NULL );
        break;

      case 2: // Left shutter
        width = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=0 ; i<screen_width/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = screen_height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 3: // Right shutter
        width = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=screen_width/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = i * EFFECT_STRIPE_WIDTH - width - 1;
            src_rect.y = dst_rect.y = 0;
            src_rect.w = dst_rect.w = width;
            src_rect.h = dst_rect.h = screen_height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 4: // Top shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=0 ; i<screen_height/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH;
            src_rect.w = dst_rect.w = screen_width;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 5: // Bottom shutter
        height = EFFECT_STRIPE_WIDTH * effect_counter / effect.duration;
        for ( i=1 ; i<=screen_height/EFFECT_STRIPE_WIDTH ; i++ ){
            src_rect.x = dst_rect.x = 0;
            src_rect.y = dst_rect.y = i * EFFECT_STRIPE_WIDTH - height - 1;
            src_rect.w = dst_rect.w = screen_width;
            src_rect.h = dst_rect.h = height;
            SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        }
        break;

      case 6: // Left curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_width/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_width;
            if ( width2 >= 0 ){
                src_rect.x = dst_rect.x = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = screen_height;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 7: // Right curtain
        width = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_width/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            width2 = width - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_width;
            if ( width2 >= 0 ){
                if ( width2 > EFFECT_STRIPE_CURTAIN_WIDTH ) width2 = EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.x = dst_rect.x = screen_width - i * EFFECT_STRIPE_CURTAIN_WIDTH - width2;
                src_rect.y = dst_rect.y = 0;
                src_rect.w = dst_rect.w = width2;
                src_rect.h = dst_rect.h = screen_height;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 8: // Top curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_height/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_height;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = i * EFFECT_STRIPE_CURTAIN_WIDTH;
                src_rect.w = dst_rect.w = screen_width;
                src_rect.h = dst_rect.h = height2;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      case 9: // Bottom curtain
        height = EFFECT_STRIPE_CURTAIN_WIDTH * effect_counter * 2 / effect.duration;
        for ( i=0 ; i<=screen_height/EFFECT_STRIPE_CURTAIN_WIDTH ; i++ ){
            height2 = height - EFFECT_STRIPE_CURTAIN_WIDTH * EFFECT_STRIPE_CURTAIN_WIDTH * i / screen_height;
            if ( height2 >= 0 ){
                src_rect.x = dst_rect.x = 0;
                src_rect.y = dst_rect.y = screen_height - i * EFFECT_STRIPE_CURTAIN_WIDTH - height2;
                src_rect.w = dst_rect.w = screen_width;
                src_rect.h = dst_rect.h = height2;
                SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
            }
        }
        break;

      default:
        printf("effect.effect %d is not implemented. Crossfade is substituted for that.\n",effect.effect);
        
      case 10: // Cross fade
        height = 255 - 256 * effect_counter / effect.duration;
        alphaBlend( text_surface, 0, 0,
                    effect_src_surface, 0, 0, screen_width, screen_height,
                    effect_dst_surface, 0, 0,
                    0, 0, height );
        break;
        
      case 11: // Left scroll
        width = screen_width * effect_counter / effect.duration;
        src_rect.x = 0;
        dst_rect.x = width;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width - width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = screen_width - width - 1;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 12: // Right scroll
        width = screen_width * effect_counter / effect.duration;
        src_rect.x = width;
        dst_rect.x = 0;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width - width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = 0;
        dst_rect.x = screen_width - width - 1;
        src_rect.y = dst_rect.y = 0;
        src_rect.w = dst_rect.w = width;
        src_rect.h = dst_rect.h = screen_height;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 13: // Top scroll
        width = screen_height * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = width;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = screen_height - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = screen_height - width - 1;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = width;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;

      case 14: // Bottom scroll
        width = screen_height * effect_counter / effect.duration;
        src_rect.x = dst_rect.x = 0;
        src_rect.y = width;
        dst_rect.y = 0;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = screen_height - width;
        SDL_BlitSurface( effect_src_surface, &src_rect, text_surface, &dst_rect );
        src_rect.x = dst_rect.x = 0;
        src_rect.y = 0;
        dst_rect.y = screen_height - width - 1;
        src_rect.w = dst_rect.w = screen_width;
        src_rect.h = dst_rect.h = width;
        SDL_BlitSurface( effect_dst_surface, &src_rect, text_surface, &dst_rect );
        break;
    }

    //printf("%d / %d\n", effect_counter, effect.duration);
        
    effect_counter += effect_timer_resolution;
    if ( effect_counter < effect.duration ){
        if ( effect.effect != 0 ){
            if ( !erase_text_window_flag ){
                shadowTextDisplay( NULL, text_surface );
                restoreTextBuffer();
            }
            flush();
        }
        return RET_WAIT;
    }
    else{
        //monocro_flag = false;
        if ( effect_image == DIRECT_EFFECT_IMAGE ){
            SDL_BlitSurface( effect_dst_surface, NULL, text_surface, NULL );
        }
        else{
            SDL_BlitSurface( effect_dst_surface, NULL, accumulation_surface, NULL );
            SDL_BlitSurface( accumulation_surface, NULL, text_surface, NULL );
        }
        if ( !erase_text_window_flag ){
            shadowTextDisplay( NULL, text_surface );
            restoreTextBuffer();
        }
        if ( effect.effect != 0 ) flush();
        return RET_CONTINUE;
    }
}

void ONScripterLabel::makeEffectStr( char **buf, char *dst_buf )
{
    int num = readEffect( buf, &tmp_effect );

    sprintf( dst_buf + strlen(dst_buf), "%d", tmp_effect.effect );
    if ( num >= 2 ){
        sprintf( dst_buf + strlen(dst_buf), ",%d", tmp_effect.duration );
        if ( num >= 3 ){
            sprintf( dst_buf + strlen(dst_buf), ",%s", tmp_effect.image );
        }
    }
}

int ONScripterLabel::readEffect( char **buf, struct EffectLink *effect )
{
    int num = 1;
    
    effect->effect = readInt( buf );
    if ( end_with_comma_flag ){
        num++;
        effect->duration = readInt( buf );
        if ( end_with_comma_flag ){
            num++;
            readStr( buf, tmp_string_buffer );
            if ( effect->image ) delete[] effect->image;
            effect->image = new char[ strlen(tmp_string_buffer ) + 1 ];
            memcpy( effect->image, tmp_string_buffer, strlen( tmp_string_buffer ) + 1 );
        }
    }

    //printf("readEffect %d: %d %d %s\n", num, effect->effect, effect->duration, effect->image );
    return num;
}

