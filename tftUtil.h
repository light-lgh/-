#ifndef __TFTUTIL_H
#define __TFTUTIL_H

#include <TFT_eSPI.h>
#pragma once
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern TFT_eSprite clk;
extern int backColor;
extern uint16_t backFillColor;
extern uint16_t penColor;
void tftInit();
void reflashTFT();
void setBrightness(uint8_t value);
void drawText(String text);
void draw2LineText(String text1, String text2);
void pushTransparentSprite(TFT_eSprite *sprite, int x, int y, uint16_t transparentColor);
// void setBrightness(uint8_t value)
#endif