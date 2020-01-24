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

#include "terminaldialog.h"
#include "messagebox.h"
#include "utilities.h"
#include "powermanager.h"
#include "debug.h"

using namespace std;

TerminalDialog::TerminalDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, const string &backdrop):
TextDialog(gmenu2x, title, description, icon, backdrop) {}

void TerminalDialog::exec(string cmd) {
	rowsPerPage = gmenu2x->listRect.h/gmenu2x->font->getHeight();

	if (gmenu2x->sc.skinRes(this->icon) == NULL)
		this->icon = "icons/terminal.png";

	buttons.push_back({"skin:imgs/manual.png", gmenu2x->tr["Running.. Please wait.."]});

	drawDialog(this->bg);

	gmenu2x->s->flip();

	if (file_exists("/usr/bin/script"))
		cmd = "/usr/bin/script -q -c " + cmdclean(cmd) + " /dev/null 2>&1";
	else
		cmd = "/bin/sh -c " + cmdclean(cmd) + " 2>&1";

	FILE* pipe = popen(cmd.c_str(), "r");

	if (!pipe) return;
	char buffer[128];

	gmenu2x->powerManager->clearTimer();

	while (!feof(pipe) && fgets(buffer, 128, pipe) != NULL) {
		rawText += buffer;
		split(text, rawText, "\r\n");
		lineWidth = drawText(&text, firstCol, -1, rowsPerPage);
	}

	pclose(pipe);
	pipe = NULL;

	text.push_back("----");
	text.push_back(gmenu2x->tr["Done"]);
	if (text.size() >= rowsPerPage) firstRow = text.size() - rowsPerPage;

	system("sync &");

	buttons.clear();
	buttons.push_back({"dpad", gmenu2x->tr["Scroll"]});
	buttons.push_back({"b", gmenu2x->tr["Exit"]});

	gmenu2x->bg->blit(this->bg,0,0);
	drawDialog(this->bg);

	while (true) {
		lineWidth = drawText(&text, firstCol, firstRow, rowsPerPage);

		inputAction = gmenu2x->input.update();

		if (gmenu2x->input[UP] && firstRow > 0) firstRow--;
		else if (gmenu2x->input[DOWN] && firstRow + rowsPerPage < text.size()) firstRow++;
		else if (gmenu2x->input[RIGHT] && firstCol > -1 * (lineWidth - gmenu2x->listRect.w) - 10) firstCol -= 30;
		else if (gmenu2x->input[LEFT]  && firstCol < 0) firstCol += 30;
		else if (gmenu2x->input[PAGEUP] || (gmenu2x->input[LEFT] && firstCol == 0)) {
			if (firstRow >= rowsPerPage - 1)
				firstRow -= rowsPerPage - 1;
			else
				firstRow = 0;
		}
		else if (gmenu2x->input[PAGEDOWN] || (gmenu2x->input[RIGHT] && firstCol == 0)) {
			if (firstRow + rowsPerPage * 2 - 1 < text.size())
				firstRow += rowsPerPage - 1;
			else
				firstRow = max(0,text.size()-rowsPerPage);
		}
		else if (gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL]) {
			gmenu2x->powerManager->resetSuspendTimer();
			return;
		}
	}
}
