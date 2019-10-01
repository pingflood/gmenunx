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

// #include <fstream>
// #include <sstream>

using namespace std;

TerminalDialog::TerminalDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, const string &backdrop)
	: Dialog(gmenu2x), title(title), description(description), icon(icon), backdrop(backdrop)
{}

int TerminalDialog::drawText(vector<string> *text, int32_t firstCol, uint32_t firstRow, uint32_t rowsPerPage) {
	gmenu2x->s->setClipRect(gmenu2x->listRect);
	int mx = 0;

	for (uint32_t i = firstRow; i < firstRow + rowsPerPage && i < text->size(); i++) {
		int y = gmenu2x->listRect.y + (i - firstRow) * gmenu2x->font->getHeight();
		mx = max(mx, gmenu2x->font->getTextWidth(text->at(i)));

		if (text->at(i)=="----") { //draw a line
			gmenu2x->s->box(5, y, gmenu2x->w - 10, 1, 255, 255, 255, 130);
			gmenu2x->s->box(5, y + 1, gmenu2x->w - 10, 1, 0, 0, 0, 130);
		} else {
			gmenu2x->font->write(gmenu2x->s, text->at(i), 5 + firstCol, y);
		}
	}

	gmenu2x->s->clearClipRect();
	gmenu2x->drawScrollBar(rowsPerPage, text->size(), firstRow, gmenu2x->listRect);
	return mx;
}

void TerminalDialog::exec(const string &_cmd) {
	string cmd;
	bool close = false, inputAction = false;
	int32_t firstCol = 0, lineWidth = 0;
	uint32_t firstRow = 0, rowsPerPage = gmenu2x->listRect.h/gmenu2x->font->getHeight();

	if (gmenu2x->sc.skinRes(icon) == NULL)
		icon = "icons/terminal.png";

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);

	gmenu2x->drawButton(this->bg, "dpad", gmenu2x->tr["Scroll"],
	gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Exit"],
	5));

	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	this->bg->blit(gmenu2x->s,0,0);

	gmenu2x->s->flip();

	if (file_exists("/usr/bin/script"))
		cmd = "/usr/bin/script -q -c " + cmdclean(_cmd) + " /dev/null 2>&1";
	else
		cmd = "/bin/sh -c " + cmdclean(_cmd) + " 2>&1";

	FILE* pipe = popen(cmd.c_str(), "r");

	if (!pipe) return;
	char buffer[128];

	gmenu2x->powerManager->clearTimer();

	while (!close) {
		do {
			if (pipe) {
				if (!feof(pipe) && fgets(buffer, 128, pipe) != NULL) {
					rawText += buffer;
				} else {
					pclose(pipe);
					pipe = NULL;
					rawText += "\r\n----\r\nDone.";
					system("if [ -d sections/systems ]; then mkdir -p sections/emulators.systems; cp -r sections/systems/* sections/emulators.systems/; rm -rf sections/systems; fi");
					system("sync &");
				}
				InputManager::wakeUp(0, (void*)false);
				split(text, rawText, "\r\n");
				if (text.size() >= rowsPerPage) firstRow = text.size() - rowsPerPage;
			}

			this->bg->blit(gmenu2x->s,0,0);
			lineWidth = drawText(&text, firstCol, firstRow, rowsPerPage);
			gmenu2x->s->flip();

			inputAction = gmenu2x->input.update();

			if ( gmenu2x->input[UP] && firstRow > 0 ) firstRow--;
			else if ( gmenu2x->input[DOWN] && firstRow + rowsPerPage < text.size() ) firstRow++;
			else if ( gmenu2x->input[RIGHT] && firstCol > -1 * (lineWidth - gmenu2x->listRect.w) - 10) firstCol -= 30;
			else if ( gmenu2x->input[LEFT]  && firstCol < 0) firstCol += 30;
			else if ( gmenu2x->input[PAGEUP] || (gmenu2x->input[LEFT] && firstCol == 0)) {
				if (firstRow >= rowsPerPage - 1)
					firstRow -= rowsPerPage - 1;
				else
					firstRow = 0;
			}
			else if ( gmenu2x->input[PAGEDOWN] || (gmenu2x->input[RIGHT] && firstCol == 0)) {
				if (firstRow + rowsPerPage * 2 - 1 < text.size())
					firstRow += rowsPerPage - 1;
				else
					firstRow = max(0,text.size()-rowsPerPage);
			}
			else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) close = true;
		} while (!inputAction);
	}
	pclose(pipe);
}
