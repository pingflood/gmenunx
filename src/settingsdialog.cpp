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

#include "settingsdialog.h"
#include "messagebox.h"
#include "menu.h"

SettingsDialog::SettingsDialog(GMenu2X *gmenu2x, /*Touchscreen &ts,*/ const string &title, const string &icon):
Dialog(gmenu2x, title, "", icon) /*, ts(ts)*/ {}

SettingsDialog::~SettingsDialog() {
	for (uint32_t i = 0; i < voices.size(); i++)
		delete voices[i];
}

bool SettingsDialog::exec() {
	bool inputAction = false;
	uint32_t i, iY, firstElement = 0, action = SD_NO_ACTION, rowHeight, numRows;

	while (loop) {
		gmenu2x->menu->initLayout();
		gmenu2x->font->setSize(gmenu2x->skinConfInt["fontSize"])->setColor(gmenu2x->skinConfColor["font"])->setOutlineColor(gmenu2x->skinConfColor["fontOutline"]);
		gmenu2x->titlefont->setSize(gmenu2x->skinConfInt["fontSizeTitle"])->setColor(gmenu2x->skinConfColor["fontAlt"])->setOutlineColor(gmenu2x->skinConfColor["fontAltOutline"]);
		rowHeight = gmenu2x->font->height() + 1;
		numRows = (gmenu2x->listRect.h - 2)/rowHeight - 1;

		if (selected < 0) selected = voices.size() - 1;
		if (selected >= voices.size()) selected = 0;
		gmenu2x->setInputSpeed();
		voices[selected]->adjustInput();

		this->description = voices[selected]->getDescription();
		drawDialog(gmenu2x->s);

		//Selection
		if (selected >= firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		iY = gmenu2x->listRect.y + 1;
		for (i = firstElement; i < voices.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == selected) {
				gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColor["selectionBg"]);
				voices[selected]->drawSelected(iY);
			}
			voices[i]->draw(iY);
		}

		gmenu2x->drawScrollBar(numRows, voices.size(), firstElement, gmenu2x->listRect);

		gmenu2x->s->flip();
	
		do {
			inputAction = gmenu2x->input->update();
		} while (!inputAction);
		
		if (gmenu2x->inputCommonActions(inputAction)) continue;

		action = SD_NO_ACTION;
		if (!(action = voices[selected]->manageInput())) {
			if (gmenu2x->input->isActive(UP)) 							action = SD_ACTION_UP;
			else if (gmenu2x->input->isActive(DOWN)) 						action = SD_ACTION_DOWN;
			else if (gmenu2x->input->isActive(PAGEUP)) 					action = SD_ACTION_PAGEUP;
			else if (gmenu2x->input->isActive(PAGEDOWN)) 					action = SD_ACTION_PAGEDOWN;
			else if (gmenu2x->input->isActive(SETTINGS)) 					action = SD_ACTION_SAVE;
			else if (gmenu2x->input->isActive(CANCEL) && allowCancel)		action = SD_ACTION_CLOSE;
		}

		switch (action) {
			case SD_ACTION_SAVE:
				save = true;
				loop = false;
				break;
			case SD_ACTION_CLOSE:
				loop = false;
				if (allowCancel && edited()) {
						MessageBox mb(gmenu2x, gmenu2x->tr["Save changes?"], this->icon);
						mb.setButton(CONFIRM, gmenu2x->tr["Yes"]);
						mb.setButton(CANCEL,  gmenu2x->tr["No"]);
						save = (mb.exec() == CONFIRM);
				}
				break;
			case SD_ACTION_UP:
				selected--;
				break;
			case SD_ACTION_DOWN:
				selected++;
				break;
			case SD_ACTION_PAGEUP:
				selected -= numRows;
				if (selected < 0) selected = 0;
				break;
			case SD_ACTION_PAGEDOWN:
				selected += numRows;
				if (selected >= voices.size()) selected = voices.size() - 1;
				break;
		}
	}

	gmenu2x->setInputSpeed();

	return true;
}

void SettingsDialog::addSetting(MenuSetting* set) {
	voices.push_back(set);
}

bool SettingsDialog::edited() {
	for (uint32_t i = 0; i < voices.size(); i++)
		if (voices[i]->edited()) return true;
	return false;
}
