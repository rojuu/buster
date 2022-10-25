#include "utils.h"

void logger_init()
{
	spdlog::set_level((spdlog::level::level_enum)SPDLOG_ACTIVE_LEVEL);
}
