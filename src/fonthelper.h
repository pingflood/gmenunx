#ifndef FONTHELPER_H
#define FONTHELPER_H

#include "surface.h"

#include <string>
#include <vector>
#include <SDL_ttf.h>

#ifdef _WIN32
    typedef unsigned int uint;
#endif
using std::vector;
using std::string;

// enum FHHAlign {
// 	HAlignLeft,
// 	HAlignRight,
// 	HAlignCenter
// };

// enum FHVAlign {
// 	VAlignTop,
// 	VAlignBottom,
// 	VAlignMiddle
// };

class Surface;

class FontHelper {
private:

	int height, halfHeight, fontSize;
	TTF_Font *font, *fontOutline;
	RGBAColor textColor, outlineColor;
	string fontName;

public:
	FontHelper(const string &fontName, int size, RGBAColor textColor = (RGBAColor){255,255,255}, RGBAColor outlineColor = (RGBAColor){5,5,5});
	~FontHelper();

	bool utf8Code(unsigned char c);

	void write(Surface *s, const string &text, int x, int y, RGBAColor fgColor, RGBAColor bgColor);
	void write(Surface *s, const string &text, int x, int y);

	// void write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign = 0);
	// void write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor);

	void write(Surface* surface, vector<string> *text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor);
	void write(Surface* surface, vector<string> *text, int x, int y, const unsigned short halign = 0, const unsigned short valign = 0);
	
	void write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor);
	void write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign);

	uint getLineWidth(const string& text);
	uint getTextWidth(const string& text);
	int getTextHeight(const string& text);
	uint getTextWidth(vector<string> *text);
	
	uint getHeight() { return height; };
	uint getHalfHeight() { return halfHeight; };
	

	void loadFont(const string &fontName, int fontSize, RGBAColor textColor, RGBAColor outlineColor);


	FontHelper *setSize(const int size);
	FontHelper *setColor(RGBAColor color);
	FontHelper *setOutlineColor(RGBAColor color);
};

#endif /* FONTHELPER_H */
