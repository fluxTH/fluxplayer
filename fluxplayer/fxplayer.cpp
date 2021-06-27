#include "FXPlayer.h"

#include <QDebug>
#include "fluxplayer.h"

#include "bassmix.h"

#include "bassflac.h"
#include "bass_aac.h"
#include "bass_ape.h"
#include "bassalac.h"
#include "bassopus.h"
#include "basswma.h"

// TODO: Attach to native bass events, including unloading to m_isLoaded

FXPlayer::FXPlayer(QObject* parent, const char& name)
	: QObject(parent),
	m_playerName(name),
	m_streamHandle(NULL),
	m_mixerHandle(nullptr),
	m_playNextSyncHandle(0),
	m_track(nullptr),
	m_isLoaded(false),
	m_hasPlayed(false)
{
	qDebug() << "Player" << name << "created!";

	QObject::connect(this, SIGNAL(bassSyncEnd()), this, SLOT(handleEject()));
	QObject::connect(this, SIGNAL(bassSyncFree()), this, SLOT(handleBassFree()));
}

FXPlayer::~FXPlayer()
{
	this->eject();
}

void FXPlayer::attachToMixer(HSTREAM* mixerHandle)
{
	this->m_mixerHandle = mixerHandle;
}

bool FXPlayer::loadFile(const QString& filepath)
{
	this->m_filePath = QFileInfo(filepath);

	DWORD fileFlags = BASS_UNICODE | BASS_ASYNCFILE | BASS_STREAM_PRESCAN | BASS_SAMPLE_FLOAT;
	if (this->isUsingMixer())
		fileFlags |= BASS_STREAM_DECODE;

	this->m_streamHandle = BASS_StreamCreateFile(FALSE, (wchar_t*)filepath.utf16(), 0, 0, fileFlags);
	if (this->isHandleValid(true)) {
		qDebug() << "Stream OK";

		// TEMP
		this->setGain(-6);
		//BASS_ChannelSetAttribute(this->m_streamHandle, BASS_ATTRIB_SRC, 4);

		if (this->isUsingMixer()) {
			DWORD mixFlags = BASS_MIXER_NORAMPIN | BASS_MIXER_CHAN_DOWNMIX | BASS_SAMPLE_FLOAT | BASS_MIXER_CHAN_PAUSE;
			if (BASS_Mixer_StreamAddChannelEx(*this->m_mixerHandle, this->m_streamHandle, mixFlags, 0, 0) == 0) { //BASS_STREAM_AUTOFREE
				// Throw a msgbox error?
				qDebug() << "Mix attach failed" << BASS_ErrorGetCode();
			}

			this->seek(0.0);
		}

		this->m_isLoaded = true;

		this->attachSyncEvents();

		return true;
	}

	this->m_streamHandle = NULL;
	return false;
}

bool FXPlayer::loadTrack(FXTrack* track)
{
	if (!this->loadFile(track->getFileInfo()->filePath()))
		return false;

	this->m_track = track;
	track->setPlayer(this);

	track->setMetadata("Duration", this->getTimeDuration());

	this->reload();

	return true;
}

void FXPlayer::eject()
{
	if (this->isHandleValid()) {
		this->stop();

		this->detachSyncEvents();
		BASS_StreamFree(this->m_streamHandle);
	}

	this->m_streamHandle = NULL;
	this->m_isLoaded = false;
	this->m_hasPlayed = false;

	if (this->m_track != nullptr)
		this->m_track->removePlayer();
	this->m_track = nullptr;

	qDebug() << "Player" << this->m_playerName << "ejected!";
	emit ejected(this);
}

bool FXPlayer::play()
{
	bool result;
	if (this->isHandleValid()) {
		if (this->isUsingMixer()) {
			result = !(BASS_Mixer_ChannelFlags(this->m_streamHandle, 0, BASS_MIXER_CHAN_PAUSE) & BASS_MIXER_CHAN_PAUSE);
		}
		else {
			result = BASS_ChannelPlay(this->m_streamHandle, FALSE);
		}

		//if (result)
			//this->m_track->setPlayed(true);

		emit trackPlayed(this);
	}

	if (result && !this->m_hasPlayed)
		this->m_hasPlayed = true;

	return result;
}

bool FXPlayer::pause()
{
	bool result = false;
	if (this->isHandleValid()) {
		if (this->isUsingMixer()) {
			result = BASS_Mixer_ChannelFlags(this->m_streamHandle, BASS_MIXER_CHAN_PAUSE, BASS_MIXER_CHAN_PAUSE);
		}
		else {
			result = BASS_ChannelPause(this->m_streamHandle);
		}
		emit trackStopped(this);
	}
	return result;
}

bool FXPlayer::stop()
{
	if (this->isHandleValid()) {
		// Log played event
		if (!this->m_track->hasPlayed()) {
			QWORD pos = BASS_ChannelGetPosition(this->m_streamHandle, BASS_POS_BYTE);
			QWORD len = BASS_ChannelGetLength(this->m_streamHandle, BASS_POS_BYTE);
			double playRatio = static_cast<double>(pos) / static_cast<double>(len);

			if (playRatio > 0.1)
				this->m_track->setPlayed(true);
		}

		return this->seek(0.00) && this->pause();
	}

	return false;
}

bool FXPlayer::seekTime(const double& sec)
{
	return this->seek(sec / this->getTimeDuration());
}

bool FXPlayer::seek(const double& ratio)
{
	QWORD pos = ratio * BASS_ChannelGetLength(this->m_streamHandle, BASS_POS_BYTE);
	return this->seek(pos);
}

bool FXPlayer::seek(QWORD byte_pos)
{
	if (this->isHandleValid()) {
		return BASS_ChannelSetPosition(this->m_streamHandle, byte_pos, BASS_POS_BYTE);
	}

	return false;
}

void FXPlayer::reload()
{
	if (this->isHandleValid()) {
		this->attachPlayNextSync();

		if (!this->isPlaying() && !this->m_hasPlayed) {
			double trimIn = this->m_track->getCuePoint("TrimIn");
			if (trimIn != -1.0) {
				this->seekTime(trimIn);
				qDebug() << "seek to trim in" << trimIn;
			}
		}

		this->setGain(this->m_track->getNormalizationGain());
	}
}

bool FXPlayer::isFree()
{
	return !this->isHandleValid();
}

bool FXPlayer::isLoaded()
{
	return this->isHandleValid();
}

bool FXPlayer::isHandleValid(const bool& overrideLocal)
{
	if (this->m_isLoaded || overrideLocal)
		if (this->m_streamHandle != 0)
			return true;

	return false;
}

bool FXPlayer::isUsingMixer()
{
	return this->m_mixerHandle != nullptr;
}

bool FXPlayer::isPlaying()
{
	if (this->isHandleValid()) {
		if (this->isUsingMixer()) {
			bool playing = !(BASS_Mixer_ChannelFlags(this->m_streamHandle, 0, 0) & BASS_MIXER_CHAN_PAUSE);
			//qDebug() << "playing:" << playing;
			return playing;
		}
		else {
			DWORD status = BASS_ChannelIsActive(this->m_streamHandle);
			//if (status == BASS_ACTIVE_PAUSED)
			//	return false;
			if (status == BASS_ACTIVE_PLAYING)
				return true;
		}
	}

	return false;
}

float FXPlayer::getAudioAvgBitrate()
{
	if (this->isHandleValid()) {
		float bitrate;
		if (BASS_ChannelGetAttribute(this->m_streamHandle, BASS_ATTRIB_BITRATE, &bitrate))
			return bitrate;
	}
	return -1.0f;
}

float FXPlayer::getAudioSampleRate()
{
	if (this->isHandleValid()) {
		float samplerate;
		if (BASS_ChannelGetAttribute(this->m_streamHandle, BASS_ATTRIB_FREQ, &samplerate))
			return samplerate;
	}
	return -1.0f;
}

QString FXPlayer::getAudioType()
{
	BASS_CHANNELINFO info = this->getAudioStreamInfo();
	switch (info.ctype) {
	case BASS_CTYPE_STREAM_VORBIS:
		return "Vorbis";
	case BASS_CTYPE_STREAM_MP1:
		return "MP1";
	case BASS_CTYPE_STREAM_MP2:
		return "MP2";
	case BASS_CTYPE_STREAM_MP3:
	case BASS_CTYPE_STREAM_WMA_MP3:
		return "MP3";
	case BASS_CTYPE_STREAM_AIFF:
		return "AIFF";
	case BASS_CTYPE_STREAM_AM:
		return "AndroidMedia";
	case BASS_CTYPE_STREAM_CA:
		return "CoreAudio";
	case BASS_CTYPE_STREAM_MF:
		return "MediaFoundation";
	case BASS_CTYPE_STREAM_WAV_PCM:
		return "PCM";
	case BASS_CTYPE_STREAM_WAV_FLOAT:
		return "PCM/F";
	case BASS_CTYPE_STREAM_FLAC:
	case BASS_CTYPE_STREAM_FLAC_OGG:
		return "FLAC";
	case BASS_CTYPE_STREAM_AAC:
	case BASS_CTYPE_STREAM_MP4:
		return "AAC";
	case BASS_CTYPE_STREAM_APE:
		return "APE";
	case BASS_CTYPE_STREAM_ALAC:
		return "ALAC";
	case BASS_CTYPE_STREAM_OPUS:
		return "OPUS";
	case BASS_CTYPE_STREAM_WMA:
		return "WMA";
	default:
		return "Unknown";
	}
}

QString FXPlayer::getAudioDriver()
{
	FXAudioDriver* driver = dynamic_cast<FluxPlayer*>(this->parent())->getAudioDriver();
	switch (*driver) {
	case FXAudioDriver::WASAPI:
		return "WASAPI";
	case FXAudioDriver::DSOUND:
		return "DSOUND";
	case FXAudioDriver::ASIO:
		return "ASIO";
	default:
		return "";
	}
}

BASS_CHANNELINFO FXPlayer::getAudioStreamInfo()
{
	// TODO: Add more checks eg stream handle
	BASS_CHANNELINFO info;
	BASS_ChannelGetInfo(this->m_streamHandle, &info);
	return info;
}

FXTrack* FXPlayer::getTrack()
{
	return this->m_track;
}

double FXPlayer::getTimePosition()
{
	if (this->isHandleValid()) {
		QWORD pos = BASS_ChannelGetPosition(this->m_streamHandle, BASS_POS_BYTE);
		return BASS_ChannelBytes2Seconds(this->m_streamHandle, pos);
	}

	return -1;
}

double FXPlayer::getTimeDuration()
{
	if (this->isHandleValid()) {
		QWORD length = BASS_ChannelGetLength(this->m_streamHandle, BASS_POS_BYTE);
		return BASS_ChannelBytes2Seconds(this->m_streamHandle, length);
	}

	return -1;
}

double FXPlayer::getDisplayTimeDuration()
{
	if (this->isHandleValid()) {
		double nextCue = this->m_track->getCuePoint("PlayNext");
		return nextCue == -1.0 ? this->getTimeDuration() : nextCue;
	}
	return this->getTimeDuration();
}

void FXPlayer::setName(const char& name)
{
	this->m_playerName = name;
}

void FXPlayer::setGain(const double& gain)
{
	if (this->isHandleValid()) {
		qDebug() << "Gain set to" << gain << "dB";
		float gain_float = (gain == 0) ? 1.0 : pow(10, gain / 20);
		BASS_ChannelSetAttribute(this->m_streamHandle, BASS_ATTRIB_VOL, gain_float);
	}
}

char* FXPlayer::getName()
{
	return &this->m_playerName;
}

void CALLBACK PlayerSyncFreeCallback(HSYNC, DWORD, DWORD, void* user)
{
	FXPlayer* player = reinterpret_cast<FXPlayer*>(user);
	//emit player->bassSyncFree(); // Disable for now, sometimes get freed after player is destroyed
	qDebug() << "BASS ch free, player" << *player->getName();
}

void CALLBACK PlayerSyncEndCallback(HSYNC, DWORD, DWORD, void* user)
{
	FXPlayer* player = reinterpret_cast<FXPlayer*>(user);
	emit player->bassSyncEnd();
}

void CALLBACK PlayerSyncPosCallback(HSYNC, DWORD, DWORD, void* user)
{
	FXPlayer* player = reinterpret_cast<FXPlayer*>(user);
	emit player->bassSyncPosNext();
}

void FXPlayer::attachSyncEvent(HSYNC syncHandle)
{
	if (syncHandle != 0)
		this->m_syncHandles.append(syncHandle);
}

bool FXPlayer::attachSyncEvents()
{
	if (this->isHandleValid()) {
		// Events that needs to be detached later
		this->attachSyncEvent(BASS_ChannelSetSync(this->m_streamHandle, BASS_SYNC_FREE, NULL, &PlayerSyncFreeCallback, this));

		// One time events
		BASS_ChannelSetSync(this->m_streamHandle, BASS_SYNC_END | BASS_SYNC_ONETIME, NULL, &PlayerSyncEndCallback, this);

		return true;
	}

	return false;
}

void FXPlayer::detachSyncEvents()
{
	foreach(HSYNC syncHandle, this->m_syncHandles)
		BASS_ChannelRemoveSync(this->m_streamHandle, syncHandle);
}

void FXPlayer::attachPlayNextSync()
{
	double playNextPos = this->m_track->getCuePoint("PlayNext");

	if (playNextPos != -1.0) {
		if (this->m_playNextSyncHandle != 0) {
			BASS_ChannelRemoveSync(this->m_streamHandle, this->m_playNextSyncHandle);
		}

		QWORD pos = BASS_ChannelSeconds2Bytes(this->m_streamHandle, playNextPos);
		this->m_playNextSyncHandle = BASS_ChannelSetSync(this->m_streamHandle, BASS_SYNC_POS | BASS_SYNC_ONETIME, pos, &PlayerSyncPosCallback, this);

		qDebug() << "PLAY NEXT SYNC ATTACH PLAYER" << *this->getName();
	}
}

// Slots
void FXPlayer::handleEject() {
	this->eject();
}

void FXPlayer::handleBassFree() {
	this->eject();
}