#pragma once

#include <QString>

class FXHelpers
{

public:
	static QString formatSecToTime(const int &total_sec);

};

enum FXAudioDriver {
	Invalid = 0,
	WASAPI = 1,
	DSOUND = 2,
	ASIO = 3
};