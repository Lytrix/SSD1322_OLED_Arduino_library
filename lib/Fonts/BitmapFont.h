#pragma once
#include <stdint.h>

// Adafruit GFX font structures
typedef struct {
  uint16_t bitmapOffset;
  uint8_t  width;
  uint8_t  height;
  uint8_t  xAdvance;
  int8_t   xOffset;
  int8_t   yOffset;
} GFXglyph;

typedef struct {
  const uint8_t *bitmap;
  const GFXglyph *glyph;
  uint8_t   first;
  uint8_t   last;
  uint8_t   yAdvance;
} GFXfont; 