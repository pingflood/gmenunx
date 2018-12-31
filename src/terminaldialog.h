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

#ifndef TERMINALDIALOG_H_
#define TERMINALDIALOG_H_

#include <string>
#include "gmenu2x.h"
#include "dialog.h"

using std::string;
using std::vector;

class TerminalDialog : protected Dialog {
protected:
	vector<string> text;
	string title, description, icon, backdrop, rawText = "";

	void preProcess();
	int drawText(vector<string> *text, int32_t firstCol, uint32_t firstRow, uint32_t rowsPerPage);

public:
	TerminalDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, const string &backdrop = "");

	// void appendText(const string &text);
	// void appendFile(const string &file);

	void exec(const string &cmd);
};

#endif /*TERMINALDIALOG_H_*/
