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
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <math.h>
#include <fstream>

#include "gmenu2x.h"
#include "linkapp.h"
#include "menu.h"
#include "filelister.h"
#include "utilities.h"
#include "debug.h"

using namespace std;

Menu::Menu(GMenu2X *gmenu2x) {
	this->gmenu2x = gmenu2x;
	iFirstDispSection = 0;

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;

	if ((dirp = opendir("sections")) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] == '.') continue;
		filepath = (string)"sections/" + dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (!S_ISDIR(st.st_mode)) continue;
		if (statRet != -1) {
			sections.push_back((string)dptr->d_name);
			linklist ll;
			links.push_back(ll);
		}
	}

	addSection("settings");
	addSection("applications");

	closedir(dirp);
	sort(sections.begin(),sections.end(),case_less());
	setSectionIndex(0);
	readLinks();
}

Menu::~Menu() {
	freeLinks();
}

void Menu::readLinks() {
	vector<string> linkfiles;

	iLink = 0;
	iFirstDispRow = 0;

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;

	for (uint32_t i = 0; i < links.size(); i++) {
		links[i].clear();
		linkfiles.clear();

		if ((dirp = opendir(sectionPath(i).c_str())) == NULL) {
			continue;
		}

		while ((dptr = readdir(dirp))) {
			if (dptr->d_name[0] == '.') continue;
			filepath = sectionPath(i) + dptr->d_name;
			if (filepath.substr(filepath.size() - 5, 5) == "-opkg") continue;
			int statRet = stat(filepath.c_str(), &st);
			// if (S_ISDIR(st.st_mode)) continue;
			if (statRet != -1 && st.st_mode & S_IFREG) {
				linkfiles.push_back(filepath);
			}
		}

		sort(linkfiles.begin(), linkfiles.end(),case_less());
		for (uint32_t x = 0; x < linkfiles.size(); x++) {
			LinkApp *link = new LinkApp(gmenu2x, gmenu2x->input, linkfiles[x].c_str());
			if (link->targetExists()) {
				links[i].push_back(link);
			} else {
				delete link;
			}
		}

		closedir(dirp);
	}
}

uint32_t Menu::firstDispRow() {
	return iFirstDispRow;
}

// SECTION MANAGEMENT
void Menu::freeLinks() {
	for (vector<linklist>::iterator section = links.begin(); section < links.end(); section++) {
		for (linklist::iterator link = section->begin(); link < section->end(); link++) {
			delete *link;
		}
	}
}

linklist *Menu::sectionLinks(int i) {
	if (i < 0 || i > (int)links.size())
		i = selSectionIndex();

	if (i < 0 || i > (int)links.size())
		return NULL;

	return &links[i];
}

void Menu::decSectionIndex() {
	setSectionIndex(iSection - 1);
}

void Menu::incSectionIndex() {
	setSectionIndex(iSection + 1);
}

uint32_t Menu::firstDispSection() {
	return iFirstDispSection;
}

int Menu::selSectionIndex() {
	return iSection;
}

const string &Menu::selSection() {
	return sections[iSection];
}

const string Menu::selSectionName() {
	string sectionname = sections[iSection];
	string::size_type pos = sectionname.find(".");
	if (pos != string::npos && pos > 0 && pos < sectionname.length()) {
		sectionname = sectionname.substr(pos + 1, sectionname.length());
	}
	return sectionname;
}

int Menu::sectionNumItems() {
	if (gmenu2x->skinConfInt["sectionBar"] == SB_LEFT || gmenu2x->skinConfInt["sectionBar"] == SB_RIGHT)
		return (gmenu2x->resY - 40) / gmenu2x->skinConfInt["sectionBarSize"];
	else if (gmenu2x->skinConfInt["sectionBar"] == SB_TOP || gmenu2x->skinConfInt["sectionBar"] == SB_BOTTOM)
		return (gmenu2x->resX - 40) / gmenu2x->skinConfInt["sectionBarSize"];
	return (gmenu2x->resX / gmenu2x->skinConfInt["sectionBarSize"]) - 1;
}

void Menu::setSectionIndex(int i) {
	if (i < 0)
		i = sections.size() - 1;
	else if (i >= (int)sections.size())
		i = 0;
	iSection = i;

	int numRows = sectionNumItems();
	numRows -= 1;
	if ( i >= (int)iFirstDispSection + numRows)
		iFirstDispSection = i - numRows;
	else if ( i < (int)iFirstDispSection)
		iFirstDispSection = i;
	iLink = 0;
	iFirstDispRow = 0;
}

string Menu::sectionPath(int section) {
	if (section < 0 || section > (int)sections.size()) section = iSection;
	return "sections/" + sections[section] + "/";
}

// LINKS MANAGEMENT
bool Menu::addActionLink(uint32_t section, const string &title, fastdelegate::FastDelegate0<> action, const string &description, const string &icon) {
	if (section >= sections.size()) {
		return false;
	}

	Link *linkact = new Link(gmenu2x, action);
	linkact->setTitle(title);
	linkact->setDescription(description);
	// if (gmenu2x->sc.exists(icon) || (icon.substr(0,5) == "skin:" && !gmenu2x->sc.getSkinFilePath(icon.substr(5, icon.length())).empty()) || file_exists(icon))
	linkact->setIcon("skin:icons/" + icon);
	linkact->setBackdrop("skin:backdrops/" + icon);

	sectionLinks(section)->push_back(linkact);
	return true;
}

bool Menu::addLink(string exec, string section, string title, string description, string icon, string params) {
	if (section.empty()) {
		section = selSection();
	} else if (find(sections.begin(), sections.end(), section) == sections.end()) {
		// section directory doesn't exists
		if (!addSection(section))
			return false;
	}

	string path = dir_name(exec);
	string file = base_name(exec);

	if (title.empty()) {
		title = file;
		// strip the extension from the filename
		string::size_type pos = title.rfind(".");
		if (pos != string::npos && pos > 0) {
			title = title.substr(0, pos);
		}
	}

	string linkpath = "sections/" + section + "/" + title;
	if (section != "favourites") {
		int x = 2;
		while (file_exists(linkpath)) {
			stringstream ss;
			linkpath = ""; ss << x; ss >> linkpath;
			linkpath = "sections/" + section + "/" + title + linkpath;
			x++;
		}
	}

	// Reduce title length to fit the link width
	if ((int)gmenu2x->font->getTextWidth(title) > gmenu2x->linkWidth) {
		while ((int)gmenu2x->font->getTextWidth(title + "..") > gmenu2x->linkWidth) {
			title = title.substr(0, title.length() - 1);
		}
		title += "..";
	}
	
	INFO("Adding link: '%s'", linkpath.c_str());
	ofstream f(linkpath.c_str());
	if (f.is_open()) {
		f << "title=" << title << endl;
		f << "exec=" << exec << endl;

		INFO("ICON: '%s'", icon.c_str());

		if (!icon.empty() && file_exists(icon))
			f << "icon=" << icon << endl;

		if (!params.empty())
			f << "params=" << params << endl;

		if (!description.empty())
			f << "description=" << description << endl;

		string selectoraliases = path + "/aliases.txt";
		if (file_exists(selectoraliases))
			f << "selectoraliases=" << selectoraliases << endl;

		f.close();

		int isection = find(sections.begin(), sections.end(), section) - sections.begin();

		if (isection >= 0 && isection < (int)sections.size()) {
			INFO("Section: '%s(%i)'", sections[isection].c_str(), isection);

			LinkApp *link = new LinkApp(gmenu2x, gmenu2x->input, linkpath.c_str());
			if (link->targetExists())
				links[isection].push_back( link );
			else
				delete link;
		}
		setLinkIndex(links[isection].size() - 1);

	} else {
		ERROR("Error while opening the file '%s' for write.", linkpath.c_str());
		return false;
	}

	return true;
}

bool Menu::addSection(const string &sectionName) {
	string sectiondir = "sections/" + sectionName;
	if (mkdir(sectiondir.c_str(),0777) == 0) {
		sections.push_back(sectionName);
		linklist ll;
		links.push_back(ll);
		return true;
	}
	return false;
}

void Menu::deleteSelectedLink() {
	string iconpath = selLink()->getIconPath();

	INFO("Deleting link '%s'", selLink()->getTitle().c_str());

	if (selLinkApp() != NULL) unlink(selLinkApp()->getFile().c_str());
	sectionLinks()->erase( sectionLinks()->begin() + selLinkIndex() );
	setLinkIndex(selLinkIndex());

	bool icon_used = false;
	for (uint32_t i = 0; i < sections.size(); i++) {
		for (uint32_t j = 0; j < sectionLinks(i)->size(); j++) {
			if (iconpath == sectionLinks(i)->at(j)->getIconPath()) {
				icon_used = true;
			}
		}
	}
	if (!icon_used) {
		gmenu2x->sc.del(iconpath);
	}
}

void Menu::deleteSelectedSection() {
	INFO("Deleting section '%s'", selSection().c_str());

	gmenu2x->sc.del("sections/" + selSection() + ".png");
	links.erase( links.begin() + selSectionIndex() );
	sections.erase( sections.begin() + selSectionIndex() );
	setSectionIndex(0); //reload sections
}

bool Menu::linkChangeSection(uint32_t linkIndex, uint32_t oldSectionIndex, uint32_t newSectionIndex) {
	if (oldSectionIndex < sections.size() && newSectionIndex < sections.size() && linkIndex < sectionLinks(oldSectionIndex)->size()) {
		sectionLinks(newSectionIndex)->push_back(sectionLinks(oldSectionIndex)->at(linkIndex));
		sectionLinks(oldSectionIndex)->erase(sectionLinks(oldSectionIndex)->begin() + linkIndex);
		// Select the same link in the new position
		setSectionIndex(newSectionIndex);
		setLinkIndex(sectionLinks(newSectionIndex)->size() - 1);
		return true;
	}
	return false;
}

void Menu::pageUp() {
	// PAGEUP with left
	if ((int)(iLink - gmenu2x->linkRows - 1) < 0) {
		setLinkIndex(0);
	} else {
		setLinkIndex(iLink - gmenu2x->linkRows + 1);
	}
}

void Menu::pageDown() {
	// PAGEDOWN with right
	if (iLink + gmenu2x->linkRows > sectionLinks()->size()) {
		setLinkIndex(sectionLinks()->size() - 1);
	} else {
		setLinkIndex(iLink + gmenu2x->linkRows - 1);
	}
}

void Menu::linkLeft() {
	setLinkIndex(iLink - 1);
}

void Menu::linkRight() {
	setLinkIndex(iLink + 1);
}

void Menu::linkUp() {
	int l = iLink - gmenu2x->linkCols;
	if (l < 0) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)gmenu2x->linkCols);
		l += (rows * gmenu2x->linkCols);
		if (l >= (int)sectionLinks()->size())
			l -= gmenu2x->linkCols;
	}
	setLinkIndex(l);
}

void Menu::linkDown() {
	uint32_t l = iLink + gmenu2x->linkCols;
	if (l >= sectionLinks()->size()) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)gmenu2x->linkCols);
		uint32_t curCol = (uint32_t)ceil((iLink+1) / (double)gmenu2x->linkCols);
		if (rows > curCol)
			l = sectionLinks()->size() - 1;
		else
			l %= gmenu2x->linkCols;
	}
	setLinkIndex(l);
}

int Menu::selLinkIndex() {
	return iLink;
}

Link *Menu::selLink() {
	if (sectionLinks()->size() == 0) return NULL;
	return sectionLinks()->at(iLink);
}

LinkApp *Menu::selLinkApp() {
	return dynamic_cast<LinkApp*>(selLink());
}

void Menu::setLinkIndex(int i) {
	if (i < 0)
		i = sectionLinks()->size() - 1;
	else if (i >= (int)sectionLinks()->size())
		i = 0;

	if (i >= (int)(iFirstDispRow * gmenu2x->linkCols + gmenu2x->linkCols * gmenu2x->linkRows))
		iFirstDispRow = i / gmenu2x->linkCols - gmenu2x->linkRows + 1;
	else if (i < (int)(iFirstDispRow * gmenu2x->linkCols))
		iFirstDispRow = i / gmenu2x->linkCols;

	iLink = i;
}

void Menu::renameSection(int index, const string &name) {
	sections[index] = name;
}

int Menu::getSectionIndex(const string &name) {
	return distance(sections.begin(), find(sections.begin(), sections.end(), name));
}

const string Menu::getSectionIcon(int i) {
	string mainsectionIcon = "";
	string subsectionIcon = "";
	string::size_type pos = sections[i].rfind(".");
	if (pos != string::npos) subsectionIcon = sections[i].substr(pos, sections[i].length());

	pos = sections[i].find(".");
	if (pos != string::npos) mainsectionIcon = sections[i].substr(0, pos);

	string sectionIcon = gmenu2x->sc.getSkinFilePath("icons/section.png");

	if (!gmenu2x->sc.getSkinFilePath("sections/" + sections[i] + ".png").empty())
		sectionIcon = gmenu2x->sc.getSkinFilePath("sections/" + sections[i] + ".png");
	else if (!gmenu2x->sc.getSkinFilePath("sections/" + subsectionIcon + ".png").empty())
		sectionIcon = gmenu2x->sc.getSkinFilePath("sections/" + subsectionIcon + ".png");
	else if (!gmenu2x->sc.getSkinFilePath("sections/" + mainsectionIcon + ".png").empty())
		sectionIcon = gmenu2x->sc.getSkinFilePath("sections/" + mainsectionIcon + ".png");
	return sectionIcon;
}
