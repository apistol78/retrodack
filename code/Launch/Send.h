#pragma once

#include <Core/Io/IStream.h>

bool sendLine(traktor::IStream* target, uint32_t base, const uint8_t* line, uint32_t length);

bool sendJump(traktor::IStream* target, uint32_t start, uint32_t sp);
