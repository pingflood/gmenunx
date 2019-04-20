#include <string>

#include "dialog.h"
#include "gmenu2x.h"

Dialog::Dialog(GMenu2X *gmenu2x) : gmenu2x(gmenu2x) {
	bg = new Surface(gmenu2x->bg);
}

Dialog::~Dialog() {
	delete bg;
}

void Dialog::drawTitleIcon(const std::string &icon, Surface *s) {
	if (s == NULL) s = gmenu2x->s;

	Surface *i = NULL;
	if (!icon.empty()) {
		i = gmenu2x->sc[icon];
		if (i == NULL) i = gmenu2x->sc.skinRes(icon);
	}

	if (i == NULL) i = gmenu2x->sc.skinRes("icons/generic.png");

	gmenu2x->s->setClipRect({4, 4, gmenu2x->skinConfInt["sectionBarSize"] - 8, gmenu2x->skinConfInt["sectionBarSize"] - 8});
	i->blit(s, {4, 4, gmenu2x->skinConfInt["sectionBarSize"] - 8, gmenu2x->skinConfInt["sectionBarSize"] - 8}, HAlignCenter | VAlignMiddle);
	gmenu2x->s->clearClipRect();
	// s->box(4, 4, 32, 32, strtorgba("ffff00ff"));
}

void Dialog::writeTitle(const std::string &title, Surface *s) {
	if (s == NULL) s = gmenu2x->s;
	s->write(gmenu2x->titlefont, title, gmenu2x->skinConfInt["sectionBarSize"], 2, VAlignTop, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	// s->box(40, 16 - gmenu2x->titlefont->getHalfHeight(), 15, gmenu2x->titlefont->getHeight(), strtorgba("ff00ffff"));
}

void Dialog::writeSubTitle(const std::string &subtitle, Surface *s) {
	if (s == NULL) s = gmenu2x->s;
	s->write(gmenu2x->font, subtitle, gmenu2x->skinConfInt["sectionBarSize"], gmenu2x->skinConfInt["sectionBarSize"] - 2, VAlignBottom, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	// s->box(40, 32 - gmenu2x->font->getHalfHeight(), 15, gmenu2x->font->getHeight(), strtorgba("00ffffff"));
}

void Dialog::drawTopBar(Surface *s = NULL, const std::string &title, const std::string &description, const std::string &icon) {
	if (s == NULL) s = gmenu2x->s;
	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL) bar->blit(s, 0, 0);
	// else
	s->setClipRect({0, 0, gmenu2x->resX, gmenu2x->skinConfInt["sectionBarSize"]});
	s->box(0, 0, gmenu2x->resX, gmenu2x->skinConfInt["sectionBarSize"], gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
	if (!title.empty()) writeTitle(title, s);
	if (!description.empty()) writeSubTitle(description, s);
	if (!icon.empty()) drawTitleIcon(icon, s);
	s->clearClipRect();
}

void Dialog::drawBottomBar(Surface *s) {
	if (s == NULL) s = gmenu2x->s;
	// Surface *bar = sc.skinRes("imgs/bottombar.png");
	// if (bar != NULL) bar->blit(s, 0, resY-bar->raw->h);
	// else
	s->box(gmenu2x->bottomBarRect, gmenu2x->skinConfColors[COLOR_BOTTOM_BAR_BG]);
}
