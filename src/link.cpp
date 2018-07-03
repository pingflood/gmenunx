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
#include <fstream>
#include <sstream>
#include "link.h"
#include "menu.h"
#include "selector.h"
// #include "debug.h"
#include "fonthelper.h"

// using namespace std;
// using namespace fastdelegate;

Link::Link(GMenu2X *gmenu2x_, LinkAction action)
	: Button(gmenu2x_->ts, true)
	, gmenu2x(gmenu2x_)
{
	// action = MakeDelegate(this, &Link::run);
	this->action = action;
	edited = false;
	iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");
	// iconX = 0;
	padding = 4;

	updateSurfaces();
}

void Link::run() {
	this->action();
}

void Link::paint() {
	iconSurface->blit(gmenu2x->s, {rect.x + padding, rect.y + padding, rect.w - 2 * padding, rect.h - 2 * padding});
	gmenu2x->s->write(gmenu2x->titlefont, gmenu2x->tr.translate(getTitle()), rect.x + padding + 36, rect.y + gmenu2x->titlefont->getHeight()/2, VAlignMiddle);
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr.translate(getDescription()), rect.x + padding + 36, rect.y + rect.h - padding/2, VAlignBottom);
}

bool Link::paintHover() {
	// if (gmenu2x->useSelectionPng)
	// 	gmenu2x->sc["imgs/selection.png"]->blit(gmenu2x->s,rect,HAlignCenter,VAlignMiddle);
	// else
		gmenu2x->s->box(rect.x, rect.y, rect.w, rect.h, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
	// return true;
}

void Link::updateSurfaces() {
	iconSurface = gmenu2x->sc[getIconPath()];
}

const string &Link::getTitle() {
	return title;
}

void Link::setTitle(const string &title) {
	this->title = title;
	edited = true;
}

const string &Link::getDescription() {
	return description;
}

void Link::setDescription(const string &description) {
	this->description = description;
	edited = true;
}

const string &Link::getIcon() {
	return icon;
}

void Link::setIcon(const string &icon) {
	this->icon = icon;

	if (icon.compare(0, 5, "skin:") == 0)
		this->iconPath = gmenu2x->sc.getSkinFilePath(icon.substr(5, string::npos));
	else
		this->iconPath = icon;

	edited = true;
	updateSurfaces();
}

const string &Link::searchIcon() {
	iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");
	return iconPath;
}

const string &Link::getIconPath() {
	if (iconPath.empty()) searchIcon();

	if (!gmenu2x->sc.getSkinFilePath(iconPath).empty()) {
		iconPath = gmenu2x->sc.getSkinFilePath(iconPath);
	}	else if (!fileExists(iconPath)) {
		iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");
	}

	return iconPath;
}

void Link::setIconPath(const string &icon) {
	if (fileExists(icon))
		iconPath = icon;
	else
		iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");
	updateSurfaces();
}

void Link::setSize(int w, int h) {
	Button::setSize(w,h);
}

void Link::setPosition(int x, int y) {
	Button::setPosition(x,y);
}
