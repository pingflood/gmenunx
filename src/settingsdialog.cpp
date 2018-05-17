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
#include "debug.h"

using namespace std;

SettingsDialog::SettingsDialog(
		GMenu2X *gmenu2x_, Touchscreen &ts_,
		const string &text_, const string &icon)
	: Dialog(gmenu2x_)
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
	gmenu2x->initBG();

	bool close = false, ts_pressed = false;
	uint i, sel = 0, iY, firstElement = 0, action;
	voices[sel]->adjustInput();

	uint rowHeight = gmenu2x->font->getHeight();
	uint numRows = gmenu2x->listRect.h/rowHeight;

	while (!close) {
		bool inputAction = gmenu2x->input.update();
		if (gmenu2x->powerManager(inputAction) || gmenu2x->inputCommonActions()) continue;

		gmenu2x->bg->blit(gmenu2x->s,0,0);

		drawTopBar(gmenu2x->s, text, "", icon);
		drawBottomBar(gmenu2x->s);
		gmenu2x->s->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

		action = SD_NO_ACTION;
		// if (gmenu2x->f200) ts.poll();
		
		writeSubTitle(voices[sel]->getDescription());

		// gmenu2x->font->setColor(gmenu2x->skinConfColors[COLOR_FONT])->setOutlineColor(gmenu2x->skinConfColors[COLOR_FONT_OUTLINE]);

		if (sel>firstElement+numRows-1) firstElement=sel-numRows+1;
		if (sel<firstElement) firstElement=sel;

		//selection
		iY = sel-firstElement;
		iY = gmenu2x->listRect.y + (iY*rowHeight);

		if (sel<voices.size())
			gmenu2x->s->box(gmenu2x->listRect.x, iY+2, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

		//selected option
		voices[sel]->drawSelected(iY);

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

		gmenu2x->drawScrollBar(numRows, voices.size(), firstElement, gmenu2x->listRect);

		gmenu2x->s->flip();

		if ( gmenu2x->input[SETTINGS] ) action = SD_ACTION_SAVE;
		else if ( gmenu2x->input[CANCEL] ) action = SD_ACTION_CLOSE;
		else if ( gmenu2x->input[UP      ] ) action = SD_ACTION_UP;
		else if ( gmenu2x->input[DOWN    ] ) action = SD_ACTION_DOWN;
		else if ( gmenu2x->input[PAGEUP  ] ) sel = (sel < numRows) ? sel = 0 : sel - numRows + 1;
		else if ( gmenu2x->input[PAGEDOWN] ) sel = (sel + numRows >= voices.size()) ? voices.size() - 1 : sel + numRows - 1;
		else action = voices[sel]->manageInput();

		switch (action) {
			case SD_ACTION_SAVE: save = true; close = true; break;
			case SD_ACTION_CLOSE: save = false; close = true; break;
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
	// WARNING("EDITED!!");
	for (uint i=0; i<voices.size(); i++)
		if (voices[i]->edited()) return true;
	return false;
}
