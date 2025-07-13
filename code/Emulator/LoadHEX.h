#pragma once

#include <string>

class Bus;
class ICPU;

bool loadHEX(const std::wstring& fileName, ICPU& cpu, Bus& bus);
