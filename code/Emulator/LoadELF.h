#pragma once

#include <string>

class Bus;
class ICPU;

bool loadELF(const std::wstring& fileName, ICPU& cpu, Bus& bus);
