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


// struct tm {
// 	int tm_sec;   // seconds of minutes from 0 to 61
// 	int tm_min;   // minutes of hour from 0 to 59
// 	int tm_hour;  // hours of day from 0 to 24
// 	int tm_mday;  // day of month from 1 to 31
// 	int tm_mon;   // month of year from 0 to 11
// 	int tm_year;  // year since 1900
// 	int tm_wday;  // days since sunday
// 	int tm_yday;  // days since January 1st
// 	int tm_isdst; // hours of daylight savings time
// };

using std::string;
using std::stringstream;
using fastdelegate::MakeDelegate;

MenuSettingDateTime::MenuSettingDateTime(GMenu2X *gmenu2x, const string &name, const string &description, string *_value)
	: MenuSetting(gmenu2x,name,description), _value(_value) {
	IconButton *btn;

// this->value = value;

	selPart = 0;

	sscanf(_value->c_str(), "%d-%d-%d %d:%d", &iyear, &imonth, &iday, &ihour, &iminute);

	this->setYear(iyear);
	this->setMonth(imonth);
	this->setDay(iday);
	this->setHour(ihour);
	this->setMinute(iminute);

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
	gmenu2x->s->write( gmenu2x->font, year + "-" + month + "-" + day + " " + hour + ":" + minute, 153, y+gmenu2x->font->getHalfHeight(), HAlignLeft, VAlignMiddle );
}

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

void MenuSettingDateTime::setYear(short int i) {
	iyear = constrain(i, 1970, 2100);
	// _value->r = r;
	stringstream ss;
	ss << iyear;
	ss >> year;
}

void MenuSettingDateTime::setMonth(short int i) {
	imonth = i;
	if (i < 1) imonth = 12;
	else if (i > 12) imonth = 0;
	// _value->g = g;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << imonth;
	ss >> month;

  // ss << std::setw(2) << std::setfill('0') << 12 << "\n";


}

void MenuSettingDateTime::setDay(short int i) {
	iday = i;
	if (i < 1) iday = 31;
	else if (i > 31) iday = 0;
	// _value->b = b;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << iday;
	ss >> day;
}

void MenuSettingDateTime::setHour(short int i) {
	ihour = i;
	if (i < 0) ihour = 23;
	else if (i > 23) ihour = 0;
	// _value->a = a;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << ihour;
	ss >> hour;
}

void MenuSettingDateTime::setMinute(short int i) {
	iminute = i;
	if (i < 0) iminute = 59;
	else if (i > 59) iminute = 0;
	// _value->a = a;
	stringstream ss;
	ss << std::setw(2) << std::setfill('0') << iminute;
	ss >> minute;
}


void MenuSettingDateTime::setSelPart(unsigned short int i) {
	switch (selPart) {
		case 1: setMonth(i); break;
		case 2: setDay(i); break;
		case 3: setHour(i); break;
		case 4: setMinute(i); break;
		default: setYear(i); break;
	}
}

string MenuSettingDateTime::value() {
	return *_value;
}

unsigned short int MenuSettingDateTime::getSelPart() {
	switch (selPart) {
		default: case 0: return iyear;
		case 1: return imonth;
		case 2: return iday;
		case 3: return ihour;
		case 4: return iminute;
	}
}

void MenuSettingDateTime::adjustInput() {
	gmenu2x->input.setInterval(150, INC );
	gmenu2x->input.setInterval(150, DEC );
}

void MenuSettingDateTime::drawSelected(int y) {
	int x = 153, w = 40; // + selPart * 36;
	switch (selPart) {
		case 1: x += gmenu2x->font->getTextWidth(year + "-"); w = gmenu2x->font->getTextWidth(month); break;
		case 2: x += gmenu2x->font->getTextWidth(year + "-" + month + "-"); w = gmenu2x->font->getTextWidth(day); break;
		case 3: x += gmenu2x->font->getTextWidth(year + "-" + month + "-" + day + " "); w = gmenu2x->font->getTextWidth(hour); break;
		case 4: x += gmenu2x->font->getTextWidth(year + "-" + month + "-" + day + " " + hour + ":"); w = gmenu2x->font->getTextWidth(minute); break;
		default: w = gmenu2x->font->getTextWidth(year); break;
	}
	gmenu2x->s->box( x-2, y+2, w+3, gmenu2x->font->getHeight(), gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	gmenu2x->s->rectangle( x-2, y+2, w+3, gmenu2x->font->getHeight(), 0,0,0,255 );

	MenuSetting::drawSelected(y);
}

bool MenuSettingDateTime::edited() {

DEBUG("WILL SET VALUE: %s", year.c_str());
// _value = year + "-" + month + "-" + day + " " + hour + ":" + minute;


	return true;
	// return originalValue.r != value().r || originalValue.g != value().g || originalValue.b != value().b || originalValue.a != value().a;
}
