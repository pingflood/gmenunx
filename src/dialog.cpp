#include <string>

#include "dialog.h"
#include "gmenu2x.h"

Dialog::Dialog(GMenu2X *gmenu2x) : gmenu2x(gmenu2x) {}

void Dialog::drawTitleIcon(const std::string &icon, bool skinRes, Surface *s) {
	if (s==NULL)
		s = gmenu2x->s;

	Surface *i = NULL;
	if (!icon.empty()) {
		if (skinRes)
			i = gmenu2x->sc.skinRes(icon);
		else
			i = gmenu2x->sc[icon];
	}

	if (i==NULL)
		i = gmenu2x->sc.skinRes("icons/generic.png");

	// i->blit(s,4,(gmenu2x->skinConfInt["topBarHeight"]-32)/2, 32, 32);
	i->blit(s,4, 4, 32, 32);
}


void Dialog::writeTitle(const std::string &title, Surface *s) {
	if (s==NULL)
		s = gmenu2x->s;

	s->write(gmenu2x->titlefont, title, 40, 4 + gmenu2x->titlefont->getHeight()/2, HAlignLeft, VAlignMiddle, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
}

void Dialog::writeSubTitle(const std::string &subtitle, Surface *s) {
	if (s==NULL)
		s = gmenu2x->s;
	s->write(gmenu2x->font, subtitle, 40, 38, HAlignLeft, VAlignBottom, gmenu2x->skinConfColors[COLOR_FONT_ALT], gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
}

// void Dialog::drawTopBar(Surface *s = NULL, const std::string &title = std::string(), const std::string &description = std::string(), const std::string &icon = std::string()) {
void Dialog::drawTopBar(Surface *s = NULL, const std::string &title, const std::string &description, const std::string &icon) {
	if (s==NULL) s = gmenu2x->s;
	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL)
	// 	bar->blit(s, 0, 0);
	// else
	s->box(0, 0, gmenu2x->resX, gmenu2x->skinConfInt["topBarHeight"], gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
	if (!title.empty()) writeTitle(title, s);
	if (!description.empty()) writeSubTitle(description, s);
	if (!icon.empty()) drawTitleIcon(icon, true, s);
}

void Dialog::drawBottomBar(Surface *s) {
	if (s==NULL) s = gmenu2x->s;

	// Surface *bar = sc.skinRes("imgs/bottombar.png");
	// if (bar != NULL)
	// 	bar->blit(s, 0, resY-bar->raw->h);
	// else
	s->box(0, gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"], gmenu2x->resX, gmenu2x->skinConfInt["bottomBarHeight"], gmenu2x->skinConfColors[COLOR_BOTTOM_BAR_BG]);
}
