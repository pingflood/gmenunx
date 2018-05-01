/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <SDL.h>
#include <SDL_gfxPrimitives.h>

#include "settingsdialog.h"
#include "messagebox.h"

using namespace std;

SettingsDialog::SettingsDialog(
		GMenu2X *gmenu2x_, InputManager &inputMgr_, Touchscreen &ts_,
		const string &text_, const string &icon)
	: Dialog(gmenu2x_)
	, inputMgr(inputMgr_)
	, ts(ts_)
	, text(text_)
{
	if (icon!="" && gmenu2x->sc[icon] != NULL)
		this->icon = icon;
	else
		this->icon = "icons/generic.png";
}

SettingsDialog::~SettingsDialog() {
	for (uint i=0; i<voices.size(); i++)
		delete voices[i];
}

bool SettingsDialog::exec() {
	//Surface bg (gmenu2x->confStr["wallpaper"],false);
	// Surface bg(gmenu2x->bg);


	gmenu2x->initBG();


	bool close = false, ts_pressed = false;
	uint i, sel = 0, iY, firstElement = 0, action;
	voices[sel]->adjustInput();

	// SDL_Rect rect = {0, gmenu2x->skinConfInt["topBarHeight"], gmenu2x->resX, gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"] - gmenu2x->skinConfInt["topBarHeight"]};
	// SDL_Rect touchRect = {0, gmenu2x->skinConfInt["topBarHeight"], gmenu2x->resX, gmenu2x->listRect.h};
	uint rowHeight = gmenu2x->font->getHeight();
	uint numRows = gmenu2x->listRect.h/rowHeight;

	gmenu2x->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);
	
	gmenu2x->drawTopBar(gmenu2x->bg);
	gmenu2x->drawBottomBar(gmenu2x->bg);

	while (!close) {
		action = SD_NO_ACTION;
		if (gmenu2x->f200) ts.poll();

		gmenu2x->bg->blit(gmenu2x->s,0,0);


		//link icon
		drawTitleIcon(icon);
		writeTitle(text);

		if (sel>firstElement+numRows-1) firstElement=sel-numRows+1;
		if (sel<firstElement) firstElement=sel;

		//selection
		iY = sel-firstElement;
		iY = gmenu2x->listRect.y + (iY*rowHeight);
		gmenu2x->s->setClipRect(gmenu2x->listRect);
		if (sel<voices.size())
			gmenu2x->s->box(gmenu2x->listRect.x, iY+2, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		gmenu2x->s->clearClipRect();

		//selected option
		voices[sel]->drawSelected(iY);

		gmenu2x->s->setClipRect(gmenu2x->listRect);
		if (ts_pressed && !ts.pressed()) ts_pressed = false;
		if (gmenu2x->f200 && ts.pressed() && !ts.inRect(gmenu2x->listRect)) ts_pressed = false;
		for (i=firstElement; i<voices.size() && i<firstElement+numRows; i++) {
			iY = i-firstElement;
			voices[i]->draw(iY*rowHeight+gmenu2x->listRect.y);
			if (gmenu2x->f200 && ts.pressed() && ts.inRect(gmenu2x->listRect.x, gmenu2x->listRect.y+(iY*rowHeight), gmenu2x->listRect.w, rowHeight)) {
				ts_pressed = true;
				sel = i;
			}
		}
		gmenu2x->s->clearClipRect();

		gmenu2x->drawScrollBar(numRows,voices.size(),firstElement,gmenu2x->listRect.y,gmenu2x->listRect.h);

		//description
		writeSubTitle(voices[sel]->getDescription());

		gmenu2x->s->flip();
		voices[sel]->handleTS();

		inputMgr.update();
// COMMON ACTIONS
		if ( inputMgr.isActive(MODIFIER) ) {
			if (inputMgr.isActive(SECTION_NEXT)) {
				if (!gmenu2x->saveScreenshot()) { continue; }
				MessageBox mb(gmenu2x, gmenu2x->tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
				continue;
			} else if (inputMgr.isActive(SECTION_PREV)) {
				int vol = gmenu2x->getVolume();
				if (vol) {
					vol = 0;
					gmenu2x->volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					gmenu2x->volumeMode = VOLUME_MODE_NORMAL;
				}
				gmenu2x->confInt["globalVolume"] = vol;
				gmenu2x->setVolume(vol);
				gmenu2x->writeConfig();
				continue;
			}
		}
		// BACKLIGHT
		else if ( inputMgr[BACKLIGHT] ) gmenu2x->setBacklight(gmenu2x->confInt["backlight"], true);
// END OF COMMON ACTIONS
		else if ( inputMgr[SETTINGS] ) action = SD_ACTION_CLOSE;
		else if ( inputMgr[UP      ] ) action = SD_ACTION_UP;
		else if ( inputMgr[DOWN    ] ) action = SD_ACTION_DOWN;
		else if ( inputMgr[PAGEUP  ] ) sel = (sel < numRows) ? sel = 0 : sel - numRows + 1;
		else if ( inputMgr[PAGEDOWN] ) sel = (sel + numRows >= voices.size()) ? voices.size() - 1 : sel + numRows - 1;
		voices[sel]->manageInput();

		switch (action) {
			case SD_ACTION_CLOSE: close = true; break;
			case SD_ACTION_UP: {
				if (sel==0)
					sel = voices.size()-1;
				else
					sel -= 1;
				gmenu2x->setInputSpeed();
				voices[sel]->adjustInput();
			} break;
			case SD_ACTION_DOWN: {
				sel += 1;
				if (sel>=voices.size()) sel = 0;
				gmenu2x->setInputSpeed();
				voices[sel]->adjustInput();
			} break;
		}
	}

	gmenu2x->setInputSpeed();

	return true;
}

void SettingsDialog::addSetting(MenuSetting* set) {
	voices.push_back(set);
}

bool SettingsDialog::edited() {
	for (uint i=0; i<voices.size(); i++)
		if (voices[i]->edited()) return true;
	return false;
}
