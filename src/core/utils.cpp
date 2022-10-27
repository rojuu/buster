#include "utils.h"

#include <cstdio>

void logger_init()
{
	spdlog::set_level((spdlog::level::level_enum)SPDLOG_ACTIVE_LEVEL);
}

Vector<u8> read_entire_file_as_bytes(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	auto file_guard = ScopeGuard([file] { fclose(file); });
	fseek(file, 0, SEEK_END);
	usz size = ftell(file);
	Vector<u8> data(size);
	fseek(file, 0, SEEK_SET);
	fread(data.data(), 1, size, file);
	return data;
}
