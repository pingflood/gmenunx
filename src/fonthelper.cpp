#include "fonthelper.h"
#include "utilities.h"
#include "debug.h"

FontHelper::FontHelper(const string &fontName, int fontSize, RGBAColor textColor, RGBAColor outlineColor)
	: fontName(fontName),
	  fontSize(fontSize),
	  textColor(textColor),
	  outlineColor(outlineColor) {
	loadFont(fontName, fontSize, textColor, outlineColor);
}

FontHelper::~FontHelper() {
	TTF_CloseFont(font);
	TTF_CloseFont(fontOutline);
}

void FontHelper::loadFont(const string &fontName, int fontSize, RGBAColor textColor, RGBAColor outlineColor) {
	// if (this->font != NULL) TTF_CloseFont(this->font);
	// if (this->fontOutline != NULL) TTF_CloseFont(this->fontOutline);

	if (!TTF_WasInit()) {
		DEBUG("Initializing font");
		if (TTF_Init() == -1) {
			ERROR("TTF_Init: %s", TTF_GetError());
			exit(2);
		}
	}
	this->font = TTF_OpenFont(fontName.c_str(), fontSize);
	if (!this->font) {
		ERROR("TTF_OpenFont %s: %s", fontName.c_str(), TTF_GetError());
		exit(2);
	}
	this->fontOutline = TTF_OpenFont(fontName.c_str(), fontSize);
	if (!this->fontOutline) {
		ERROR("TTF_OpenFont %s: %s", fontName.c_str(), TTF_GetError());
		exit(2);
	}
	TTF_SetFontHinting(this->font, TTF_HINTING_NORMAL);
	TTF_SetFontHinting(this->fontOutline, TTF_HINTING_NORMAL);
	TTF_SetFontOutline(this->fontOutline, 1);
	height = 0;
	// Get maximum line height with a sample text
	TTF_SizeUTF8(this->font, "AZ0987654321", NULL, &height);
	halfHeight = height/2;
}


bool FontHelper::utf8Code(unsigned char c) {
	return (c>=194 && c<=198) || c==208 || c==209;
}

FontHelper *FontHelper::setSize(const int size) {
	TTF_CloseFont(font);
	TTF_CloseFont(fontOutline);
	fontSize = size;
	loadFont(this->fontName, this->fontSize, this->textColor, this->outlineColor);
	return this;
}

FontHelper *FontHelper::setColor(RGBAColor color) {
	textColor = color;
	return this;
}

FontHelper *FontHelper::setOutlineColor(RGBAColor color) {
	outlineColor = color;
	return this;
}

void FontHelper::write(SDL_Surface *s, const string &text, int x, int y) {
	write(s, text, x, y, textColor, outlineColor);
}

void FontHelper::write(SDL_Surface *s, const string &text, int x, int y, RGBAColor fgColor, RGBAColor bgColor) {
	if (text.empty()) return;

	Surface bg;
	bg.raw = TTF_RenderUTF8_Blended(fontOutline, text.c_str(), rgbatosdl(bgColor));

	Surface fg;
	fg.raw = TTF_RenderUTF8_Blended(font, text.c_str(), rgbatosdl(fgColor));

	// Modify alpha channel of outline and text and merge them in the process
	RGBAColor fgcol, bgcol;
	for (int iy=0; iy<bg.raw->h; iy++)
		for (int ix=0; ix<bg.raw->w; ix++) {
			bgcol = bg.pixelColor(ix, iy);
			if (bgcol.a != 0) {
				bgcol.a = bgcol.a * bgColor.a / 255;
			}
			if (ix > 0 && ix-1 < fg.raw->w && iy > 0 && iy-1 < fg.raw->h) {
				fgcol = fg.pixelColor(ix-1, iy-1);
				if (fgcol.a > 50) {
					bgcol = fgcol;
					bgcol.a = bgcol.a * fgColor.a / 255;
				}
			}
			bg.putPixel(ix, iy, bgcol);
		}

	bg.blit(s, x,y);
}

void FontHelper::write(SDL_Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign) {
	write(surface, text, x, y, halign, valign, textColor, outlineColor);
}

void FontHelper::write(SDL_Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor) {
	switch (halign) {
		case HAlignCenter:
			x -= getTextWidth(text)/2;
		break;
		case HAlignRight:
			x -= getTextWidth(text);
		break;
	}

	switch (valign) {
		case VAlignMiddle:
			y -= getHalfHeight();
		break;
		case VAlignBottom:
			y -= getHeight();
		break;
	}

	write(surface, text, x, y, fgColor, bgColor);
}

void FontHelper::write(SDL_Surface* surface, vector<string> *text, int x, int y, const unsigned short halign, const unsigned short valign) {
	write(surface, text, x, y, halign, valign, textColor, outlineColor);
}

void FontHelper::write(SDL_Surface* surface, vector<string> *text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor) {
	switch (valign) {
		case VAlignMiddle:
			y -= getHalfHeight()*text->size();
		break;
		case VAlignBottom:
			y -= getHeight()*text->size();
		break;
	}

	for (uint i=0; i<text->size(); i++) {
		int ix = x;
		switch (halign) {
			case HAlignCenter:
				ix -= getTextWidth(text->at(i))/2;
			break;
			case HAlignRight:
				ix -= getTextWidth(text->at(i));
			break;
		}

		write(surface, text->at(i), x, y+getHeight()*i, fgColor, bgColor);
	}
}

void FontHelper::write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign) {
	write(surface, text, x, y, halign, valign, textColor, outlineColor);
}

void FontHelper::write(Surface* surface, const string& text, int x, int y, const unsigned short halign, const unsigned short valign, RGBAColor fgColor, RGBAColor bgColor) {
	if (text.find("\n",0)!=string::npos) {
		vector<string> textArr;
		split(textArr,text,"\n");
		write(surface->raw, &textArr, x, y, halign, valign, fgColor, bgColor);
	} else
		write(surface->raw, text, x, y, halign, valign, fgColor, bgColor);
}

uint FontHelper::getLineWidth(const string& text) {
	int width = 0;
	TTF_SizeUTF8(fontOutline, text.c_str(), &width, NULL);
	return width;
}

uint FontHelper::getTextWidth(const string& text) {
	if (text.find("\n",0)!=string::npos) {
		vector<string> textArr;
		split(textArr,text,"\n");
		return getTextWidth(&textArr);
	} else
		return getLineWidth(text);
}

uint FontHelper::getTextWidth(vector<string> *text) {
	int w = 0;
	for (uint i=0; i<text->size(); i++)
		w = max( getLineWidth(text->at(i)), w );
	return w;
}

int FontHelper::getTextHeight(const string& text) {
	vector<string> textArr;
	split(textArr,text,"\n");
	return textArr.size();
}
