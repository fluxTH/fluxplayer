#pragma once

#include <QObject>
#include <QFileInfo>
#include <QMap>

#include "bass.h"
#include "fxtrack.h"

class FluxPlayer;


class FXPlayer : public QObject
{
	Q_OBJECT

public:
	FXPlayer(QObject *parent, const char &name);
	~FXPlayer();

	void attachToMixer(HSTREAM *mixerHandle);

	void eject();
	bool loadTrack(FXTrack *track);
	bool play();
	bool pause();
	bool stop();
	bool seek(const double &ratio);
	bool seek(QWORD byte_pos);

	bool isFree();

	bool isHandleValid(const bool &overrideLocal = false);
	bool isUsingMixer();
	bool isPlaying();

	float getAudioAvgBitrate();
	float getAudioSampleRate();
	QString getAudioType();
	QString getAudioDriver();
	BASS_CHANNELINFO getAudioStreamInfo();

	FXTrack* getTrack();

	double getTimePosition();
	double getTimeDuration();

	char* getName();
	void setName(const char &name);

private:
	char m_playerName;
	bool m_isLoaded;
	QVector<HSYNC> m_syncHandles;

	HSTREAM m_streamHandle;
	HSTREAM *m_mixerHandle;
	FXTrack *m_track;

	QFileInfo m_filePath;

	bool loadFile(const QString &filepath);
	void attachSyncEvent(HSYNC syncHandle);
	bool attachSyncEvents();
	void detachSyncEvents();

signals:
	void ejected(FXPlayer*);
	void fadeNext();

	void bassSyncEnd();
	void bassSyncFree();
	void bassSyncPosNext();

	void trackPlayed(FXPlayer*);
	void trackStopped(FXPlayer*);

public slots:
	void handleEject();
	void handleBassFree();
};
