#include "skindialog.h"
#include "messagebox.h"
#include "debug.h"

SkinDialog::SkinDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon):
Dialog(gmenu2x, title, description, icon) {}

SkinDialog::~SkinDialog() {
}

bool SkinDialog::exec() {
	bool inputAction = false;
	uint32_t i, iY, firstElement = 0;
	uint32_t rowHeight = gmenu2x->font->height() + 1;
	uint32_t numRows = (gmenu2x->listRect.h - 2) / rowHeight - 1;
	int32_t selected = 0;

	this->showFullPath = true;
	this->showDirectories = true;
	this->showFiles = false;
	addExclude("..");

	browse(dataPath + "/skins");
	skins = getDirectories();

	browse(homePath + "/skins");
	skins.insert(skins.end(), getFiles().begin(), getFiles().end());

	for (uint32_t i = 0; i < skins.size(); i++) {
		if (gmenu2x->confStr["skin"] == skins[i]) {
			selected = i;
			break;
		}
	}

	buttons.push_back({"select", gmenu2x->tr["Menu"]});
	buttons.push_back({"b", gmenu2x->tr["Cancel"]});
	buttons.push_back({"a", gmenu2x->tr["Select"]});

	while (true) {
		if (selected < 0) selected = skins.size() - 1;
		if (selected >= skins.size()) selected = 0;

		// gmenu2x->setBackground(this->bg, skins[selected]);

		gmenu2x->setSkin(skins[selected], false);

		drawDialog(gmenu2x->s);

		if (skins.size() < 1) {
			MessageBox mb(gmenu2x, gmenu2x->tr["No skins available"]);
			mb.setAutoHide(1);
			mb.setBgAlpha(0);
			mb.exec();
		} else {
			// Selection
			if (selected >= firstElement + numRows) firstElement = selected - numRows;
			if (selected < firstElement) firstElement = selected;

			// Files & Directories
			iY = gmenu2x->listRect.y + 1;
			for (i = firstElement; i < skins.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
				if (i == selected) gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
				gmenu2x->s->write(gmenu2x->font, base_name(skins[i]), gmenu2x->listRect.x + 5, iY + rowHeight/2, VAlignMiddle);
			}

			gmenu2x->drawScrollBar(numRows, skins.size(), firstElement, gmenu2x->listRect);

			gmenu2x->s->flip();
		}

		do {
			inputAction = gmenu2x->input.update();
			if (gmenu2x->inputCommonActions(inputAction)) continue;

			if (gmenu2x->input[UP]) {
				selected--;
			} else if (gmenu2x->input[DOWN]) {
				selected++;
			} else if (gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT]) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if (gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT]) {
				selected += numRows;
				if (selected >= skins.size()) selected = skins.size() - 1;
			} else if (gmenu2x->input[MENU] || gmenu2x->input[CANCEL]) {
				return false;
			} else if ((gmenu2x->input[SETTINGS] || gmenu2x->input[CONFIRM]) && skins.size() > 0) {
				skin = skins[selected];
				return true;
			}
		} while (!inputAction);
	}
}
