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

	gmenu2x->titlefont->setColor(gmenu2x->skinConfColors[COLOR_FONT_ALT])->setOutlineColor(gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	s->write(gmenu2x->titlefont, title, 40, 4 + gmenu2x->titlefont->getHeight()/2, HAlignLeft, VAlignMiddle);
	gmenu2x->titlefont->setColor(gmenu2x->skinConfColors[COLOR_FONT])->setOutlineColor(gmenu2x->skinConfColors[COLOR_FONT_OUTLINE]);
}

void Dialog::writeSubTitle(const std::string &subtitle, Surface *s) {
	if (s==NULL)
		s = gmenu2x->s;
	gmenu2x->font->setColor(gmenu2x->skinConfColors[COLOR_FONT_ALT])->setOutlineColor(gmenu2x->skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	s->write(gmenu2x->font, subtitle, 40, 38, HAlignLeft, VAlignBottom);
	gmenu2x->font->setColor(gmenu2x->skinConfColors[COLOR_FONT])->setOutlineColor(gmenu2x->skinConfColors[COLOR_FONT_OUTLINE]);
}
