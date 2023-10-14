#pragma once

#include "def.h"
#include "non_null.h"
#include "containers/vector.h"

namespace bstr::core {

class AseFile {
public:
	static AseFile load_from_file(const char* filename);

private:
	friend class AsepriteLoader;

	AseFile() = default;
};

}