#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <string>

class GMenu2X;
class Surface;

class Dialog
{
public:
	Dialog(GMenu2X *gmenu2x);

protected:
	// static const std::string empty = std::string();
	void drawTitleIcon(const std::string &icon, bool skinRes = false, Surface *s = NULL);
	void writeTitle(const std::string &title, Surface *s = NULL);
	void writeSubTitle(const std::string &subtitle, Surface *s = NULL);
	// void drawTopBar(Surface *s = NULL, const std::string &title = std::string(), const std::string &description = std::string(), const std::string &icon = std::string());
	void drawTopBar(Surface *s, const std::string &title = {}, const std::string &description = {}, const std::string &icon = {});
	void drawBottomBar(Surface *s=NULL);

	GMenu2X *gmenu2x;
};

#endif
