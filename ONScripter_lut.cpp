/* -*- C++ -*-
 * 
 *  ONScripter_lut.cpp - command lookup-table for ONScripter
 *
 *  Copyright (c) 2001-2019 Ogapee. All rights reserved.
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

#include "ONScripter.h"

static ONScripter::FuncLUT func_lut[] = {
    {"zenkakko",		&ONScripter::zenkakkoCommand},

    {"yesnobox",		&ONScripter::yesnoboxCommand},

    {"windoweffect",	&ONScripter::effectCommand},
    {"windowchip",		&ONScripter::windowchipCommand},
    {"windowback",		&ONScripter::windowbackCommand},
    {"wavestop",		&ONScripter::wavestopCommand},
    {"waveloop",		&ONScripter::waveCommand},
    {"wave",			&ONScripter::waveCommand},
    {"waittimer",		&ONScripter::waittimerCommand},
    {"wait",			&ONScripter::waitCommand},

    {"vsp2",			&ONScripter::vspCommand},
    {"vsp",				&ONScripter::vspCommand},
    {"voicevol",		&ONScripter::voicevolCommand},
    {"versionstr",		&ONScripter::versionstrCommand},

    {"usewheel",		&ONScripter::usewheelCommand},
    {"useescspc",		&ONScripter::useescspcCommand},
    {"underline",		&ONScripter::underlineCommand},

    {"trap",			&ONScripter::trapCommand},
    {"transmode",		&ONScripter::transmodeCommand},
    {"transbtn",		&ONScripter::transbtnCommand},
    {"time",			&ONScripter::timeCommand},
    {"textspeeddefault",&ONScripter::textspeeddefaultCommand},
    {"textspeed",		&ONScripter::textspeedCommand},
    {"textshow",		&ONScripter::textshowCommand},
    {"texton",			&ONScripter::textonCommand},
    {"textoff",			&ONScripter::textoffCommand},
    {"texthide",		&ONScripter::texthideCommand},
    {"textgosub",		&ONScripter::textgosubCommand},
    {"textcolor",		&ONScripter::textcolorCommand},
    {"textclear",		&ONScripter::textclearCommand},
    {"textbtnwait",		&ONScripter::btnwaitCommand},
    {"texec",			&ONScripter::texecCommand},
    {"tateyoko",		&ONScripter::tateyokoCommand},
    {"tan",				&ONScripter::tanCommand},
    {"tal",				&ONScripter::talCommand},
    {"tablegoto",		&ONScripter::tablegotoCommand},

    {"systemcall",		&ONScripter::systemcallCommand},
    {"sub",				&ONScripter::subCommand},
    {"strsph",			&ONScripter::strspCommand},
    {"strsp",			&ONScripter::strspCommand},
    {"stralias",		&ONScripter::straliasCommand},
    {"stop",			&ONScripter::stopCommand},
    {"sp_rgb_gradation",&ONScripter::sp_rgb_gradationCommand},
    {"spstr",			&ONScripter::spstrCommand},
    {"spreload",		&ONScripter::spreloadCommand},
    {"splitstring",		&ONScripter::splitCommand},
    {"split",			&ONScripter::splitCommand},
    {"spi",				&ONScripter::soundpressplginCommand},
    {"spclclk",			&ONScripter::spclclkCommand},
    {"spbtn",			&ONScripter::spbtnCommand},
    {"soundpressplgin",	&ONScripter::soundpressplginCommand},
    {"skipoff",			&ONScripter::skipoffCommand},
    {"skip",			&ONScripter::skipCommand},
    {"sin",				&ONScripter::sinCommand},
    {"shadedistance",	&ONScripter::shadedistanceCommand},
    {"sevol",			&ONScripter::sevolCommand},
    {"setwindow3",		&ONScripter::setwindow3Command},
    {"setwindow2",		&ONScripter::setwindow2Command},
    {"setwindow",		&ONScripter::setwindowCommand},
    {"setlayer",		&ONScripter::setlayerCommand},
    {"setkinsoku",		&ONScripter::setkinsokuCommand},
    {"setcursor",		&ONScripter::setcursorCommand},
    {"selnum",			&ONScripter::selectCommand},
    {"selgosub",		&ONScripter::selectCommand},
    {"selectvoice",		&ONScripter::selectvoiceCommand},
    {"selectcolor",		&ONScripter::selectcolorCommand},
    {"selectbtnwait",	&ONScripter::btnwaitCommand},
    {"select",			&ONScripter::selectCommand},
    {"savetime",		&ONScripter::savetimeCommand},
    {"savescreenshot2",	&ONScripter::savescreenshotCommand},
    {"savescreenshot",	&ONScripter::savescreenshotCommand},
    {"savepoint",		&ONScripter::savepointCommand},
    {"saveon",			&ONScripter::saveonCommand},
    {"saveoff",			&ONScripter::saveoffCommand},
    {"savenumber",		&ONScripter::savenumberCommand},
    {"savename",		&ONScripter::savenameCommand},
    {"savegame2",		&ONScripter::savegameCommand},
    {"savegame",		&ONScripter::savegameCommand},
    {"savefileexist",	&ONScripter::savefileexistCommand},
    {"savedir",         &ONScripter::savedirCommand},
    {"sar",				&ONScripter::nsaCommand},

    {"rubyon",			&ONScripter::rubyonCommand},
    {"rubyoff",			&ONScripter::rubyoffCommand},
    {"roff",			&ONScripter::roffCommand},
    {"rnd2",			&ONScripter::rndCommand},
    {"rnd",				&ONScripter::rndCommand},
    {"rmode",			&ONScripter::rmodeCommand},
    {"rmenu",			&ONScripter::rmenuCommand},
    {"return",			&ONScripter::returnCommand},
    {"resettimer",		&ONScripter::resettimerCommand},
    {"reset",			&ONScripter::resetCommand},
    {"repaint",			&ONScripter::repaintCommand},

    {"quakey",			&ONScripter::quakeCommand},
    {"quakex",			&ONScripter::quakeCommand},
    {"quake",			&ONScripter::quakeCommand},

    {"puttext",			&ONScripter::puttextCommand},
    {"prnumclear",		&ONScripter::prnumclearCommand},
    {"prnum",			&ONScripter::prnumCommand},
    {"print",			&ONScripter::printCommand},
    {"pretextgosub",	&ONScripter::pretextgosubCommand},
    {"playstop",		&ONScripter::playstopCommand},
    {"playonce",		&ONScripter::playCommand},
    {"play",			&ONScripter::playCommand},
    {"pagetag",			&ONScripter::pagetagCommand},

    {"okcancelbox",		&ONScripter::yesnoboxCommand},
    {"ofscpy",			&ONScripter::ofscopyCommand},
    {"ofscopy",			&ONScripter::ofscopyCommand},

    {"numalias",		&ONScripter::numaliasCommand},
    {"nsadir",			&ONScripter::nsadirCommand},
    {"nsa",				&ONScripter::nsaCommand},
    {"notif",			&ONScripter::ifCommand},
    {"next",			&ONScripter::nextCommand},
    {"nsa",				&ONScripter::arcCommand},
    {"ns3",				&ONScripter::nsaCommand},
    {"ns2",				&ONScripter::nsaCommand},
    {"nega",			&ONScripter::negaCommand},
    {"nextcsel",		&ONScripter::nextcselCommand},

    {"mul",				&ONScripter::mulCommand},
    {"msp2",			&ONScripter::mspCommand},
    {"msp",				&ONScripter::mspCommand},
    {"mpegplay",		&ONScripter::mpegplayCommand},
    {"mp3vol",			&ONScripter::mp3volCommand},
    {"mp3stop",			&ONScripter::mp3stopCommand},
    {"mp3save",			&ONScripter::mp3Command},
    {"mp3loop",			&ONScripter::mp3Command},
    {"mp3fadeout",		&ONScripter::mp3fadeoutCommand},
    {"mp3fadein",		&ONScripter::mp3fadeinCommand},
    {"mp3",				&ONScripter::mp3Command},
    {"movl",			&ONScripter::movCommand},
    {"movie",			&ONScripter::movieCommand},
    {"movemousecursor",	&ONScripter::movemousecursorCommand},
    {"mov9",			&ONScripter::movCommand},
    {"mov8",			&ONScripter::movCommand},
    {"mov7",			&ONScripter::movCommand},
    {"mov6",			&ONScripter::movCommand},
    {"mov5",			&ONScripter::movCommand},
    {"mov4",			&ONScripter::movCommand},
    {"mov3",			&ONScripter::movCommand},
    {"mov10",			&ONScripter::movCommand},
    {"mov",				&ONScripter::movCommand},
    {"monocro",			&ONScripter::monocroCommand},
    {"mode_saya",		&ONScripter::mode_sayaCommand},
    {"mode_ext",		&ONScripter::mode_extCommand},
    {"mod",				&ONScripter::modCommand},
    {"mid",				&ONScripter::midCommand},
    {"menu_window",		&ONScripter::menu_windowCommand},
    {"menu_full",		&ONScripter::menu_fullCommand},
    {"menu_click_page",	&ONScripter::menu_click_pageCommand},
    {"menu_click_def",	&ONScripter::menu_click_defCommand},
    {"menu_automode",	&ONScripter::menu_automodeCommand},
    {"menusetwindow",	&ONScripter::menusetwindowCommand},
    {"menuselectvoice",	&ONScripter::menuselectvoiceCommand},
    {"menuselectcolor",	&ONScripter::menuselectcolorCommand},
    {"maxkaisoupage",	&ONScripter::maxkaisoupageCommand},

    {"luasub",			&ONScripter::luasubCommand},
    {"luacall",			&ONScripter::luacallCommand},
    {"lsph2sub",		&ONScripter::lsp2Command},
    {"lsph2add",		&ONScripter::lsp2Command},
    {"lsph2",			&ONScripter::lsp2Command},
    {"lsph",			&ONScripter::lspCommand},
    {"lsp2sub",			&ONScripter::lsp2Command},
    {"lsp2add",			&ONScripter::lsp2Command},
    {"lsp2",			&ONScripter::lsp2Command},
    {"lsp",				&ONScripter::lspCommand},
    {"lr_trap",			&ONScripter::trapCommand},
    {"lrclick",			&ONScripter::clickCommand},
    {"loopbgmstop",		&ONScripter::loopbgmstopCommand},
    {"loopbgm",			&ONScripter::loopbgmCommand},
    {"lookbacksp",		&ONScripter::lookbackspCommand},
    {"lookbackflush",	&ONScripter::lookbackflushCommand},
    {"lookbackcolor",	&ONScripter::lookbackcolorCommand},
    {"lookbackbutton",	&ONScripter::lookbackbuttonCommand},
    {"logsp2",			&ONScripter::logspCommand},
    {"logsp",			&ONScripter::logspCommand},
    {"locate",			&ONScripter::locateCommand},
    {"loadgosub",		&ONScripter::loadgosubCommand},
    {"loadgame",		&ONScripter::loadgameCommand},
    {"linepage2",		&ONScripter::linepageCommand},
    {"linepage",		&ONScripter::linepageCommand},
    {"len",				&ONScripter::lenCommand},
    {"ld",				&ONScripter::ldCommand},
    {"layermessage",	&ONScripter::layermessageCommand},
    {"langjp",			&ONScripter::langjpCommand},
    {"langen",			&ONScripter::langenCommand},
    {"labellog",		&ONScripter::labellogCommand},

    {"kinsoku",			&ONScripter::kinsokuCommand},

    {"jumpf",			&ONScripter::jumpfCommand},
    {"jumpb",			&ONScripter::jumpbCommand},

    {"kidokuskip",		&ONScripter::kidokuskipCommand},
    {"kidokumode",		&ONScripter::kidokumodeCommand},

    {"itoa2",			&ONScripter::itoaCommand},
    {"itoa",			&ONScripter::itoaCommand},
    {"isskip",			&ONScripter::isskipCommand},
    {"ispage",			&ONScripter::ispageCommand},
    {"isfull",			&ONScripter::isfullCommand},
    {"isdown",			&ONScripter::isdownCommand},
    {"intlimit",		&ONScripter::intlimitCommand},
    {"input",			&ONScripter::inputCommand},
    {"indent",			&ONScripter::indentCommand},
    {"inc",				&ONScripter::incCommand},
    {"if",				&ONScripter::ifCommand},

    {"humanz",			&ONScripter::humanzCommand},
    {"humanorder",		&ONScripter::humanorderCommand},

    {"goto",			&ONScripter::gotoCommand},
    {"gosub",			&ONScripter::gosubCommand},
    {"globalon",		&ONScripter::globalonCommand},
    {"getzxc",			&ONScripter::getzxcCommand},
    {"getvoicevol",		&ONScripter::getvoicevolCommand},
    {"getversion",		&ONScripter::getversionCommand},
    {"gettimer",		&ONScripter::gettimerCommand},
    {"gettext",			&ONScripter::gettextCommand},
    {"gettaglog",		&ONScripter::gettaglogCommand},
    {"gettag",			&ONScripter::gettagCommand},
    {"gettab",			&ONScripter::gettabCommand},
    {"getspsize",		&ONScripter::getspsizeCommand},
    {"getsppos",		&ONScripter::getspposCommand},
    {"getspmode",		&ONScripter::getspmodeCommand},
    {"getsevol",		&ONScripter::getsevolCommand},
    {"getscreenshot",	&ONScripter::getscreenshotCommand},
    {"getsavestr",		&ONScripter::getsavestrCommand},
    {"getret",			&ONScripter::getretCommand},
    {"getreg",			&ONScripter::getregCommand},
    {"getreadlang",		&ONScripter::getreadlangCommand},
    {"getparam2",		&ONScripter::getparamCommand},
    {"getparam",		&ONScripter::getparamCommand},
    {"getpageup",		&ONScripter::getpageupCommand},
    {"getpage",			&ONScripter::getpageCommand},
    {"getmp3vol",		&ONScripter::getmp3volCommand},
    {"getmousepos",		&ONScripter::getmouseposCommand},
    {"getmouseover",	&ONScripter::getmouseoverCommand},
    {"getmclick",		&ONScripter::getmclickCommand},
    {"getlogtext",		&ONScripter::getlogCommand},
    {"getlog",			&ONScripter::getlogCommand},
    {"getinsert",		&ONScripter::getinsertCommand},
    {"getfunction",		&ONScripter::getfunctionCommand},
    {"getenter",		&ONScripter::getenterCommand},
    {"getcursorpos2",	&ONScripter::getcursorpos2Command},
    {"getcursorpos",	&ONScripter::getcursorposCommand},
    {"getcursor",		&ONScripter::getcursorCommand},
    {"getcselstr",		&ONScripter::getcselstrCommand},
    {"getcselnum",		&ONScripter::getcselnumCommand},
    {"getbtntimer",		&ONScripter::gettimerCommand},
    {"getbgmvol",		&ONScripter::getmp3volCommand},
    {"game",			&ONScripter::gameCommand},

    {"for",				&ONScripter::forCommand},
    {"filelog",			&ONScripter::filelogCommand},
    {"fileexist",		&ONScripter::fileexistCommand},

    {"existspbtn",		&ONScripter::spbtnCommand},
    {"exec_dll",		&ONScripter::exec_dllCommand},
    {"exbtn_d",			&ONScripter::exbtnCommand},
    {"exbtn",			&ONScripter::exbtnCommand},
    {"erasetextwindow",	&ONScripter::erasetextwindowCommand},
    {"english",			&ONScripter::englishCommand},
    {"end",				&ONScripter::endCommand},
    {"effectcut",		&ONScripter::effectcutCommand},
    {"effectblank",		&ONScripter::effectblankCommand},
    {"effect",			&ONScripter::effectCommand},

    {"dwavestop",		&ONScripter::dwavestopCommand},
    {"dwaveplayloop",	&ONScripter::dwaveCommand},
    {"dwaveplay",		&ONScripter::dwaveCommand},
    {"dwaveloop",		&ONScripter::dwaveCommand},
    {"dwaveload",		&ONScripter::dwaveCommand},
    {"dwave",			&ONScripter::dwaveCommand},
    {"drawtext",		&ONScripter::drawtextCommand},
    {"drawsp3",			&ONScripter::drawsp3Command},
    {"drawsp2",			&ONScripter::drawsp2Command},
    {"drawsp",			&ONScripter::drawspCommand},
    {"drawfill",		&ONScripter::drawfillCommand},
    {"drawclear",		&ONScripter::drawclearCommand},
    {"drawbg2",			&ONScripter::drawbg2Command},
    {"drawbg",			&ONScripter::drawbgCommand},
    {"draw",			&ONScripter::drawCommand},
    {"div",				&ONScripter::divCommand},
    {"dim",				&ONScripter::dimCommand},
    {"delay",			&ONScripter::delayCommand},
    {"defvoicevol",		&ONScripter::defvoicevolCommand},
    {"defsub",			&ONScripter::defsubCommand},
    {"defsevol",		&ONScripter::defsevolCommand},
    {"defmp3vol",		&ONScripter::defmp3volCommand},
    {"definereset",		&ONScripter::defineresetCommand},
    {"defaultspeed",	&ONScripter::defaultspeedCommand},
    {"defaultfont",		&ONScripter::defaultfontCommand},
    {"dec",				&ONScripter::decCommand},
    {"date",			&ONScripter::dateCommand},

    {"csp2",			&ONScripter::cspCommand},
    {"csp",				&ONScripter::cspCommand},
    {"cselgoto",		&ONScripter::cselgotoCommand},
    {"cselbtn",			&ONScripter::cselbtnCommand},
    {"csel",			&ONScripter::selectCommand},
    {"cos",				&ONScripter::cosCommand},
    {"cmp",				&ONScripter::cmpCommand},
    {"clickvoice",		&ONScripter::clickvoiceCommand},
    {"clickstr",		&ONScripter::clickstrCommand},
    {"click",			&ONScripter::clickCommand},
    {"cl",				&ONScripter::clCommand},
    {"chvol",			&ONScripter::chvolCommand},
    {"checkpage",		&ONScripter::checkpageCommand},
    {"checkkey",		&ONScripter::checkkeyCommand},
    {"cellcheckspbtn",	&ONScripter::spbtnCommand},
    {"cellcheckexbtn",	&ONScripter::exbtnCommand},
    {"cell",			&ONScripter::cellCommand},
    {"caption",			&ONScripter::captionCommand},

    {"btrans",			&ONScripter::transbtnCommand},
    {"btnwait2",		&ONScripter::btnwaitCommand},
    {"btnwait",			&ONScripter::btnwaitCommand},
    {"btntime2",		&ONScripter::btntimeCommand},
    {"btntime",			&ONScripter::btntimeCommand},
    {"btndown",			&ONScripter::btndownCommand},
    {"btndef",			&ONScripter::btndefCommand},
    {"btn",				&ONScripter::btnCommand},
    {"btime",			&ONScripter::btntimeCommand},
    {"bsp",				&ONScripter::bspCommand},
    {"break",			&ONScripter::breakCommand},
    {"br",				&ONScripter::brCommand},
    {"blt",				&ONScripter::bltCommand},
    {"bgmvol",			&ONScripter::mp3volCommand},
    {"bgmstop",			&ONScripter::mp3stopCommand},
    {"bgmonce",			&ONScripter::mp3Command}, 
    {"bgmfadeout",		&ONScripter::mp3fadeoutCommand},
    {"bgmfadein",		&ONScripter::mp3fadeinCommand},
    {"bgm",				&ONScripter::mp3Command}, 
    {"bgcpy",			&ONScripter::bgcopyCommand},
    {"bgcopy",			&ONScripter::bgcopyCommand},
    {"bg",				&ONScripter::bgCommand},
    {"bexec",			&ONScripter::btnwaitCommand},
    {"bdown",			&ONScripter::bdownCommand},
    {"bdef",			&ONScripter::exbtnCommand},
    {"bcursor",			&ONScripter::getcursorCommand},
    {"bclear",			&ONScripter::btndefCommand},
    {"barclear",		&ONScripter::barclearCommand},
    {"bar",				&ONScripter::barCommand},

    {"avi",				&ONScripter::aviCommand},
    {"autosaveoff",		&ONScripter::autosaveoffCommand},
    {"automode_time",	&ONScripter::automode_timeCommand},
    {"automode",		&ONScripter::mode_extCommand},
    {"autoclick",		&ONScripter::autoclickCommand},
    {"atoi",			&ONScripter::atoiCommand},
    {"arc",				&ONScripter::arcCommand},
    {"amsp2",			&ONScripter::amspCommand},
    {"amsp",			&ONScripter::amspCommand},
    {"allspresume",		&ONScripter::allspresumeCommand},
    {"allsphide",		&ONScripter::allsphideCommand},
    {"allsp2resume",	&ONScripter::allsp2resumeCommand},
    {"allsp2hide",		&ONScripter::allsp2hideCommand},
    {"addkinsoku",		&ONScripter::addkinsokuCommand},
    {"add",				&ONScripter::addCommand},
    {"abssetcursor",	&ONScripter::setcursorCommand},

    {"", NULL}
};

void ONScripter::makeFuncLUT()
{
    for (int i='z'-'a' ; i>=0 ; i--)
        func_hash[i].func = NULL;

    int idx = 0;
    while (func_lut[idx].method){
        int j = func_lut[idx].command[0]-'a';
        if (func_hash[j].func == NULL) func_hash[j].func = func_lut+idx;
        func_hash[j].num = func_lut+idx - func_hash[j].func + 1;
        idx++;
    }
}
