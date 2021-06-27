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
	FXPlayer(QObject* parent, const char& name);
	~FXPlayer();

	void attachToMixer(HSTREAM* mixerHandle);

	void eject();
	bool loadTrack(FXTrack* track);
	bool play();
	bool pause();
	bool stop();
	bool seekTime(const double& sec);
	bool seek(const double& ratio);
	bool seek(QWORD byte_pos);

	void reload();

	bool isFree();
	bool isLoaded();

	bool isHandleValid(const bool& overrideLocal = false);
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
	double getDisplayTimeDuration();

	char* getName();
	void setName(const char& name);

	void setGain(const double& gain);

private:
	char m_playerName;
	bool m_isLoaded;
	bool m_hasPlayed;
	QVector<HSYNC> m_syncHandles;
	HSYNC m_playNextSyncHandle;

	HSTREAM m_streamHandle;
	HSTREAM* m_mixerHandle;
	FXTrack* m_track;

	QFileInfo m_filePath;

	bool loadFile(const QString& filepath);
	void attachSyncEvent(HSYNC syncHandle);
	bool attachSyncEvents();
	void detachSyncEvents();

	void attachPlayNextSync();

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
