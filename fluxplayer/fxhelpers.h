#pragma once

#include <QString>
#include <QMap>

#undef h_filterPositive
#define h_filterPositive(x) ((x<0)?(0):(x))

#undef h_max
#define h_max(x, y) ((x>y)?(y):(x))

#undef h_min
#define h_min(x, y) ((x<y)?(y):(x))

#undef h_float2db
#define h_float2db(f) ((f > 0) ? (20 * log10(f)) : (-1000))

class FXHelpers
{

public:
	static QString formatSecToTime(const float &total_sec, const bool &show_decimals = false);

};

enum class FXAudioDriver : uint8_t {
	Invalid = 0,
	WASAPI = 1,
	DSOUND = 2,
	ASIO = 3
};

const QStringList AUDIO_EXTS = {
	"aac", "aif", "aiff", "alac", "ape", "au", "flac", "m4a", "m4b", "mp1", "mp2", "mp3",
	"mp4", "ogg", "oga", "mogg", "opus", "ts", "wav", "wma", "wv", "webm", "cda"
};

const QString BassErrorString[] = {
	/*  0 */ "BASS_OK",
	/*  1 */ "BASS_ERROR_MEM",
	/*  2 */ "BASS_ERROR_FILEOPEN",
	/*  3 */ "BASS_ERROR_DRIVER",
	/*  4 */ "BASS_ERROR_BUFLOST",
	/*  5 */ "BASS_ERROR_HANDLE",
	/*  6 */ "BASS_ERROR_FORMAT",
	/*  7 */ "BASS_ERROR_POSITION",
	/*  8 */ "BASS_ERROR_INIT",
	/*  9 */ "BASS_ERROR_START",
	/* 10 */ "BASS_ERROR_SSL",
	/* 11 */ "",
	/* 12 */ "",
	/* 13 */ "",
	/* 14 */ "BASS_ERROR_ALREADY",
	/* 15 */ "",
	/* 16 */ "",
	/* 17 */ "BASS_ERROR_NOTAUDIO",
	/* 18 */ "BASS_ERROR_NOCHAN",
	/* 19 */ "BASS_ERROR_ILLTYPE",
	/* 20 */ "BASS_ERROR_ILLPARAM",
	/* 21 */ "BASS_ERROR_NO3D",
	/* 22 */ "BASS_ERROR_NOEAX",
	/* 23 */ "BASS_ERROR_DEVICE",
	/* 24 */ "BASS_ERROR_NOPLAY",
	/* 25 */ "BASS_ERROR_FREQ",
	/* 26 */ "",
	/* 27 */ "BASS_ERROR_NOTFILE",
	/* 28 */ "",
	/* 29 */ "BASS_ERROR_NOHW",
	/* 30 */ "",
	/* 31 */ "BASS_ERROR_EMPTY",
	/* 32 */ "BASS_ERROR_NONET",
	/* 33 */ "BASS_ERROR_CREATE",
	/* 34 */ "BASS_ERROR_NOFX",
	/* 35 */ "",
	/* 36 */ "",
	/* 37 */ "BASS_ERROR_NOTAVAIL",
	/* 38 */ "BASS_ERROR_DECODE",
	/* 39 */ "BASS_ERROR_DX",
	/* 40 */ "BASS_ERROR_TIMEOUT",
	/* 41 */ "BASS_ERROR_FILEFORM",
	/* 42 */ "BASS_ERROR_SPEAKER",
	/* 43 */ "BASS_ERROR_VERSION",
	/* 44 */ "BASS_ERROR_CODEC",
	/* 45 */ "BASS_ERROR_ENDED",
	/* 46 */ "BASS_ERROR_BUSY",
	/* 47 */ "BASS_ERROR_UNSTREAMABLE"
};