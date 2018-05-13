#include <string>
#include <iostream>
#include "browsedialog.h"
#include "FastDelegate.h"
#include "filelister.h"
#include "gmenu2x.h"
#include "messagebox.h"

using namespace fastdelegate;
using namespace std;

BrowseDialog::BrowseDialog(GMenu2X *gmenu2x, const string &title, const string &subtitle)
	: Dialog(gmenu2x), title(title), subtitle(subtitle), ts_pressed(false), buttonBox(gmenu2x) {
	IconButton *btn;

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/start.png", gmenu2x->tr["Exit"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::cancel));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/b.png", gmenu2x->tr["Up one folder"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::directoryUp));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/a.png", gmenu2x->tr["Select"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::directoryEnter));
	buttonBox.add(btn);

	iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	iconFile = gmenu2x->sc.skinRes("imgs/file.png");
}

BrowseDialog::~BrowseDialog() {}

bool BrowseDialog::exec() {
	gmenu2x->initBG();

	// moved out of the loop to fix weird scroll behavior
	unsigned int i, iY;
	unsigned int firstElement=0; //, lastElement;
	unsigned int offsetY;
	// Surface *icon;

	if (!fl)
		return false;

	string path = fl->getPath();
	if (path.empty() || !dirExists(path) || path.compare(0,CARD_ROOT_LEN,CARD_ROOT)!=0)
		setPath(CARD_ROOT);

	fl->browse();

	// clipRect = (SDL_Rect){0, gmenu2x->skinConfInt["topBarHeight"], gmenu2x->resX, gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"] - gmenu2x->skinConfInt["topBarHeight"]};
	// touchRect = gmenu2x->listRect; //(SDL_Rect){2, gmenu2x->skinConfInt["topBarHeight"]+4, gmenu2x->resX-12, gmenu2x->listRect.h};

	rowHeight = gmenu2x->font->getHeight()+1;
	numRows = gmenu2x->listRect.h/rowHeight - 1;

	selected = 0;
	close = false;

	gmenu2x->drawTopBar(gmenu2x->bg);
	gmenu2x->drawBottomBar(gmenu2x->bg);
	gmenu2x->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	while (!close) {
		if (gmenu2x->f200) gmenu2x->ts.poll();

		gmenu2x->bg->blit(gmenu2x->s, 0, 0);

		beforeFileList();

		drawTitleIcon("icons/explorer.png", true);
		writeTitle(title);
		writeSubTitle(subtitle);

		buttonBox.paint(5);

		if (selected>firstElement+numRows) firstElement=selected-numRows;
		if (selected<firstElement) firstElement=selected;

		//paint();
		offsetY = gmenu2x->listRect.y;

		//Selection
		iY = selected-firstElement;
		iY = offsetY + iY*rowHeight;
		gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);


		//Files & Directories
		gmenu2x->s->setClipRect(gmenu2x->listRect);
		for (i = firstElement; i < fl->size() && i <= firstElement+numRows; i++) {

			if (fl->isDirectory(i)) {
				if ((*fl)[i] == "..")
						iconGoUp->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2);
					else
						iconFolder->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2);
				} else{
					iconFile->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2);
				}
				gmenu2x->s->write(gmenu2x->font, (*fl)[i], gmenu2x->listRect.x + 21, offsetY+4, HAlignLeft, VAlignMiddle);

			if (gmenu2x->f200 && gmenu2x->ts.pressed() && gmenu2x->ts.inRect(gmenu2x->listRect.x, offsetY + 3, gmenu2x->listRect.w, rowHeight)) {
				ts_pressed = true;
				selected = i;
			}

			offsetY += rowHeight;
		}
		gmenu2x->s->clearClipRect();

		gmenu2x->drawScrollBar(numRows, fl->size(), firstElement, gmenu2x->listRect);
		gmenu2x->s->flip();

		handleInput();
	}
	return result;
}

BrowseDialog::Action BrowseDialog::getAction() {
	BrowseDialog::Action action = BrowseDialog::ACT_NONE;

// COMMON ACTIONS
	if ( gmenu2x->input.isActive(MODIFIER) ) {
		if (gmenu2x->input.isActive(SECTION_NEXT)) {
			if (gmenu2x->saveScreenshot()) {
				MessageBox mb(gmenu2x, gmenu2x->tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
			}
		} else if (gmenu2x->input.isActive(SECTION_PREV)) {
			int vol = gmenu2x->getVolume();
			if (vol) {
				vol = 0;
				gmenu2x->volumeMode = VOLUME_MODE_MUTE;
			} else {
				vol = 100;
				gmenu2x->volumeMode = VOLUME_MODE_NORMAL;
			}
			gmenu2x->confInt["globalVolume"] = vol;
			gmenu2x->setVolume(vol);
			gmenu2x->writeConfig();
		}
	}
	// BACKLIGHT
	else if ( gmenu2x->input[BACKLIGHT] ) gmenu2x->setBacklight(gmenu2x->confInt["backlight"], true);
// END OF COMMON ACTIONS
	else if (gmenu2x->input[MENU])
		action = BrowseDialog::ACT_CLOSE;
	else if (gmenu2x->input[UP])
		action = BrowseDialog::ACT_UP;
	else if (gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT])
		action = BrowseDialog::ACT_SCROLLUP;
	else if (gmenu2x->input[DOWN])
		action = BrowseDialog::ACT_DOWN;
	else if (gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT])
		action = BrowseDialog::ACT_SCROLLDOWN;
	else if (gmenu2x->input[CANCEL])
		action = BrowseDialog::ACT_GOUP;
	else if (gmenu2x->input[CONFIRM])
		action = BrowseDialog::ACT_SELECT;
	else if (gmenu2x->input[SETTINGS]) {
			action = BrowseDialog::ACT_CLOSE;
		}
	return action;
}

void BrowseDialog::handleInput() {
	BrowseDialog::Action action;

	gmenu2x->input.update();

	if (ts_pressed && !gmenu2x->ts.pressed()) {
		action = BrowseDialog::ACT_SELECT;
		ts_pressed = false;
	} else {
		action = getAction();
	}

	if (gmenu2x->f200 && gmenu2x->ts.pressed() && !gmenu2x->ts.inRect(gmenu2x->listRect)) ts_pressed = false;
	// if (action == BrowseDialog::ACT_SELECT && (*fl)[selected] == "..")
		// action = BrowseDialog::ACT_GOUP;
	switch (action) {
	case BrowseDialog::ACT_CLOSE:
		if (fl->isDirectory(selected)) {
			confirm();
		} else {
			cancel();
		}
		break;
	case BrowseDialog::ACT_UP:
		if (selected == 0)
			selected = fl->size() - 1;
		else
			selected -= 1;
		break;
	case BrowseDialog::ACT_SCROLLUP:
		if (selected < numRows)
			selected = 0;
		else
			selected -= numRows;
		break;
	case BrowseDialog::ACT_DOWN:
		if (fl->size() - 1 <= selected)
			selected = 0;
		else
			selected += 1;
		break;
	case BrowseDialog::ACT_SCROLLDOWN:
		if (selected + numRows >= fl->size())
			selected = fl->size()-1;
		else
			selected += numRows;
		break;
	case BrowseDialog::ACT_GOUP:
		directoryUp();
		break;
	case BrowseDialog::ACT_SELECT:
		if (fl->isDirectory(selected)) {
			directoryEnter();
			break;
		}
		/* Falltrough */
	case BrowseDialog::ACT_CONFIRM:
		confirm();
		break;
	default:
		break;
	}

	buttonBox.handleTS();
}

void BrowseDialog::directoryUp() {
	string path = fl->getPath();
	string::size_type p = path.rfind("/");

	if (p == path.size() - 1)
		p = path.rfind("/", p - 1);

	if (p == string::npos || p < 4 || path.compare(0, CARD_ROOT_LEN, CARD_ROOT) != 0) {
		close = true;
		result = false;
	} else {
		selected = 0;
		setPath("/"+path.substr(0, p));
	}
}

void BrowseDialog::directoryEnter() {
	string path = fl->getPath();
	setPath(path + "/" + fl->at(selected));
	selected = 0;
}

void BrowseDialog::confirm() {
	result = true;
	close = true;
}

void BrowseDialog::cancel() {
	result = false;
	close = true;
}
