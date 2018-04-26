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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>

#include "messagebox.h"

using namespace std;

MessageBox::MessageBox(GMenu2X *gmenu2x, const string &text, const string &icon) {
	this->gmenu2x = gmenu2x;
	this->text = text;
	this->icon = icon;
	this->autohide = 0;

	buttons.resize(19);
	buttonLabels.resize(19);
	buttonPositions.resize(19);
	for (uint x=0; x<buttons.size(); x++) {
		buttons[x] = "";
		buttonLabels[x] = "";
		buttonPositions[x].h = gmenu2x->font->getHeight();
	}

	//Default enabled button
	buttons[CONFIRM] = "OK";

	//Default labels
	buttonLabels[UP] = "up";
	buttonLabels[DOWN] = "down";
	buttonLabels[LEFT] = "left";
	buttonLabels[RIGHT] = "right";
	buttonLabels[MODIFIER] = "a";
#if defined(TARGET_RS97)
	buttonLabels[CONFIRM] = "a";
	buttonLabels[CANCEL] = "b";
#else
	buttonLabels[CONFIRM] = "b";
	buttonLabels[CANCEL] = "x";
#endif
	buttonLabels[MANUAL] = "y";
	buttonLabels[DEC] = "x";
	buttonLabels[INC] = "y";
	buttonLabels[SECTION_PREV] = "l";
	buttonLabels[SECTION_NEXT] = "r";
	buttonLabels[PAGEUP] = "l";
	buttonLabels[PAGEDOWN] = "r";
	buttonLabels[SETTINGS] = "start";
	buttonLabels[MENU] = "select";
	buttonLabels[VOLUP] = "vol+";
	buttonLabels[VOLDOWN] = "vol-";
}

void MessageBox::setButton(int action, const string &btn) {
	buttons[action] = btn;
}

void MessageBox::setAutoHide(int autohide) {
	this->autohide = autohide;
}

int MessageBox::exec() {
	int result = -1;
	int inputAction = 0;

	// Surface bg(gmenu2x->s);
	//Darken background
	gmenu2x->s->box(0, 0, gmenu2x->resX, gmenu2x->resY, 0,0,0,200);

	SDL_Rect box;
	box.h = gmenu2x->font->getHeight()*2.5;
	box.w = gmenu2x->font->getTextWidth(text) + 24 + (gmenu2x->sc[icon] != NULL ? 37 : 0);
	box.x = gmenu2x->halfX - box.w/2 -2;
	box.y = gmenu2x->halfY - box.h/2 -2;

	//outer box
	gmenu2x->s->box(box, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);
	//draw inner rectangle
	gmenu2x->s->rectangle(box.x+2, box.y+2, box.w-4, box.h-4,
	gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BORDER]);
	//icon+text
	if (gmenu2x->sc[icon] != NULL)
		gmenu2x->sc[icon]->blitCenter( gmenu2x->s, box.x+25, box.y+gmenu2x->font->getHeight()+7 );
	gmenu2x->s->write( gmenu2x->font, text, box.x+(gmenu2x->sc[icon] != NULL ? 47 : 10), gmenu2x->halfY, HAlignLeft, VAlignMiddle );

	if (this->autohide) {
		gmenu2x->s->flip();
		SDL_Delay(this->autohide);
		return -1;
	}
	//draw buttons rectangle
	gmenu2x->s->box(box.x, box.y+box.h, box.w, gmenu2x->font->getHeight(), gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);

	int btnX = gmenu2x->halfX+box.w/2-6;
	for (uint i=0; i<buttons.size(); i++) {
		if (buttons[i] != "") {
			buttonPositions[i].y = box.y+box.h+gmenu2x->font->getHalfHeight();
			buttonPositions[i].w = btnX;

			btnX = gmenu2x->drawButtonRight(gmenu2x->s, buttonLabels[i], buttons[i], btnX, buttonPositions[i].y);

			buttonPositions[i].x = btnX;
			buttonPositions[i].w = buttonPositions[i].x-btnX-6;
		}
	}
	gmenu2x->s->flip();

	while (result<0) {
		//touchscreen
		if (gmenu2x->f200) {
			if (gmenu2x->ts.poll()) {
				for (uint i=0; i<buttons.size(); i++)
					if (buttons[i]!="" && gmenu2x->ts.inRect(buttonPositions[i])) {
						result = i;
						i = buttons.size();
					}
			}
		}

		inputAction = gmenu2x->input.update(0);
		if (inputAction) {
// COMMON ACTIONS
			if ( gmenu2x->input.isActive(MODIFIER) ) {
				if (gmenu2x->input.isActive(SECTION_NEXT)) {
					if (!gmenu2x->saveScreenshot()) { continue; }
					MessageBox mb(gmenu2x, gmenu2x->tr["Screenshot Saved"]);
					mb.setAutoHide(1000);
					mb.exec();
					continue;
				} else if (gmenu2x->input.isActive(SECTION_PREV)) {
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
			else if ( gmenu2x->input[BACKLIGHT] ) gmenu2x->setBacklight(gmenu2x->confInt["backlight"], true);
// END OF COMMON ACTIONS

			for (uint i=0; i<buttons.size(); i++) {
				if (buttons[i]!="" && gmenu2x->input[i]) result = i;
			}
		}

		//usleep(LOOP_DELAY);
		gmenu2x->s->flip();
		usleep(50000);
	}

	return result;
}
