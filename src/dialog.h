#ifndef __DIALOG_H__
#define __DIALOG_H__

// #include <string>

class GMenu2X;
class Surface;

class Dialog
{
public:
	Dialog(GMenu2X *gmenu2x);

protected:
	~Dialog();

	Surface *bg;
	void drawTopBar(Surface *s, const std::string &title = {}, const std::string &description = {}, const std::string &icon = {});
	void drawBottomBar(Surface *s=NULL);

	GMenu2X *gmenu2x;
};

#endif
