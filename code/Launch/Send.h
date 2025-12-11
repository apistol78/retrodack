#pragma once

#include <Core/Io/IStream.h>

bool sendWrite(traktor::IStream* target, uint32_t base, const uint8_t* line, uint32_t length);

bool sendJump(traktor::IStream* target, uint32_t start, uint32_t sp);

bool sendCreateFile(traktor::IStream* target, const char* fileName);

bool sendWriteFile(traktor::IStream* target, const uint8_t* line, uint32_t length);

bool sendCloseFile(traktor::IStream* target);
