#include "messagebox.h"
#include "browsedialog.h"
#include "FastDelegate.h"
#include "debug.h"

using namespace fastdelegate;
using namespace std;

BrowseDialog::BrowseDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
: Dialog(gmenu2x), title(title), description(description), icon(icon) {
	directoryEnter(gmenu2x->confStr["defaultDir"]);
}
BrowseDialog::~BrowseDialog() {
}

bool BrowseDialog::exec() {
	this->bg = new Surface(gmenu2x->bg); // needed to redraw on child screen return

	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");
	Surface *iconSd = gmenu2x->sc.skinRes("imgs/sd.png");
	Surface *iconCur;

	close = false;

	uint32_t i, iY, firstElement = 0, padding = 6;
	int32_t animation = 0;
	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
	uint32_t numRows = (gmenu2x->listRect.h - 2)/rowHeight - 1;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);
	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	int buttonPos = gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Cancel"], 5);
	if (selected >= size()) selected = 0;

	if (!showFiles && allowSelectDirectory) {
		buttonPos = gmenu2x->drawButton(this->bg, "start", gmenu2x->tr["Select"], buttonPos);
	} else if ((allowEnterDirectory && isDirectory(selected)) || !isDirectory(selected)) {
		buttonPos = gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"], buttonPos);
	}

	string path = getPath();
	if (path.empty() || !dir_exists(path))
		directoryEnter(gmenu2x->confStr["defaultDir"]);

	string preview = getPreview(selected);

	while (!close) {
		bool inputAction = false; int tmpButtonPos = buttonPos;
		this->bg->blit(gmenu2x->s,0,0);

		if (allowDirUp && getPath() != "/") tmpButtonPos = gmenu2x->drawButton(gmenu2x->s, "x", gmenu2x->tr["Folder up"], buttonPos);

		if (!size()) {
			MessageBox mb(gmenu2x, gmenu2x->tr["This directory is empty"]);
			mb.setAutoHide(-1);
			mb.setBgAlpha(0);
			mb.exec();
		} else {
			// Selection
			if (selected >= firstElement + numRows) firstElement = selected - numRows;
			if (selected < firstElement) firstElement = selected;
			string curPath = getPath();

			if (curPath == "/media" && getFile(selected) != ".." && isDirectory(selected)) {
				gmenu2x->drawButton(gmenu2x->s, "select", gmenu2x->tr["Umount"], tmpButtonPos);
			}

			//Files & Directories
			iY = gmenu2x->listRect.y + 1;
			for (i = firstElement; i < size() && i <= firstElement + numRows; i++, iY += rowHeight) {
				if (i == selected) gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

				if (isDirectory(i)) {
					if (at(i) == "..")
						iconCur = iconGoUp;
					else if ((curPath == "/" && getFileName(i) == "media") || curPath == "/media")
						iconCur = iconSd;
					else
						iconCur = iconFolder;
				}

				iconCur->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);

				gmenu2x->s->write(gmenu2x->font, getFileName(i), gmenu2x->listRect.x + 21, iY + rowHeight/2, VAlignMiddle);
			}

			Surface anim = new Surface(gmenu2x->s);

			if (!preview.empty()) {
				if (!gmenu2x->sc.exists(preview + "scaled")) {
					Surface *previm = new Surface(preview);
					gmenu2x->sc.add(previm, preview + "scaled");
					gmenu2x->sc[preview + "scaled"]->softStretch(gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding, true, false);
				}

				do {
					animation += gmenu2x->skinConfInt["previewWidth"] / 8;

					if (animation > gmenu2x->skinConfInt["previewWidth"])
						animation = gmenu2x->skinConfInt["previewWidth"];

					anim.blit(gmenu2x->s,0,0);
					gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_PREVIEW_BG]);
					gmenu2x->sc[preview + "scaled"]->blit(gmenu2x->s, {gmenu2x->resX - animation + padding, gmenu2x->listRect.y + padding, gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding}, HAlignCenter | VAlignMiddle, gmenu2x->resY);
					gmenu2x->s->flip();
					SDL_Delay(10);
				} while (animation < gmenu2x->skinConfInt["previewWidth"]);
			} else {
				while (animation > 0) {
					anim.blit(gmenu2x->s,0,0);
					gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_PREVIEW_BG]);
					gmenu2x->s->flip();
					animation -= gmenu2x->skinConfInt["previewWidth"] / 8;
					if (animation < 0) animation = 0;
					SDL_Delay(10);
				}
			}

			gmenu2x->drawScrollBar(numRows, size(), firstElement, gmenu2x->listRect);
			gmenu2x->s->flip();
		}

		do {
			inputAction = gmenu2x->input.update();
			if (gmenu2x->inputCommonActions(inputAction)) continue;
			if (customAction(inputAction)) continue;

			if (gmenu2x->input[UP]) {
				selected -= 1;
				if (selected < 0) selected = this->size() - 1;
			} else if (gmenu2x->input[DOWN]) {
				selected += 1;
				if (selected >= this->size()) selected = 0;
			} else if (gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT]) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if (gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT]) {
				selected += numRows;
				if (selected >= this->size()) selected = this->size() - 1;
			} else if ( gmenu2x->input[MENU]) {
				if (getPath() == "/media" && getFile(selected) != ".." && isDirectory(selected)) {
					string umount = "sync; umount -fl " + getFilePath(selected) + "; rm -r " + getFilePath(selected);
					system(umount.c_str());
				}
				directoryEnter(getPath()); // refresh
			} else if ( allowDirUp && (gmenu2x->input[MODIFIER] || (gmenu2x->input[CONFIRM] && getFile(selected) == "..")) ) { /*Directory Up */
				selected = 0;
				preview = "";
				if (browse_history.size() > 0) {
					selected = browse_history.back();
					browse_history.pop_back();
				}
				directoryEnter(getPath() + "/..");
			} else if (gmenu2x->input[CONFIRM]) {
				if (allowEnterDirectory && isDirectory(selected)) {
					browse_history.push_back(selected);
					selected = 0;
					directoryEnter(getFilePath(browse_history.back()));
				} else {
					result = true;
					close = true;
				}
			} else if (gmenu2x->input[SETTINGS] && allowSelectDirectory) {
				result = true;
				close = true;
			} else if (gmenu2x->input[CANCEL] || gmenu2x->input[SETTINGS]) {
				result = false;
				close = preview.empty(); //true; // close only if preview is empty.
				preview = "";
			}

			if (gmenu2x->input[UP] || gmenu2x->input[DOWN] || gmenu2x->input[LEFT] || gmenu2x->input[RIGHT] || gmenu2x->input[PAGEUP] || gmenu2x->input[PAGEDOWN]) {
				preview = getPreview(selected);
			}

		} while (!inputAction);
	}
	return result;
}

void BrowseDialog::directoryEnter(const string &path) {
	MessageBox mb(gmenu2x, gmenu2x->tr["Loading"]);
	mb.setAutoHide(-1);
	mb.setBgAlpha(0);
	mb.exec(3e3);

	setPath(path);
	onChangeDir();

	mb.clearTimer();
}
const std::string BrowseDialog::getFile(uint32_t i) {
	return at(i);
}
const std::string BrowseDialog::getFileName(uint32_t i) {
	return at(i);
}
const std::string BrowseDialog::getFilePath(uint32_t i) {
	return getPath() + "/" + getFile(i);
}
const std::string BrowseDialog::getExt(uint32_t i) {
	return file_ext(getFile(i), true);
}
const std::string BrowseDialog::getPreview(uint32_t i) {
	string ext = getExt(i);
	if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp") return getFilePath(i);
	return "";
}
