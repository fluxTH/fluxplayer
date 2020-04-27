#pragma once

#include <QtWidgets/QMainWindow>

#include "ui_fluxplayer.h"

#include "fxplayer.h"
#include "fxplaylistmodel.h"
#include "fxhelpers.h"


class FluxPlayer : public QMainWindow
{
	Q_OBJECT

public:
	FluxPlayer(QWidget *parent = Q_NULLPTR);
	~FluxPlayer();

	bool initialize();
	FXAudioDriver* getAudioDriver();

private:
	const QStringList BASS_PLUGINS = { 
		"bassflac", "bassopus", "bass_aac", "bass_ape", "bassalac", "basswebm", "basswma" 
	};

	const QStringList AUDIO_EXTS = {
		"aac", "aif", "aiff", "alac", "ape", "au", "flac", "m4a", "m4b", "mp1", "mp2", "mp3",
		"mp4", "ogg", "oga", "mogg", "opus", "ts", "wav", "wma", "wv", "webm", "cda"
	};
	const char* PLAYER_NAME_MAP = "ABCDEF";
	const short int PLAYER_COUNT = 3;

	Ui::FluxPlayerClass ui;

	FXPlaylistModel* m_playlistModel;

	QList<FXPlayer*>* m_players;
	QVector<HPLUGIN> m_bassPlugins;

	HSTREAM m_mixerHandle;

	FXAudioDriver m_audioDriver;
	int m_audioSampleRate;
	int m_audioDeviceIndex;
	int m_audioChannelLayout;

	bool initBass();
	void initPlayers();
	void initPlaylist();

	short int createPlayer(const char &name);

	bool deletePlayer(short int &index);
	bool deletePlayer(FXPlayer *player);

public slots:
	void handleAttachPlayerControl(FXPlayer*, bool);
	void handleEjectPlayer(FXPlayer*);
	void handlePlaylistAddFiles();
	void handlePlaylistAddFolder();
	void handleTogglePlaylistNumbering();

	void handleActionConfiguration();

};
