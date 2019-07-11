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
#include "messagebox.h"
#include "powermanager.h"
#include "debug.h"

using namespace std;

MessageBox::MessageBox(GMenu2X *gmenu2x, vector<MenuOption> options) {
	Surface bg(gmenu2x->s);
	bool close = false, inputAction = false;
	int sel = 0;
	uint32_t i, fadeAlpha = 0, h = gmenu2x->font->getHeight(), h2 = gmenu2x->font->getHalfHeight();

	SDL_Rect box;
	box.h = h * options.size() + 8;
	box.w = 0;
	for (i = 0; i < options.size(); i++) {
		int w = gmenu2x->font->getTextWidth(options[i].text);
		if (w > box.w) box.w = w;
	}
	box.w += 23;
	box.x = gmenu2x->resX/2 - box.w/2;
	box.y = gmenu2x->resY/2 - box.h/2;

	uint32_t tickStart = SDL_GetTicks();
	while (!close) {
		bg.blit(gmenu2x->s, 0, 0);

		gmenu2x->s->box(0, 0, gmenu2x->resX, gmenu2x->resY, 0,0,0, fadeAlpha);
		gmenu2x->s->box(box.x, box.y, box.w, box.h, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);
		gmenu2x->s->rectangle(box.x + 2, box.y + 2, box.w - 4, box.h - 4, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

		//draw selection rect
		gmenu2x->s->box( box.x + 4, box.y + 4 + h * sel, box.w - 8, h, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_SELECTION] );
		for (i = 0; i < options.size(); i++)
			gmenu2x->s->write(gmenu2x->font, options[i].text, box.x + 12, box.y + h2 + 3 + h * i, VAlignMiddle, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);

		gmenu2x->s->flip();

		if (fadeAlpha < 200) {
			fadeAlpha = intTransition(0, 200, tickStart, 200);
			continue;
		}
		do {
			inputAction = gmenu2x->input.update();

			if (gmenu2x->inputCommonActions(inputAction)) continue;

			if ( gmenu2x->input[MENU] || gmenu2x->input[CANCEL]) close = true;
			else if ( gmenu2x->input[UP] ) sel = (sel - 1 < 0) ? (int)options.size() - 1 : sel - 1 ;
			else if ( gmenu2x->input[DOWN] ) sel = (sel + 1 > (int)options.size() - 1) ? 0 : sel + 1;
			else if ( gmenu2x->input[LEFT] || gmenu2x->input[PAGEUP] ) sel = 0;
			else if ( gmenu2x->input[RIGHT] || gmenu2x->input[PAGEDOWN] ) sel = (int)options.size() - 1;
			else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CONFIRM] ) { 
				options[sel].action();
				close = true; 
			}
		} while (!inputAction);
	}
}

MessageBox::MessageBox(GMenu2X *gmenu2x, const string &text, const string &icon) {
	this->gmenu2x = gmenu2x;
	this->text = text;
	this->icon = icon;
	this->autohide = 0;
	this->bgalpha = 200;

	buttonText.resize(19);
	button.resize(19);
	buttonPosition.resize(19);
	for (uint32_t x = 0; x < buttonText.size(); x++) {
		buttonText[x] = "";
		button[x] = "";
		buttonPosition[x].h = gmenu2x->font->getHeight();
	}

	// Default enabled button
	buttonText[CONFIRM] = "OK";

	// Default labels
	button[UP] = "up";
	button[DOWN] = "down";
	button[LEFT] = "left";
	button[RIGHT] = "right";
	button[MODIFIER] = "a";
	button[CONFIRM] = "a";
	button[CANCEL] = "b";
	button[MANUAL] = "y";
	button[DEC] = "x";
	button[INC] = "y";
	button[SECTION_PREV] = "l";
	button[SECTION_NEXT] = "r";
	button[PAGEUP] = "l";
	button[PAGEDOWN] = "r";
	button[SETTINGS] = "start";
	button[MENU] = "select";
	button[VOLUP] = "vol+";
	button[VOLDOWN] = "vol-";
}
MessageBox::~MessageBox() {
	clearTimer();
}
void MessageBox::setButton(int action, const string &btn) {
	buttonText[action] = btn;
}

void MessageBox::setAutoHide(int autohide) {
	this->autohide = autohide;
}

void MessageBox::setBgAlpha(bool bgalpha) {
	this->bgalpha = bgalpha;
}

int MessageBox::exec() {
	int result = -1;

	gmenu2x->powerManager->clearTimer();

	//Darken background
	gmenu2x->s->box((SDL_Rect){0, 0, gmenu2x->resX, gmenu2x->resY}, (RGBAColor){0,0,0,bgalpha});

	SDL_Rect box;
	box.h = gmenu2x->font->getTextHeight(text) * gmenu2x->font->getHeight() + gmenu2x->font->getHeight();
	if (gmenu2x->sc[icon] != NULL && box.h < 40) box.h = 48;

	box.w = gmenu2x->font->getTextWidth(text) + 24;// + (gmenu2x->sc[icon] != NULL ? 42 : 0);
	int ix = 0;
	for (uint32_t i = 0; i < buttonText.size(); i++) {
		if (!buttonText[i].empty())
			ix += gmenu2x->font->getTextWidth(buttonText[i]) + 24;
	}
	ix += 6;

	if (ix > box.w) box.w = ix;

	ix = (gmenu2x->sc[icon] != NULL ? 42 : 0);
	box.w += ix;

	box.x = (gmenu2x->resX - box.w) / 2 - 2;
	box.y = (gmenu2x->resY - box.h) / 2 - 2;

	//outer box
	gmenu2x->s->box(box, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);
	
	//draw inner rectangle
	gmenu2x->s->rectangle(box.x + 2, box.y + 2, box.w - 4, box.h - 4, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

	//icon+text
	if (gmenu2x->sc[icon] != NULL) {
		gmenu2x->s->setClipRect({box.x + 8, box.y + 8, 32, 32});
		gmenu2x->sc[icon]->blit(gmenu2x->s, box.x + 24, box.y + 24, HAlignCenter | VAlignMiddle);
		gmenu2x->s->clearClipRect();
	}

	// gmenu2x->s->box(ix + box.x, box.y, (box.w - ix), box.h, strtorgba("ffff00ff"));
	gmenu2x->s->write(gmenu2x->font, text, ix + box.x + (box.w - ix) / 2, box.y + box.h / 2, HAlignCenter | VAlignMiddle, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);

	if (this->autohide) {
		gmenu2x->s->flip();
		if (this->autohide > 0) SDL_Delay(this->autohide);
		gmenu2x->powerManager->resetSuspendTimer(); // prevent immediate suspend
		return -1;
	}

	//draw buttons rectangle
	gmenu2x->s->box(box.x, box.y+box.h, box.w, gmenu2x->font->getHeight(), gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);

	int btnX = (gmenu2x->resX + box.w) / 2 - 6;
	for (uint32_t i = 0; i < buttonText.size(); i++) {
		if (buttonText[i] != "") {
			buttonPosition[i].y = box.y+box.h+gmenu2x->font->getHalfHeight();
			buttonPosition[i].w = btnX;

			btnX = gmenu2x->drawButtonRight(gmenu2x->s, button[i], buttonText[i], btnX, buttonPosition[i].y);

			buttonPosition[i].x = btnX;
			buttonPosition[i].w = buttonPosition[i].x-btnX-6;
		}
	}
	gmenu2x->s->flip();

	while (result < 0) {
		//touchscreen
		// if (gmenu2x->f200 && gmenu2x->ts.poll()) {
		// 	for (uint32_t i = 0; i < buttonText.size(); i++) {
		// 		if (buttonText[i] != "" && gmenu2x->ts.inRect(buttonPosition[i])) {
		// 			result = i;
		// 			break;
		// 		}
		// 	}
		// }

		bool inputAction = gmenu2x->input.update();
		if (inputAction) {
			// if (gmenu2x->inputCommonActions(inputAction)) continue; // causes power button bounce
			for (uint32_t i = 0; i < buttonText.size(); i++) {
				if (buttonText[i] != "" && gmenu2x->input[i]) {
					result = i;
					break;
				}
			}
		}
	}

	gmenu2x->input.dropEvents(); // prevent passing input away
	gmenu2x->powerManager->resetSuspendTimer();
	return result;
}

void MessageBox::exec(uint32_t timeOut) {
	clearTimer();
	popupTimer = SDL_AddTimer(timeOut, execTimer, this);
}

void MessageBox::clearTimer() {
	SDL_RemoveTimer(popupTimer); popupTimer = NULL;
}

uint32_t MessageBox::execTimer(uint32_t interval, void *param) {
	MessageBox *mb = reinterpret_cast<MessageBox *>(param);
	mb->clearTimer();
	mb->exec();
	return 0;
}
