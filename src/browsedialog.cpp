#include "messagebox.h"
#include "browsedialog.h"
#include "FastDelegate.h"
#include "debug.h"
#include <algorithm>

using namespace fastdelegate;
using namespace std;

BrowseDialog::BrowseDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
: Dialog(gmenu2x), title(title), description(description), icon(icon) {

}
BrowseDialog::~BrowseDialog() {
}

bool BrowseDialog::exec() {
	this->bg = new Surface(gmenu2x->bg); // needed to redraw on child screen return

	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");

	selected = 0;
	close = false;
	bool inputAction = false;

	uint32_t i, iY, firstElement = 0, animation = 0, padding = 6;
	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
	uint32_t numRows = (gmenu2x->listRect.h - 2)/rowHeight - 1;
	string filename;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);
	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	int buttonPos = gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Cancel"], 5);

	if (!showFiles && allowSelectDirectory) {
		buttonPos = gmenu2x->drawButton(this->bg, "start", gmenu2x->tr["Select"], buttonPos);
	} else if ((allowEnterDirectory && isDirectory(selected)) || !isDirectory(selected)) {
		buttonPos = gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"], buttonPos);
	}

	// string path = getPath();
	// if (path.empty() || !dirExists(path))
	// setPath(CARD_ROOT);
	// browse();
	directoryEnter(getPath());

	uint32_t tickStart = SDL_GetTicks();

	while (!close) {
		this->bg->blit(gmenu2x->s,0,0);

		if (!size()) {
			MessageBox mb(gmenu2x, gmenu2x->tr["This directory is empty"]);
			mb.setAutoHide(-1);
			mb.setBgAlpha(0);
			mb.exec();
		} else {
			// Selection
			if (selected >= firstElement + numRows) firstElement = selected - numRows;
			if (selected < firstElement) firstElement = selected;

			if (getPath() == "/media" && getFile() != ".." && isDirectory(selected)) {
				gmenu2x->drawButton(gmenu2x->s, "select", gmenu2x->tr["Umount"], buttonPos);
			}

			//Files & Directories
			iY = gmenu2x->listRect.y + 1;
			for (i = firstElement; i < size() && i <= firstElement + numRows; i++, iY += rowHeight) {
				if (i == selected) gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
				if (isDirectory(i)) {
					if (at(i) == "..")
						iconGoUp->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
					else
						iconFolder->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
				} else {
					iconFile->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
				}
				gmenu2x->s->write(gmenu2x->font, at(i), gmenu2x->listRect.x + 21, iY + rowHeight/2, VAlignMiddle);
			}

			// preview
			filename = getPath() + "/" + getFile();
			string ext = getExt();

			if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif") {
				gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);

				gmenu2x->sc[filename]->softStretch(gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding, true, false);
				gmenu2x->sc[filename]->blit(gmenu2x->s, {gmenu2x->resX - animation + padding, gmenu2x->listRect.y + padding, gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding}, HAlignCenter | VAlignMiddle, gmenu2x->resY);

				if (animation < gmenu2x->skinConfInt["previewWidth"]) {
					animation = intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 110);
					gmenu2x->s->flip();
					gmenu2x->input.setWakeUpInterval(45);
					continue;
				}
			} else {
				if (animation > 0) {
					gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
					animation = gmenu2x->skinConfInt["previewWidth"] - intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 80);
					gmenu2x->s->flip();
					gmenu2x->input.setWakeUpInterval(45);
					continue;
				}
			}
			gmenu2x->input.setWakeUpInterval(1000);

			gmenu2x->drawScrollBar(numRows, size(), firstElement, gmenu2x->listRect);
			gmenu2x->s->flip();
		}
	
		do {
			inputAction = gmenu2x->input.update();
			if (gmenu2x->inputCommonActions(inputAction)) continue;
			if (inputAction) tickStart = SDL_GetTicks();

			uint32_t action = getAction();

		// if (action == BD_ACTION_SELECT && (*fl)[selected] == "..")
			// action = BD_ACTION_GOUP;
			switch (action) {
				case BD_ACTION_CANCEL:
					cancel();
					break;
				case BD_ACTION_CLOSE:
					if (allowSelectDirectory && isDirectory(selected)) confirm();
					// else cancel();
					break;
				case BD_ACTION_UP:
					selected -= 1;
					if (selected < 0) selected = size() - 1;
					break;
				case BD_ACTION_DOWN:
					selected += 1;
					if (selected >= size()) selected = 0;
					break;
				case BD_ACTION_PAGEUP:
					selected -= numRows;
					if (selected < 0) selected = 0;
					break;
				case BD_ACTION_PAGEDOWN:
					selected += numRows;
					if (selected >= size()) selected = size() - 1;
					break;
				case BD_ACTION_GOUP:
					if (allowDirUp) directoryUp();
					break;
				case BD_ACTION_UMOUNT:
					if (getPath() == "/media" && isDirectory(selected)) {
						string umount = "sync; umount -fl " + filename + "; rm -r " + filename;
						system(umount.c_str());
					}
					directoryEnter(getPath()); // refresh
					break;
				case BD_ACTION_SELECT:
					if (allowEnterDirectory && isDirectory(selected)) {
						directoryEnter(filename);
						// directoryEnter();
						break;
					}
			/* Falltrough */
				case BD_ACTION_CONFIRM:
					confirm();
					break;
				default:
					// directoryEnter(getPath()); // refresh
					break;
			}
		} while (!inputAction);
	}
	return result;
}

uint32_t BrowseDialog::getAction() {
	uint32_t action = BD_NO_ACTION;

	if (gmenu2x->input[SETTINGS]) action = BD_ACTION_CLOSE;
	else if (gmenu2x->input[UP]) action = BD_ACTION_UP;
	else if (gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT]) action = BD_ACTION_PAGEUP;
	else if (gmenu2x->input[DOWN]) action = BD_ACTION_DOWN;
	else if (gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT]) action = BD_ACTION_PAGEDOWN;
	else if (gmenu2x->input[MODIFIER]) action = BD_ACTION_GOUP;
	else if (gmenu2x->input[CONFIRM]) action = BD_ACTION_SELECT;
	else if (gmenu2x->input[CANCEL]) action = BD_ACTION_CANCEL;
	else if (gmenu2x->input[MENU]) action = BD_ACTION_UMOUNT;
	return action;
}
void BrowseDialog::directoryUp() {
	string path = getPath();
	string::size_type p = path.rfind("/");
	if (p == path.size() - 1) p = path.rfind("/", p - 1);
	selected = 0;
	directoryEnter("/" + path.substr(0, p));
}
void BrowseDialog::confirm() {
	result = true;
	close = true;
}
void BrowseDialog::cancel() {
	result = false;
	close = true;
}
void BrowseDialog::directoryEnter(const string &path) {
	selected = 0;

	MessageBox mb(gmenu2x, gmenu2x->tr["Loading"]);
	mb.setAutoHide(-1);
	mb.setBgAlpha(0);
	mb.exec(3e3);

	showDirectories = showDirectories;
	showFiles = showFiles;
	allowDirUp = allowDirUp;
	setPath(path);
	onChangeDir();

	mb.clearTimer();
}
std::string BrowseDialog::getFile() {
	return at(selected);
}
const std::string BrowseDialog::getExt() {
	string filename = getFile();
	string ext = "";
	string::size_type pos = filename.rfind(".");
	if (pos != string::npos && pos > 0) {
		ext = filename.substr(pos, filename.length());
		transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) tolower);
	}
	return ext;
}
