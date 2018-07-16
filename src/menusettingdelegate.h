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
#ifndef MENUSETTINGDELEGATE_H
#define MENUSETTINGDELEGATE_H

#include "menusetting.h"

#include "FastDelegate.h"
using fastdelegate::MakeDelegate;

typedef FastDelegate0<> msd_callback_t;

class MenuSettingDelegate : public MenuSetting {
protected:
	std::string _value;
	msd_callback_t callback; // variable to store function pointer type

public:
	virtual ~MenuSettingDelegate() {}
	MenuSettingDelegate(GMenu2X *gmenu2x, const std::string &title,
		const std::string &description, const std::string &value,
		msd_callback_t callback);
	virtual void drawSelected(int y);
	virtual void draw(int y);
	virtual uint32_t manageInput();
	virtual bool edited();
};

#endif
