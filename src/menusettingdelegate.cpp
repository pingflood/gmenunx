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
#include "menusettingdelegate.h"
#include "gmenu2x.h"
#include "iconbutton.h"

MenuSettingDelegate::MenuSettingDelegate(GMenu2X *gmenu2x, const std::string &title,
	const std::string &description, const std::string &_value, msd_callback_t callback)
	: MenuSetting(gmenu2x, title, description), _value(_value), callback(callback)
{
	IconButton *btn;
	btn = new IconButton(gmenu2x, "skin:imgs/buttons/a.png", gmenu2x->tr["Select"]);
	// btn->setAction(MakeDelegate(this, &this->onChange));
	buttonBox.add(btn);
}

void MenuSettingDelegate::draw(int y) {
	MenuSetting::draw(y);
	gmenu2x->s->write( gmenu2x->font, this->_value, 155, y+gmenu2x->font->getHalfHeight(), VAlignMiddle );
}

uint32_t MenuSettingDelegate::manageInput() {
	if (gmenu2x->input[CONFIRM]) this->callback();
	return 0; // SD_NO_ACTION
}

void MenuSettingDelegate::drawSelected(int y) {
	MenuSetting::drawSelected(y);
}

bool MenuSettingDelegate::edited() {
	return false;
}
