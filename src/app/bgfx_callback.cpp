#include "bgfx_callback.h"
#include <core/utils.h>

void BgfxCallback::fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str)
{
	const char* codeName = "";
	switch (code) {
	case bgfx::Fatal::DebugCheck:
		codeName = "DebugCheck";
		break;
	case bgfx::Fatal::InvalidShader:
		codeName = "InvalidShader";
		break;
	case bgfx::Fatal::UnableToInitialize:
		codeName = "UnableToInitialize";
		break;
	case bgfx::Fatal::UnableToCreateTexture:
		codeName = "UnableToCreateTexture";
		break;
	case bgfx::Fatal::DeviceLost:
		codeName = "DeviceLost";
		break;

	case bgfx::Fatal::Count:
		codeName = "Count";
		break;
	}
	LOG_CRITICAL("BGFX_ERROR {}:{}: {}: {}", filePath, line, codeName, str);
}

void BgfxCallback::traceVargs(const char* filePath, uint16_t line, const char* format, va_list argList)
{
	char buf[1024];
	vsprintf(buf, format, argList);
	LOG_TRACE("BGFX: {}:{}: {}", filePath, line, buf);
}

void BgfxCallback::profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line)
{
	//TODO:
	LOG_DEBUG("profile begin: {}, {}, {}, {}", name, abgr, filePath, line);
}

void BgfxCallback::profilerBeginLiteral(const char* name, uint32_t abgr, const char* filePath, uint16_t line)
{
	//TODO:
	LOG_DEBUG("profile begin literal: {}, {}, {}, {}", name, abgr, filePath, line);
}

void BgfxCallback::profilerEnd()
{
	//TODO:
	LOG_DEBUG("profile end");
}

uint32_t BgfxCallback::cacheReadSize(uint64_t id)
{
	return 0;
}

bool BgfxCallback::cacheRead(uint64_t id, void* data, uint32_t size)
{
	return false;
}

void BgfxCallback::cacheWrite(uint64_t id, const void* data, uint32_t size)
{
}

void BgfxCallback::screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data, uint32_t size, bool yflip)
{
}

void BgfxCallback::captureBegin(uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum format, bool yflip)
{
}

void BgfxCallback::captureEnd()
{
}

void BgfxCallback::captureFrame(const void* data, uint32_t size)
{
}
