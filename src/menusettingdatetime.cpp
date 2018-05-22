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
#include "menusettingdatetime.h"
#include "gmenu2x.h"
#include "debug.h"

#include <sstream>
#include <iomanip>

using std::string;
using std::stringstream;
using fastdelegate::MakeDelegate;

MenuSettingDateTime::MenuSettingDateTime(GMenu2X *gmenu2x, const string &name, const string &description, const string &value)
	: MenuSetting(gmenu2x,name,description) {
	IconButton *btn;

	selPart = 0;
	// _value = value;
	// originalValue = *value;
	// this->setYYYY(this->value().r);
	// this->setMM(this->value().g);
	// this->setDD(this->value().b);
	// this->setHH(this->value().a);

	this->setYYYY(2018);
	this->setMM(05);
	this->setDD(22);
	this->setHH(10);
	this->setmm(54);


	btn = new IconButton(gmenu2x, "skin:imgs/buttons/left.png");
	btn->setAction(MakeDelegate(this, &MenuSettingDateTime::leftComponent));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/right.png", gmenu2x->tr["Component"]);
	btn->setAction(MakeDelegate(this, &MenuSettingDateTime::rightComponent));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/y.png", gmenu2x->tr["Decrease"]);
	btn->setAction(MakeDelegate(this, &MenuSettingDateTime::dec));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/x.png", gmenu2x->tr["Increase"]);
	btn->setAction(MakeDelegate(this, &MenuSettingDateTime::inc));
	buttonBox.add(btn);

}

void MenuSettingDateTime::draw(int y) {
	this->y = y;
	MenuSetting::draw(y);
	// gmenu2x->s->box(153, y + 2 + (gmenu2x->font->getHeight()/2) - 6, 12, 12, value() );
	// gmenu2x->s->rectangle(153, y + 2 + (gmenu2x->font->getHeight()/2) - 6, 12, 12, 0, 0, 0, 255);
	gmenu2x->s->write( gmenu2x->font, YYYY + "-" + MM + "-" + DD + " " + HH + ":" + mm, 153, y+gmenu2x->font->getHalfHeight(), HAlignLeft, VAlignMiddle );
}

// getTextWidth

// void MenuSettingDateTime::handleTS() {
// 	if (gmenu2x->ts.pressed()) {
// 		for (int i = 0; i <= 5; i++) {
// 			if (i != selPart && gmenu2x->ts.inRect(166+i*36, y, 36, 14)) {
// 				selPart = i;
// 				i = 4;
// 			}
// 		}
// 	}

// 	MenuSetting::handleTS();
// }

uint MenuSettingDateTime::manageInput() {
	if (gmenu2x->input[INC]) inc();
	if (gmenu2x->input[DEC]) dec();
	if (gmenu2x->input[LEFT]) leftComponent();
	if (gmenu2x->input[RIGHT]) rightComponent();
}

void MenuSettingDateTime::dec()
{
	// setSelPart(constrain(getSelPart()-1,1970,2100));
	setSelPart(getSelPart()-1);
}

void MenuSettingDateTime::inc()
{
	// setSelPart(constrain(getSelPart()+1,1970,2100));
	setSelPart(getSelPart()+1);
}

void MenuSettingDateTime::leftComponent()
{
	selPart = constrain(selPart-1,0,4);
}

void MenuSettingDateTime::rightComponent()
{
	selPart = constrain(selPart+1,0,4);
}

void MenuSettingDateTime::setYYYY(short int i)
{
	i = constrain(i, 1970, 2100);
	// _value->r = r;
	stringstream ss;
	ss << i;
	ss >> YYYY;
}

void MenuSettingDateTime::setMM(short int i)
{
	if (i < 0) i = 12;
	else if (i > 12) i = 0;
	// _value->g = g;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << i;
	ss >> MM;

  // ss << std::setw(2) << std::setfill('0') << 12 << "\n";


}

void MenuSettingDateTime::setDD(short int i)
{
	if (i < 0) i = 31;
	else if (i > 31) i = 0;
	// _value->b = b;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << i;
	ss >> DD;
}

void MenuSettingDateTime::setHH(short int i)
{
	if (i < 0) i = 23;
	else if (i > 23) i = 0;
	// _value->a = a;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << i;
	ss >> HH;
}

void MenuSettingDateTime::setmm(short int i)
{
	if (i < 0) i = 59;
	else if (i > 59) i = 0;
	// _value->a = a;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << i;
	ss >> mm;
}


void MenuSettingDateTime::setSelPart(unsigned short int value)
{
	switch (selPart) {
		case 1: setMM(value); break;
		case 2: setDD(value); break;
		case 3: setHH(value); break;
		case 4: setmm(value); break;
		default: setYYYY(value); break;
	}
}

// RGBAColor MenuSettingDateTime::value()
// {
// 	return *_value;
// }

unsigned short int MenuSettingDateTime::getSelPart()
{
	switch (selPart) {
		default: case 0: return atoi(YYYY.c_str());
		case 1: return atoi(MM.c_str());
		case 2: return atoi(DD.c_str());
		case 3: return atoi(HH.c_str());
		case 4: return atoi(mm.c_str());
	}
}

void MenuSettingDateTime::adjustInput() {
	gmenu2x->input.setInterval(30, INC );
	gmenu2x->input.setInterval(30, DEC );
}

void MenuSettingDateTime::drawSelected(int y)
{
	int x = 153, w = 40; // + selPart * 36;
	switch (selPart) {
		case 1: x += gmenu2x->font->getTextWidth(YYYY + "-"); w = gmenu2x->font->getTextWidth(MM); break;
		case 2: x += gmenu2x->font->getTextWidth(YYYY + "-" + MM + "-"); w = gmenu2x->font->getTextWidth(DD); break;
		case 3: x += gmenu2x->font->getTextWidth(YYYY + "-" + MM + "-" + DD + " "); w = gmenu2x->font->getTextWidth(HH); break;
		case 4: x += gmenu2x->font->getTextWidth(YYYY + "-" + MM + "-" + DD + " " + HH + ":"); w = gmenu2x->font->getTextWidth(mm); break;
		default: w = gmenu2x->font->getTextWidth(YYYY); break;
	}
	gmenu2x->s->box( x-2, y+2, w+3, gmenu2x->font->getHeight(), gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	gmenu2x->s->rectangle( x-2, y+2, w+3, gmenu2x->font->getHeight(), 0,0,0,255 );

	MenuSetting::drawSelected(y);
}

bool MenuSettingDateTime::edited()
{
	return false;
	// return originalValue.r != value().r || originalValue.g != value().g || originalValue.b != value().b || originalValue.a != value().a;
}
