#include <string>

#include "dialog.h"
#include "gmenu2x.h"

Dialog::Dialog(GMenu2X *gmenu2x) : gmenu2x(gmenu2x) {
	bg = new Surface(gmenu2x->bg);
}

Dialog::~Dialog() {
	delete bg;
}

void Dialog::drawTopBar(Surface *s = NULL, const std::string &title, const std::string &description, const std::string &icon) {
	if (s == NULL) s = gmenu2x->s;
	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL) bar->blit(s, 0, 0);
	// else
	s->setClipRect({0, 0, gmenu2x->w, gmenu2x->skinConfInt["sectionBarSize"]});
	s->box(0, 0, gmenu2x->w, gmenu2x->skinConfInt["sectionBarSize"], gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);

	int iconOffset = 2;

	if (!icon.empty() && gmenu2x->skinConfInt["showDialogIcon"]) { // drawTitleIcon
		Surface *i = NULL;

		i = gmenu2x->sc[icon];
		if (i == NULL) i = gmenu2x->sc.skinRes(icon);
		if (i == NULL) i = gmenu2x->sc.skinRes("icons/generic.png");

		iconOffset = gmenu2x->skinConfInt["sectionBarSize"];
		gmenu2x->s->setClipRect({4, 4, iconOffset - 8, iconOffset - 8});
		i->blit(s, {4, 4, iconOffset - 8, iconOffset - 8}, HAlignCenter | VAlignMiddle);
		gmenu2x->s->clearClipRect();
	}

	if (!title.empty()) // writeTitle
		s->write(gmenu2x->titlefont, title, iconOffset, 2, VAlignTop, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);

	if (!description.empty()) // writeSubTitle
		s->write(gmenu2x->font, description, iconOffset, gmenu2x->skinConfInt["sectionBarSize"] - 2, VAlignBottom, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);

	s->clearClipRect();
}

void Dialog::drawBottomBar(Surface *s) {
	if (s == NULL) s = gmenu2x->s;
	// Surface *bar = sc.skinRes("imgs/bottombar.png");
	// if (bar != NULL) bar->blit(s, 0, gmenu2x->h - bar->raw->h);
	// else
	s->box(gmenu2x->bottomBarRect, gmenu2x->skinConfColors[COLOR_BOTTOM_BAR_BG]);
}
