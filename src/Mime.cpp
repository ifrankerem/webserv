#include "../includes/Mime.hpp"
#include "../includes/Utils.hpp"
#include <cstddef>

struct MimeEntry {
	const char* ext;
	const char* type;
};

static const MimeEntry mimeTable[] = {
	{ ".html", "text/html; charset=UTF-8" },
	{ ".htm",  "text/html; charset=UTF-8" },
	{ ".css",  "text/css" },
	{ ".js",   "application/javascript" },
	{ ".json", "application/json" },
	{ ".txt",  "text/plain; charset=UTF-8" },
	{ ".png",  "image/png" },
	{ ".jpg",  "image/jpeg" },
	{ ".jpeg", "image/jpeg" },
	{ ".gif",  "image/gif" },
	{ ".svg",  "image/svg+xml" },
	{ ".ico",  "image/x-icon" },
	{ ".pdf",  "application/pdf" }
};

std::string mimeType(const std::string& path)
{
	std::string ext = utils::toLower(utils::getExtension(path));

	size_t count = sizeof(mimeTable) / sizeof(mimeTable[0]);
	for (size_t i = 0; i < count; i++)
	{
		if (ext == mimeTable[i].ext)
			return mimeTable[i].type;
	}
	return "application/octet-stream";
}
