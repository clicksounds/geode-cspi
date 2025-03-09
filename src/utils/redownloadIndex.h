#pragma once
#include <Geode/Geode.hpp>

void redownloadIndex();

struct downloadedzipStruc {
	bool Finished = false;
	bool Failed = false;
	bool StartedDownloading = false;
};
static downloadedzipStruc indexzip;