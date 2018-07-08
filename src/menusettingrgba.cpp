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
#include "menusettingrgba.h"
#include "gmenu2x.h"

#include <sstream>

using std::string;
using std::stringstream;
using fastdelegate::MakeDelegate;

MenuSettingRGBA::MenuSettingRGBA(GMenu2X *gmenu2x, const string &name, const string &description, RGBAColor *value)
	: MenuSetting(gmenu2x,name,description) {
	IconButton *btn;

	selPart = 0;
	_value = value;
	originalValue = *value;
	this->setR(this->value().r);
	this->setG(this->value().g);
	this->setB(this->value().b);
	this->setA(this->value().a);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/left.png");
	btn->setAction(MakeDelegate(this, &MenuSettingRGBA::leftComponent));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/right.png", gmenu2x->tr["Component"]);
	btn->setAction(MakeDelegate(this, &MenuSettingRGBA::rightComponent));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/y.png", gmenu2x->tr["Decrease"]);
	btn->setAction(MakeDelegate(this, &MenuSettingRGBA::dec));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/x.png", gmenu2x->tr["Increase"]);
	btn->setAction(MakeDelegate(this, &MenuSettingRGBA::inc));
	buttonBox.add(btn);
}

void MenuSettingRGBA::draw(int y) {
	this->y = y;
	MenuSetting::draw(y);
	gmenu2x->s->box(153, y + (gmenu2x->font->getHeight()/2) - 6, 12, 12, value());
	gmenu2x->s->rectangle(153, y + (gmenu2x->font->getHeight()/2) - 6, 12, 12, 0, 0, 0, 255);
	gmenu2x->s->write(gmenu2x->font, /*"R: "+*/ strR, 169, y + gmenu2x->font->getHalfHeight(), VAlignMiddle);
	gmenu2x->s->write(gmenu2x->font, /*"G: "+*/ strG, 205, y + gmenu2x->font->getHalfHeight(), VAlignMiddle);
	gmenu2x->s->write(gmenu2x->font, /*"B: "+*/ strB, 241, y + gmenu2x->font->getHalfHeight(), VAlignMiddle);
	gmenu2x->s->write(gmenu2x->font, /*"A: "+*/ strA, 277, y + gmenu2x->font->getHalfHeight(), VAlignMiddle);
}

void MenuSettingRGBA::handleTS() {
	if (gmenu2x->ts.pressed()) {
		for (int i=0; i<4; i++) {
			if (i!=selPart && gmenu2x->ts.inRect(166+i*36,y,36,14)) {
				selPart = i;
				i = 4;
			}
		}
	}

	MenuSetting::handleTS();
}

uint32_t MenuSettingRGBA::manageInput() {
	if (gmenu2x->input[INC]) inc();
	if (gmenu2x->input[DEC]) dec();
	if (gmenu2x->input[LEFT]) leftComponent();
	if (gmenu2x->input[RIGHT]) rightComponent();
}

void MenuSettingRGBA::dec() {
	setSelPart(constrain(getSelPart()-1,0,255));
}

void MenuSettingRGBA::inc() {
	setSelPart(constrain(getSelPart()+1,0,255));
}

void MenuSettingRGBA::leftComponent() {
	selPart = constrain(selPart-1,0,3);
}

void MenuSettingRGBA::rightComponent() {
	selPart = constrain(selPart+1,0,3);
}

void MenuSettingRGBA::setR(uint16_t r) {
	_value->r = r;
	stringstream ss;
	ss << r; ss >> strR;
}

void MenuSettingRGBA::setG(uint16_t g) {
	_value->g = g;
	stringstream ss;
	ss << g; ss >> strG;
}

void MenuSettingRGBA::setB(uint16_t b) {
	_value->b = b;
	stringstream ss;
	ss << b; ss >> strB;
}

void MenuSettingRGBA::setA(uint16_t a) {
	_value->a = a;
	stringstream ss;
	ss << a; ss >> strA;
}

void MenuSettingRGBA::setSelPart(uint16_t value) {
	switch (selPart) {
		case 1: setG(value); break;
		case 2: setB(value); break;
		case 3: setA(value); break;
		default: setR(value); break;
	}
}

RGBAColor MenuSettingRGBA::value() {
	return *_value;
}

uint16_t MenuSettingRGBA::getSelPart() {
	switch (selPart) {
		case 1: return value().g;
		case 2: return value().b;
		case 3: return value().a;
		default: return value().r;
	}
}

void MenuSettingRGBA::adjustInput() {
	gmenu2x->input.setInterval(30, INC );
	gmenu2x->input.setInterval(30, DEC );
}

void MenuSettingRGBA::drawSelected(int y) {
	int x = 166 + selPart * 36;

	RGBAColor color;
	switch (selPart) {
		case 0: color = (RGBAColor){255,   0,   0, 255}; break;
		case 1: color = (RGBAColor){  0, 255,   0, 255}; break;
		case 2: color = (RGBAColor){  0,   0, 255, 255}; break;
		default: color = gmenu2x->skinConfColors[COLOR_SELECTION_BG]; break;
	}
	gmenu2x->s->box( x, y, 36, gmenu2x->font->getHeight() + 1, color );
	gmenu2x->s->rectangle( x, y, 36, gmenu2x->font->getHeight() + 1, 0,0,0,255 );
	MenuSetting::drawSelected(y);
}

bool MenuSettingRGBA::edited() {
	return originalValue.r != value().r || originalValue.g != value().g || originalValue.b != value().b || originalValue.a != value().a;
}
