/**
 ****************************************************************************************
 *
 * \file SSD1322_GFX.h
 *
 * \brief Simple GFX library to draw some basic shapes on OLED display.
 *
 *
 * Copyright (C) 2020 Wojciech Klimek
 * MIT license:
 * https://github.com/wjklimek1/SSD1322_OLED_library
 *
 ****************************************************************************************
 */

#pragma once

#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "SSD1322_API.h"
#include "Fonts/BitmapFont.h"

class SSD1322_GFX
{
private:
  SSD1322_API *api_instance;
  int OLED_HEIGHT;
  int OLED_WIDTH;
  uint16_t _buffer_height;
  uint16_t _buffer_width;
  const GFXfont* currentFont = nullptr;

public:
  SSD1322_GFX(SSD1322_API *api, int OLED_HEIGHT_SIZE, int OLED_WIDTH_SIZE);
  void set_buffer_size(uint16_t buffer_width, uint16_t buffer_height);
  void fill_buffer(uint8_t *frame_buffer, uint8_t brightness);
  void draw_pixel(uint8_t *frame_buffer, uint16_t x, uint16_t y, uint8_t brightness);
  void draw_vline(uint8_t *frame_buffer, uint16_t x, uint16_t y0, uint16_t y1, uint8_t brightness);
  void draw_hline(uint8_t *frame_buffer, uint16_t x, uint16_t y, uint16_t x1, uint8_t brightness);
  void draw_line(uint8_t *frame_buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t brightness);
  void draw_AA_line(uint8_t *frame_buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t brightness);
  void draw_rect(uint8_t *frame_buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t x2, uint8_t brightness);
  void draw_rect_filled(uint8_t *frame_buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t x2, uint8_t brightness);
  void draw_circle(uint8_t *frame_buffer, uint16_t x0, uint16_t y0, uint16_t r, uint8_t brightness);
  void draw_bitmap_8bpp(uint8_t *frame_buffer, const uint8_t *bitmap, uint16_t x0, uint16_t y0, uint16_t x_size, uint16_t y_size);
  void draw_bitmap_4bpp(uint8_t *frame_buffer, const uint8_t *bitmap, uint16_t x0, uint16_t y0, uint16_t x_size, uint16_t y_size);

  void select_font(const GFXfont* font);
  void draw_char(uint8_t *frame_buffer, uint8_t text, uint16_t x, uint16_t y, uint8_t brightness);
  void draw_text(uint8_t *frame_buffer, const char *text, uint16_t x, uint16_t y, uint8_t brightness);

  void send_buffer_to_OLED(uint8_t *frame_buffer, uint16_t start_x, uint16_t start_y);
};
