#include "utils.h"

#include <cstdio>

namespace bstr::core {

void logger_init()
{
	spdlog::set_level((spdlog::level::level_enum)SPDLOG_ACTIVE_LEVEL);
}

vector<u8> read_entire_file_as_bytes(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	vector<u8> data;
	if (file) {
		defer{ fclose(file); };
		fseek(file, 0, SEEK_END);
		usz size = ftell(file);
		data.resize(size);
		fseek(file, 0, SEEK_SET);
		fread(data.data(), 1, size, file);
	}
	return data;
}

}
