#pragma once

#include <Core/Object.h>
#include <Core/Ref.h>
#include <Core/Containers/AlignedVector.h>
#include <Core/Io/Path.h>

class FileSystemImage : public traktor::Object
{
    T_RTTI_CLASS;

public:
    static traktor::Ref< FileSystemImage > createFromDirectory(const traktor::Path& path);

    uint8_t* ptr() { return m_data.ptr(); }

    uint32_t size() const { return (uint32_t)m_data.size(); }

private:
    traktor::AlignedVector< uint8_t > m_data;
};
