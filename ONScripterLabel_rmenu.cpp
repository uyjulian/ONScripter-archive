/* -*- C++ -*-
 *
 *  ONScripterLabel_rmenu.cpp - Right click menu handler of ONScripter
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

void ONScripterLabel::enterSystemCall()
{
    shelter_button_link = root_button_link.next;
    root_button_link.next = NULL;
    shelter_select_link = root_select_link.next;
    root_select_link.next = NULL;
    SDL_BlitSurface( accumulation_surface, NULL, shelter_accumulation_surface, NULL );
    shelter_event_mode = event_mode;
    shelter_mouse_state.x = last_mouse_state.x;
    shelter_mouse_state.y = last_mouse_state.y;
    event_mode = IDLE_EVENT_MODE;
    system_menu_enter_flag = true;
    yesno_caller = SYSTEM_NULL;
}

void ONScripterLabel::leaveSystemCall( bool restore_flag )
{
    if ( restore_flag ){
        SDL_BlitSurface( shelter_accumulation_surface, NULL, accumulation_surface, NULL );

        dirty_rect.fill( screen_width, screen_height );
        flush();
        root_button_link.next = shelter_button_link;
        root_select_link.next = shelter_select_link;
        event_mode = shelter_event_mode;
        if ( event_mode & WAIT_BUTTON_MODE ){
            if ( mouse_rotation_mode == MOUSE_ROTATION_NONE ||
                 mouse_rotation_mode == MOUSE_ROTATION_PDA_VGA )
                SDL_WarpMouse( shelter_mouse_state.x, shelter_mouse_state.y );
            else if ( mouse_rotation_mode == MOUSE_ROTATION_PDA )
                SDL_WarpMouse( screen_height - shelter_mouse_state.y - 1, shelter_mouse_state.x );
        }
        current_text_buffer = cached_text_buffer;
    }

    system_menu_mode = SYSTEM_NULL;
    system_menu_enter_flag = false;
    yesno_caller = SYSTEM_NULL;
    key_pressed_flag = false;

    //printf("leaveSystemCall %d %d\n",event_mode, clickstr_state);

    refreshMouseOverButton();
    advancePhase();
}

void ONScripterLabel::executeSystemCall()
{
    //printf("*****  executeSystemCall %d %d %d*****\n", system_menu_enter_flag, volatile_button_state.button, system_menu_mode );
    dirty_rect.fill( screen_width, screen_height );
    
    if ( !system_menu_enter_flag ){
        enterSystemCall();
    }
    
    switch( system_menu_mode ){
      case SYSTEM_SKIP:
        executeSystemSkip();
        break;
      case SYSTEM_RESET:
        executeSystemReset();
        break;
      case SYSTEM_SAVE:
        executeSystemSave();
        break;
      case SYSTEM_YESNO:
        executeSystemYesNo();
        break;
      case SYSTEM_LOAD:
        executeSystemLoad();
        break;
      case SYSTEM_LOOKBACK:
        executeSystemLookback();
        break;
      case SYSTEM_WINDOWERASE:
        executeWindowErase();
        break;
      case SYSTEM_MENU:
        executeSystemMenu();
        break;
      default:
        leaveSystemCall();
    }
}

void ONScripterLabel::executeSystemMenu()
{
    MenuLink *link;
    int counter = 1;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button == -1 ){
            if ( menuselectvoice_file_name[MENUSELECTVOICE_CANCEL] )
                playWave( menuselectvoice_file_name[MENUSELECTVOICE_CANCEL], false, DEFAULT_WAVE_CHANNEL );
            leaveSystemCall();
            return;
        }
    
        if ( menuselectvoice_file_name[MENUSELECTVOICE_CLICK] )
            playWave( menuselectvoice_file_name[MENUSELECTVOICE_CLICK], false, DEFAULT_WAVE_CHANNEL );
        
        link = root_menu_link.next;
        while ( link ){
            if ( current_button_state.button == counter++ ){
                system_menu_mode = link->system_call_no;
                break;
            }
            link = link->next;
        }

        advancePhase();
    }
    else{
        if ( menuselectvoice_file_name[MENUSELECTVOICE_OPEN] )
            playWave( menuselectvoice_file_name[MENUSELECTVOICE_OPEN], false, DEFAULT_WAVE_CHANNEL );
        refreshSurface( accumulation_surface, NULL );
        shadowTextDisplay( accumulation_surface, accumulation_surface, NULL, &menu_font );
        flush();

        menu_font.num_xy[0] = menu_link_width;
        menu_font.num_xy[1] = menu_link_num;
        menu_font.top_xy[0] = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height * screen_ratio2 / screen_ratio1  - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;

        menu_font.setXY( (menu_font.num_xy[0] - menu_link_width) / 2,
                         (menu_font.num_xy[1] - menu_link_num) / 2 );

        link = root_menu_link.next;
        while( link ){
            ButtonLink *button = getSelectableSentence( link->label, &menu_font, false );
            root_button_link.insert( button );
            button->no = counter++;

            link = link->next;
            flush( &button->image_rect );
        }

        flushEvent();
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_MENU;
        yesno_caller = SYSTEM_MENU;
    }
}

void ONScripterLabel::executeSystemSkip()
{
    skip_flag = true;
    if ( !(shelter_event_mode & WAIT_BUTTON_MODE) )
        shelter_event_mode &= ~WAIT_TIMER_MODE;
    leaveSystemCall();
}

void ONScripterLabel::executeSystemReset()
{
    if ( yesno_caller == SYSTEM_RESET ){
        leaveSystemCall();
    }
    else{
        if ( yesno_caller == SYSTEM_NULL )
            yesno_caller = SYSTEM_RESET;
        system_menu_mode = SYSTEM_YESNO;
        advancePhase();
    }
}

void ONScripterLabel::executeWindowErase()
{
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){
        event_mode = IDLE_EVENT_MODE;

        leaveSystemCall();
    }
    else{
        refreshSurface( accumulation_surface, NULL, mode_saya_flag ? REFRESH_SAYA_MODE : REFRESH_NORMAL_MODE );
        flush();

        event_mode = WAIT_INPUT_MODE;
        system_menu_mode = SYSTEM_WINDOWERASE;
    }
}

void ONScripterLabel::executeSystemLoad()
{
    SaveFileInfo save_file_info;
    
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        if ( current_button_state.button > 0 ){
            searchSaveFile( save_file_info, current_button_state.button );
            if ( !save_file_info.valid ){
                event_mode  = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
                refreshMouseOverButton();
                return;
            }
            deleteButtonLink();
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_LOAD;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
        }
        else{
            deleteButtonLink();
            leaveSystemCall();
        }
    }
    else{
        refreshSurface( accumulation_surface, NULL );
        shadowTextDisplay( accumulation_surface, accumulation_surface, NULL, &menu_font );
        flush();
        
        system_font.setXY( (system_font.num_xy[0] - strlen( load_menu_name ) / 2) / 2, 0 );
        drawString( load_menu_name, system_font.color, &system_font, true, accumulation_surface );
        system_font.xy[1] += 2;
        
        bool nofile_flag;
        char *buffer = new char[ strlen( save_item_name ) + 30 + 1 ];

        for ( unsigned int i=1 ; i<=num_save_file ; i++ ){
            searchSaveFile( save_file_info, i );
            system_font.setXY( (system_font.num_xy[0] - (strlen( save_item_name ) / 2 + 15) ) / 2 );

            if ( save_file_info.valid ){
                sprintf( buffer, "%s%s�@%s��%s��%s��%s��",
                         save_item_name,
                         save_file_info.sjis_no,
                         save_file_info.sjis_month,
                         save_file_info.sjis_day,
                         save_file_info.sjis_hour,
                         save_file_info.sjis_minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, "%s%s�@�|�|�|�|�|�|�|�|�|�|�|�|",
                         save_item_name,
                         save_file_info.sjis_no );
                nofile_flag = true;
            }
            ButtonLink *button = getSelectableSentence( buffer, &system_font, false, nofile_flag );
            root_button_link.insert( button );
            button->no = i;
            flush( &button->image_rect );
        }
        delete[] buffer;

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_LOAD;
    }
}

void ONScripterLabel::executeSystemSave()
{
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button > 0 ){
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_SAVE;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
            return;
        }
        leaveSystemCall();
    }
    else{
        refreshSurface( accumulation_surface, NULL );
        shadowTextDisplay( accumulation_surface, accumulation_surface, NULL, &menu_font );
        flush();

        system_font.xy[0] = (system_font.num_xy[0] - strlen( save_menu_name ) / 2 ) / 2;
        system_font.xy[1] = 0;
        drawString( save_menu_name, system_font.color, &system_font, true, accumulation_surface );
        system_font.xy[1] += 2;
        
        bool nofile_flag;
        char *buffer = new char[ strlen( save_item_name ) + 30 + 1 ];
    
        for ( unsigned int i=1 ; i<=num_save_file ; i++ ){
            SaveFileInfo save_file_info;
            searchSaveFile( save_file_info, i );
            system_font.xy[0] = (system_font.num_xy[0] - (strlen( save_item_name ) / 2 + 15) ) / 2;

            if ( save_file_info.valid ){
                sprintf( buffer, "%s%s�@%s��%s��%s��%s��",
                         save_item_name,
                         save_file_info.sjis_no,
                         save_file_info.sjis_month,
                         save_file_info.sjis_day,
                         save_file_info.sjis_hour,
                         save_file_info.sjis_minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, "%s%s�@�|�|�|�|�|�|�|�|�|�|�|�|",
                         save_item_name,
                         save_file_info.sjis_no );
                nofile_flag = true;
            }
            ButtonLink *button = getSelectableSentence( buffer, &system_font, false, nofile_flag );
            root_button_link.insert( button );
            button->no = i;
            flush( &button->image_rect );
        }
        delete[] buffer;

        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
        system_menu_mode = SYSTEM_SAVE;
    }
}

void ONScripterLabel::executeSystemYesNo()
{
    char name[64] = {'\0'};
	
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button == 1 ){ // yes is selected
            if ( menuselectvoice_file_name[MENUSELECTVOICE_YES] )
                playWave( menuselectvoice_file_name[MENUSELECTVOICE_YES], false, DEFAULT_WAVE_CHANNEL );
            if ( yesno_caller == SYSTEM_SAVE ){
                saveSaveFile( yesno_selected_file_no );
                leaveSystemCall();
            }
            else if ( yesno_caller == SYSTEM_LOAD ){

                if ( loadSaveFile( yesno_selected_file_no ) ){
                    system_menu_mode = yesno_caller;
                    advancePhase();
                    return;
                }
                leaveSystemCall( false );
                saveon_flag = true;
                internal_saveon_flag = true;
            }
            else if ( yesno_caller == SYSTEM_RESET || 
                      yesno_caller == SYSTEM_MENU ){

                resetCommand();
                event_mode = WAIT_SLEEP_MODE;
                leaveSystemCall( false );
            }
        }
        else{
            if ( menuselectvoice_file_name[MENUSELECTVOICE_NO] )
                playWave( menuselectvoice_file_name[MENUSELECTVOICE_NO], false, DEFAULT_WAVE_CHANNEL );
            system_menu_mode = yesno_caller;
            advancePhase();
        }
    }
    else{
        refreshSurface( accumulation_surface, NULL );
        shadowTextDisplay( accumulation_surface, accumulation_surface, NULL, &menu_font );
        flush();

        if ( yesno_caller == SYSTEM_SAVE || yesno_caller == SYSTEM_LOAD ){
            SaveFileInfo save_file_info;
            searchSaveFile( save_file_info, yesno_selected_file_no );
            sprintf( name, "%s%s", 
                     save_item_name,
                     save_file_info.sjis_no );
        }
            
        if ( yesno_caller == SYSTEM_SAVE )
            strcat( name, "�ɃZ�[�u���܂��B��낵���ł����H" );
        else if ( yesno_caller == SYSTEM_LOAD )
            strcat( name, "�����[�h���܂��B��낵���ł����H" );
        else if ( yesno_caller == SYSTEM_RESET || 
                  yesno_caller == SYSTEM_MENU )
            strcpy( name, "�I�����܂��B��낵���ł����H" );
        system_font.xy[0] = (system_font.num_xy[0] - strlen( name ) / 2 ) / 2;
        system_font.xy[1] = 10;
        drawString( name, system_font.color, &system_font, true, accumulation_surface );

        strcpy( name, "�͂�" );
        system_font.xy[0] = 12;
        system_font.xy[1] = 12;
        ButtonLink *button = getSelectableSentence( name, &system_font, false );
        root_button_link.insert( button );
        button->no = 1;
        flush( &button->image_rect );

        strcpy( name, "������" );
        system_font.xy[0] = 18;
        system_font.xy[1] = 12;
        button = getSelectableSentence( name, &system_font, false );
        root_button_link.insert( button );
        button->no = 2;
        flush( &button->image_rect );
        
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}

void ONScripterLabel::setupLookbackButton()
{
    AnimationInfo *info[2];
    SDL_Rect rect;
    int offset;

    deleteButtonLink();
    
    /* ---------------------------------------- */
    /* Previous button check */
    if ( (current_text_buffer->previous->buffer2_count > 0 ) &&
         current_text_buffer != start_text_buffer ){
        ButtonLink *button = new ButtonLink();
        root_button_link.insert( button );
    
        button->no = 1;
        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_sp[0] >= 0 ){
            info[0] = &sprite_info[ lookback_sp[0] ];
            info[0]->current_cell = 0;
            info[1] = &sprite_info[ lookback_sp[0] ];
            button->image_rect.x = sprite_info[ lookback_sp[0] ].pos.x;
            button->image_rect.y = sprite_info[ lookback_sp[0] ].pos.y;
        }
        else{
            info[0] = &lookback_info[0];
            info[1] = &lookback_info[1];
            button->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - info[0]->pos.w;
            button->image_rect.y = sentence_font_info.pos.y;
        }
            
        if ( info[0]->image_surface ){
            button->image_rect.w = info[0]->pos.w;
            button->image_rect.h = info[0]->pos.h;
            
            button->selected_surface =
                SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG,
                                      info[0]->pos.w, info[0]->pos.h,
                                      32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( button->selected_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );

            offset = (info[0]->pos.w + info[0]->alpha_offset) * info[0]->current_cell;
            rect.x = rect.y = 0;
            rect.w = info[0]->pos.w;
            rect.h = info[0]->pos.h;
            alphaBlend( button->selected_surface, rect,
                        accumulation_surface, button->image_rect.x, button->image_rect.y,
                        info[0]->image_surface, offset, 0,
                        info[0]->mask_surface, info[0]->alpha_offset,
                        info[0]->trans_mode );
        }

        if ( lookback_sp[0] >= 0 ) info[1]->current_cell = 1;

        if ( info[1]->image_surface ){
            button->no_selected_surface =
                SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG,
                                      info[1]->pos.w, info[1]->pos.h,
                                      32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( button->no_selected_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
        
            offset = (info[1]->pos.w + info[1]->alpha_offset) * info[1]->current_cell;
            rect.x = rect.y = 0;
            rect.w = info[1]->pos.w;
            rect.h = info[1]->pos.h;
            alphaBlend( button->no_selected_surface, rect,
                        accumulation_surface, button->image_rect.x, button->image_rect.y,
                        info[1]->image_surface, offset, 0,
                        info[1]->mask_surface, info[1]->alpha_offset,
                        info[1]->trans_mode );
            SDL_BlitSurface( button->no_selected_surface, NULL, accumulation_surface, &button->image_rect );
        }
    }

    /* ---------------------------------------- */
    /* Next button check */
    if ( current_text_buffer->next != cached_text_buffer ){
        ButtonLink *button = new ButtonLink();
        root_button_link.insert( button );
    
        button->no = 2;
        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h*2/3;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_sp[1] >= 0 ){
            info[0] = &sprite_info[ lookback_sp[1] ];
            info[0]->current_cell = 0;
            info[1] = &sprite_info[ lookback_sp[1] ];
            button->image_rect.x = sprite_info[ lookback_sp[1] ].pos.x;
            button->image_rect.y = sprite_info[ lookback_sp[1] ].pos.y;
        }
        else{
            info[0] = &lookback_info[2];
            info[1] = &lookback_info[3];
            button->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - info[0]->pos.w;
            button->image_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h - info[0]->pos.h;
        }
            
        if ( info[0]->image_surface ){
            button->image_rect.w = info[0]->pos.w;
            button->image_rect.h = info[0]->pos.h;
            
            button->selected_surface =
                SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG,
                                      info[0]->pos.w, info[0]->pos.h,
                                      32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( button->selected_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );

            offset = (info[0]->pos.w + info[0]->alpha_offset) * info[0]->current_cell;
            rect.x = rect.y = 0;
            rect.w = info[0]->pos.w;
            rect.h = info[0]->pos.h;
            alphaBlend( button->selected_surface, rect,
                        accumulation_surface, button->image_rect.x, button->image_rect.y,
                        info[0]->image_surface, offset, 0,
                        info[0]->mask_surface, info[0]->alpha_offset,
                        info[0]->trans_mode );
        }

        if ( lookback_sp[1] >= 0 ) info[1]->current_cell = 1;

        if ( info[1]->image_surface ){
            button->no_selected_surface =
                SDL_CreateRGBSurface( DEFAULT_SURFACE_FLAG,
                                      info[1]->pos.w, info[1]->pos.h,
                                      32, rmask, gmask, bmask, amask );
            SDL_SetAlpha( button->no_selected_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
        
            offset = (info[1]->pos.w + info[1]->alpha_offset) * info[1]->current_cell;
            rect.x = rect.y = 0;
            rect.w = info[1]->pos.w;
            rect.h = info[1]->pos.h;
            alphaBlend( button->no_selected_surface, rect,
                        accumulation_surface, button->image_rect.x, button->image_rect.y,
                        info[1]->image_surface, offset, 0,
                        info[1]->mask_surface, info[1]->alpha_offset,
                        info[1]->trans_mode );
            SDL_BlitSurface( button->no_selected_surface, NULL, accumulation_surface, &button->image_rect );
        }
    }
}

void ONScripterLabel::executeSystemLookback()
{
    int i;
    uchar3 color;
    
    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){
        if ( current_button_state.button == 0 ||
             ( current_text_buffer == start_text_buffer &&
               current_button_state.button == -2 ) )
            return;
        if ( current_button_state.button == -1 ||
             ( current_button_state.button == -3 &&
               current_text_buffer->next == cached_text_buffer ) ||
             current_button_state.button <= -4 )
        {
            event_mode = IDLE_EVENT_MODE;
            deleteButtonLink();
            leaveSystemCall();
            return;
        }
        
        if ( current_button_state.button == 1 ||
             current_button_state.button == -2 ){
            current_text_buffer = current_text_buffer->previous;
        }
        else
            current_text_buffer = current_text_buffer->next;
    }
    else{
        current_text_buffer = current_text_buffer->previous;
        if ( current_text_buffer->buffer2_count == 0 ){
            leaveSystemCall();
            return;
        }

        //SDL_BlitSurface( picture_surface, NULL, accumulation_surface, NULL );
        
        event_mode = WAIT_INPUT_MODE | WAIT_BUTTON_MODE;
        system_menu_mode = SYSTEM_LOOKBACK;
    }

    shadowTextDisplay( accumulation_surface, picture_surface, NULL, &sentence_font );
    for ( i=0 ; i<3 ; i++ ){
        color[i] = sentence_font.color[i];
        sentence_font.color[i] = lookback_color[i];
    }
    restoreTextBuffer( accumulation_surface, NULL, false );
    for ( i=0 ; i<3 ; i++ ) sentence_font.color[i] = color[i];
    setupLookbackButton();
    refreshMouseOverButton();
    dirty_rect.fill( screen_width, screen_height );
    flush();
}
