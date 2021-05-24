#pragma once

#include <cstdint>
#include <utility>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics.hpp"
#include "error.hpp"

void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color);
Error WriteUnicode(PixelWriter& writer, Vector2D<int> pos, char32_t c, const PixelColor& color);
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color);

int CountUTF8Size(uint8_t c);
std::pair<char32_t, int> ConvertUTF8To32(const char* u8);
bool IsHankaku(char32_t c);

void InitializeFont();

WithError<FT_Face> NewFTFace();
Error RenderUnicode(char32_t c, FT_Face face);

