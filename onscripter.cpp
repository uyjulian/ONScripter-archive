/* -*- C++ -*-
 * 
 *  onscripter.cpp -- main function of ONScripter
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

void optionHelp()
{
    printf( "Usage: onscripter [option ...]\n" );
    printf( "      --cdaudio\t\tuse CD audio if available\n");
    printf( "      --font font_file\t\tuse font_file as a default font\n");
    printf( "  -h, --help\t\tdisplay this help and exit\n");
    printf( "  -v, --version\t\toutput version information and exit\n");
    exit(0);
}

void optionVersion()
{
    printf("ONScripter version %s\n", ONS_VERSION );
    printf("Written by Ogapee<ogapee@aqua.dti2.ne.jp>\n\n");
    printf("Copyright (c) 2001-2002 Ogapee.\n");
    printf("This is free software; see the source for copying conditions.\n");
    exit(0);
}

int main( int argc, char **argv )
{
    bool cdaudio_flag = false;
    char *default_font = NULL;
    
    /* ---------------------------------------- */
    /* Parse options */
    argv++;
    while( argc > 1 ){
        if ( argv[0][0] == '-' ){
            if ( !strcmp( argv[0]+1, "h" ) || !strcmp( argv[0]+1, "-help" ) ){
                optionHelp();
            }
            else if ( !strcmp( argv[0]+1, "v" ) || !strcmp( argv[0]+1, "-version" ) ){
                optionVersion();
            }
            else if ( !strcmp( argv[0]+1, "-cdaudio" ) ){
                cdaudio_flag = true;
            }
            else if ( !strcmp( argv[0]+1, "-font" ) ){
                argc--;
                argv++;
                if ( default_font ) delete[] default_font;
                default_font = new char[ strlen( argv[0] ) + 1 ];
                memcpy( default_font, argv[0], strlen( argv[0] ) + 1 );
            }
            else{
                optionHelp();
            }
        }
        else{
            optionHelp();
        }
        argc--;
        argv++;
    }
    
    /* ---------------------------------------- */
    /* Run ONScripter */
    ONScripterLabel *ons = new ONScripterLabel( cdaudio_flag, default_font );
    ons->eventLoop();
    
    exit(0);
}
