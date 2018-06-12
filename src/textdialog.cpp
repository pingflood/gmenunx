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

#include "textdialog.h"
#include "messagebox.h"

using namespace std;

// static SDL_Rect rect;

TextDialog::TextDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, vector<string> *text, const string &backdrop)
	: Dialog(gmenu2x), text(text), title(title), description(description), icon(icon), backdrop(backdrop)
{
	// this->text = text;
	// this->title = title;
	// this->description = description;
	// this->icon = icon;
	preProcess();
}

void TextDialog::preProcess() {
	uint i=0;
	string row;

	while (i<text->size()) {
		//clean this row
		row = trim(text->at(i));

		//check if this row is not too long
		if (gmenu2x->font->getTextWidth(row)>gmenu2x->resX-15) {
			vector<string> words;
			split(words, row, " ");

			uint numWords = words.size();
			//find the maximum number of rows that can be printed on screen
			while (gmenu2x->font->getTextWidth(row)>gmenu2x->resX-15 && numWords>0) {
				numWords--;
				row = "";
				for (uint x=0; x<numWords; x++)
					row += words[x] + " ";
				row = trim(row);
			}

			//if numWords==0 then the string must be printed as-is, it cannot be split
			if (numWords>0) {
				//replace with the shorter version
				text->at(i) = row;

				//build the remaining text in another row
				row = "";
				for (uint x=numWords; x<words.size(); x++)
					row += words[x] + " ";
				row = trim(row);

				if (!row.empty())
					text->insert(text->begin()+i+1, row);
			}
		}
		i++;
	}
}

void TextDialog::drawText(vector<string> *text, uint firstRow, uint rowsPerPage) {
	gmenu2x->s->setClipRect(gmenu2x->listRect);

	for (uint i = firstRow; i < firstRow + rowsPerPage && i < text->size(); i++) {
		int rowY;
		if (text->at(i)=="----") { //draw a line
			rowY = gmenu2x->listRect.y + (int)((i - firstRow + 0.5) * gmenu2x->font->getHeight());
			gmenu2x->s->hline(5, rowY, gmenu2x->resX - 16, 255,255,255,130);
			gmenu2x->s->hline(5, rowY + 1,gmenu2x->resX - 16,0,0,0,130);
		} else {
			rowY = gmenu2x->listRect.y + (i - firstRow) * gmenu2x->font->getHeight();
			gmenu2x->font->write(gmenu2x->s, text->at(i), 5, rowY);
		}
	}

	gmenu2x->s->clearClipRect();
	gmenu2x->drawScrollBar(rowsPerPage, text->size(), firstRow, gmenu2x->listRect);
}

void TextDialog::exec() {
	// gmenu2x->initBG(backdrop);
	gmenu2x->sc[backdrop]->blit(this->bg,0,0);

	bool close = false;

	drawTopBar(this->bg, title, description);

	//link icon
	if (gmenu2x->sc.skinRes(icon)==NULL)
		drawTitleIcon("icons/ebook.png", this->bg);
	else
		drawTitleIcon(icon, this->bg);

	drawBottomBar(this->bg);
	gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Exit"],
	gmenu2x->drawButton(this->bg, "down", gmenu2x->tr["Scroll"],
	gmenu2x->drawButton(this->bg, "up", "", 5)-10));

	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	uint firstRow = 0, rowsPerPage = gmenu2x->listRect.h/gmenu2x->font->getHeight();
	while (!close) {
		this->bg->blit(gmenu2x->s,0,0);
		drawText(text, firstRow, rowsPerPage);
		gmenu2x->s->flip();

		bool inputAction = gmenu2x->input.update();
		if (gmenu2x->inputCommonActions(inputAction)) continue;

		if ( gmenu2x->input[UP  ] && firstRow > 0 ) firstRow--;
		else if ( gmenu2x->input[DOWN] && firstRow + rowsPerPage < text->size() ) firstRow++;
		else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT]) {
			if (firstRow >= rowsPerPage - 1)
				firstRow -= rowsPerPage - 1;
			else
				firstRow = 0;
		}
		else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT]) {
			if (firstRow + rowsPerPage * 2 - 1 < text->size())
				firstRow += rowsPerPage - 1;
			else
				firstRow = max(0,text->size()-rowsPerPage);
		}
		else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) close = true;
	}
}
