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

#include "inputdialog.h"
#include "messagebox.h"
#include "debug.h"

using namespace std;
using namespace fastdelegate;

InputDialog::InputDialog(GMenu2X *gmenu2x,
		/*Touchscreen &ts_, */ const string &text,
		const string &startvalue, const string &title, const string &icon)
	: Dialog(gmenu2x) /*, ts(ts_)*/
{
	if (title == "") {
		this->title = text;
		this->text = "";
	} else {
		this->title = title;
		this->text = text;
	}
	this->icon = "";
	if (icon != "" && gmenu2x->sc[icon] != NULL)
		this->icon = icon;

	input = startvalue;
	selCol = 0;
	selRow = 0;
	keyboard.resize(7);

	keyboard[0].push_back("qwertyuiop-789");
	keyboard[0].push_back(" asdfghjkl/456");
	keyboard[0].push_back(" zxcvbnm,.0123");

	keyboard[1].push_back("QWERTYUIOP_-+=");
	keyboard[1].push_back("@ASDFGHJKL'\"`");
	keyboard[1].push_back("#ZXCVBNM:;/?");

	keyboard[2].push_back("¡¿*+-/\\&<=>|");
	keyboard[2].push_back("()[]{}@#$%^~");
	keyboard[2].push_back("_\"'`.,:;!?");

	keyboard[3].push_back("àáâãäåèéêëęěìíîï");
	keyboard[3].push_back("ąćčòóôôõöùúûüůýÿ");
	keyboard[3].push_back("ďĺľłñńňŕřśšťźżž");

	keyboard[4].push_back("ÀÁÂÃÄÅÈÉÊËĘĚÌÍÎÏ");
	keyboard[4].push_back("ĄĆČÒÓÔÔÕÖÙÚÛÜŮÝŸ");
	keyboard[4].push_back("ĎĹĽŁÑŃŇŔŘŚŠŤŹŻŽ");

	keyboard[5].push_back("æçабвгдеёжзий");
	keyboard[5].push_back("клмнопрстуфхцч");
	keyboard[5].push_back("шщъыьэюяøðßÐÞþ");

	keyboard[6].push_back("ÆÇАБВГДЕЁЖЗИЙ");
	keyboard[6].push_back("КЛМНОПРСТУФХЦЧ");
	keyboard[6].push_back("ШЩЪЫЬЭЮЯØðßÐÞþ");

	setKeyboard(0);
}

void InputDialog::setKeyboard(int kb) {
	kb = constrain(kb,0,keyboard.size()-1);
	curKeyboard = kb;
	this->kb = &(keyboard[kb]);
	kbLength = this->kb->at(0).length();

	for (int x = 0, l = kbLength; x < l; x++) {
		if (gmenu2x->font->utf8Code(this->kb->at(0)[x])) {
			kbLength--;
			x++;
		}
	}

	kbLeft = (gmenu2x->resX - kbLength * KEY_WIDTH) / 2;
	kbWidth = kbLength * KEY_WIDTH + 3;
	kbHeight = (this->kb->size()) * KEY_HEIGHT + 3;

	kbRect.x = kbLeft - 3;
	kbRect.y = gmenu2x->bottomBarRect.y - kbHeight;
	kbRect.w = kbWidth;
	kbRect.h = kbHeight;
}

bool InputDialog::exec() {
	bg = new Surface(gmenu2x->s);

	SDL_Rect box = {gmenu2x->listRect.x + 2, 0, gmenu2x->listRect.w - 4, gmenu2x->font->getHeight() + 4};
	box.y = kbRect.y - box.h;

	uint32_t caretTick = 0, curTick;
	bool caretOn = true;

	uint32_t action;
	close = false;
	ok = true;

	this->bg->box(gmenu2x->bottomBarRect, (RGBAColor){0,0,0,255});

	drawBottomBar(this->bg);
	gmenu2x->drawButton(this->bg, "r", gmenu2x->tr["Space"],
	gmenu2x->drawButton(this->bg, "l", gmenu2x->tr["Backspace"],
	gmenu2x->drawButton(this->bg, "y", gmenu2x->tr["Shift"],
	gmenu2x->drawButton(this->bg, "start", gmenu2x->tr["Save"]
	))));

	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	SDL_TimerID wakeUpTimer = NULL;
	while (!close) {
		SDL_RemoveTimer(wakeUpTimer);
		wakeUpTimer = SDL_AddTimer(500, gmenu2x->input.wakeUp, (void*)false);

		this->bg->blit(gmenu2x->s,0,0);

		gmenu2x->s->box(gmenu2x->listRect.x, box.y - 2, gmenu2x->listRect.w, box.h + 2, (RGBAColor){0,0,0,220});
		gmenu2x->s->box(box, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		gmenu2x->s->setClipRect(box);

		gmenu2x->s->write(gmenu2x->font, input, box.x + box.w / 2, box.y + box.h / 2, HAlignCenter | VAlignMiddle);

		curTick = SDL_GetTicks();
		if (curTick - caretTick >= 600) {
			caretOn = !caretOn;
			caretTick = curTick;
		}

		if (caretOn) gmenu2x->s->box(box.x + (box.w + gmenu2x->font->getTextWidth(input)) / 2, box.y + 3, 8, box.h - 6, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		gmenu2x->s->clearClipRect();

		// if (gmenu2x->f200) ts.poll();
		action = drawVirtualKeyboard();
		gmenu2x->s->flip();

		bool inputAction = gmenu2x->input.update();
		if (gmenu2x->inputCommonActions(inputAction)) continue;

		if ( gmenu2x->input[CANCEL] || gmenu2x->input[MENU] ) action = ID_ACTION_CLOSE;
		else if ( gmenu2x->input[SETTINGS] ) action = ID_ACTION_SAVE;
		else if ( gmenu2x->input[UP]       ) action = ID_ACTION_UP;
		else if ( gmenu2x->input[DOWN]     ) action = ID_ACTION_DOWN;
		else if ( gmenu2x->input[LEFT]     ) action = ID_ACTION_LEFT;
		else if ( gmenu2x->input[RIGHT]    ) action = ID_ACTION_RIGHT;
		else if ( gmenu2x->input[CONFIRM]  ) action = ID_ACTION_SELECT;
		else if ( gmenu2x->input[MANUAL]   ) action = ID_ACTION_KB_CHANGE;
		else if ( gmenu2x->input[SECTION_PREV] ) action = ID_ACTION_BACKSPACE;
		else if ( gmenu2x->input[SECTION_NEXT] ) action = ID_ACTION_SPACE;

		switch (action) {
			case ID_ACTION_SAVE:
				ok = true;
				close = true;
				break;
			case ID_ACTION_CLOSE:
				ok = false;
				close = true;
				break;
			case ID_ACTION_UP:
				selRow--;
				break;
			case ID_ACTION_DOWN:
				selRow++;
				if (selRow == (int)kb->size()) selCol = selCol < 8 ? 0 : 1;
				break;
			case ID_ACTION_LEFT:
				selCol--;
				break;
			case ID_ACTION_RIGHT:
				selCol++;
				break;
			case ID_ACTION_BACKSPACE:
				backspace();
				break;
			case ID_ACTION_SPACE:
				space();
				break;
			case ID_ACTION_KB_CHANGE:
				changeKeys();
				break;
			case ID_ACTION_SELECT:
				confirm();
				break;
		}
	}

	gmenu2x->input.dropEvents(); // prevent passing input away

	return ok;
}

void InputDialog::backspace() {
	// check for utf8 characters
	input = input.substr(0,input.length()-( gmenu2x->font->utf8Code(input[input.length()-2]) ? 2 : 1 ));
}

void InputDialog::space() {
	input += " ";
}

void InputDialog::confirm() {
	if (selRow == (int)kb->size()) {
		if (selCol == 0) ok = false;
		close = true;
	} else {
		bool utf8;
		int xc=0;
		for (uint32_t x = 0; x < kb->at(selRow).length(); x++) {
			utf8 = gmenu2x->font->utf8Code(kb->at(selRow)[x]);
			if (xc == selCol) input += kb->at(selRow).substr(x, utf8 ? 2 : 1);
			if (utf8) x++;
			xc++;
		}
	}
}

void InputDialog::changeKeys() {
	if (curKeyboard == 6) setKeyboard(0);
	else setKeyboard(curKeyboard + 1);
}

int InputDialog::drawVirtualKeyboard() {
	int action = ID_NO_ACTION;

	gmenu2x->s->box(gmenu2x->listRect.x, kbRect.y, gmenu2x->listRect.w, kbRect.h, (RGBAColor){0,0,0,220});

	if (selCol < 0) selCol = selRow == (int)kb->size() ? 1 : kbLength - 1;
	if (selCol >= (int)kbLength) selCol = 0;
	if (selRow < 0) selRow = kb->size() - 1;
	if (selRow >= (int)kb->size()) selRow = 0;

	// selection
	if (selRow < (int)kb->size())
		gmenu2x->s->box(kbLeft + selCol * KEY_WIDTH, kbRect.y + 2 + selRow * KEY_HEIGHT, KEY_WIDTH - 1, KEY_HEIGHT - 2, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

	// keys
	for (uint32_t l = 0; l < kb->size(); l++) {
		string line = kb->at(l);
		for (uint32_t x = 0, xc = 0; x < line.length(); x++) {
			string charX;
			// utf8 characters
			if (gmenu2x->font->utf8Code(line[x])) {
				charX = line.substr(x,2);
				x++;
			} else {
				charX = line[x];
			}

			SDL_Rect re = {kbLeft + xc * KEY_WIDTH, kbRect.y + 2 + l * KEY_HEIGHT, KEY_WIDTH - 1, KEY_HEIGHT - 2};

			// if ts on rect, change selection
			// if (gmenu2x->f200 && ts.pressed() && ts.inRect(re)) {
			// 	selCol = xc;
			// 	selRow = l;
			// }

			gmenu2x->s->rectangle(re, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
			gmenu2x->s->write(gmenu2x->font, charX, kbLeft + xc * KEY_WIDTH + KEY_WIDTH / 2, kbRect.y + 2 + l * KEY_HEIGHT + KEY_HEIGHT / 2 - 2, HAlignCenter | VAlignMiddle, (RGBAColor){0xff,0xff,0xff,0xff}, (RGBAColor){0,0,0,200});
			xc++;
		}
	}

	// if ts released
	// if (gmenu2x->f200 && ts.released() && ts.inRect(kbRect)) {
	// 	action = ID_ACTION_SELECT;
	// }

	return action;
}
