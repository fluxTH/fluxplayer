#include "fxtrack.h"

#include <QDebug>

FXTrack::FXTrack(QObject *parent, QString filePath)
	: QObject(parent),
	m_file(QFileInfo(filePath)),
	m_player(nullptr),
	m_playerErrorCode(0),
	m_played(false)
{
	// TODO: Test if file exists, etc

	// Read Tag
	this->readTags();
}

FXTrack::~FXTrack()
{

}

QFileInfo* FXTrack::getFileInfo()
{
	return &this->m_file;
}

void FXTrack::readTags()
{
	this->m_tagFileRef = TagLib::FileRef((wchar_t*)this->getFileInfo()->filePath().utf16());

	if (!this->m_tagFileRef.isNull() && this->m_tagFileRef.tag()) {
		TagLib::Tag *tag = this->m_tagFileRef.tag();

		this->setMetadata("Title", TStringToQString(tag->title()));
		this->setMetadata("Artist", TStringToQString(tag->artist()));
	}

	if (!this->m_tagFileRef.isNull() && this->m_tagFileRef.audioProperties()) {
		TagLib::AudioProperties *properties = this->m_tagFileRef.audioProperties();

		this->setMetadata("Bitrate", properties->bitrate());
		this->setMetadata("SampleRate", properties->sampleRate());
		this->setMetadata("Channels", properties->channels());
		this->setMetadata("Duration", properties->length());
	}
	else {
		// TODO: Handle stream may not be playable
		qDebug() << this->m_metadata;
	}

	emit textUpdated(this);
}

QVariant FXTrack::getMetadata(const QString &key, const QVariant &default)
{
	return this->m_metadata.value(key, default);
}

QString FXTrack::getTitle()
{
	QString title = this->getMetadata("Title", "").toString();
	if (!title.isEmpty())
		return title;

	return this->m_file.fileName();
}

QString FXTrack::getArtist(const QString &unknownText)
{
	QString artist = this->getMetadata("Artist", "").toString();
	if (!artist.isEmpty())
		return artist;

	return unknownText;
}

void FXTrack::setMetadata(const QString &key, const QVariant &value)
{
	this->m_metadata[key] = value;
}

bool FXTrack::isLoadedToPlayer()
{
	if (this->m_player == nullptr)
		return false;

	return true;
}

void FXTrack::setPlayer(FXPlayer *player)
{
	this->m_player = player;
}

FXPlayer* FXTrack::getPlayer()
{
	return this->m_player;
}

void FXTrack::removePlayer()
{
	this->m_player = nullptr;
}

void FXTrack::setPlayerErrorCode(const int &code)
{
	this->m_playerErrorCode = code;
}

int& FXTrack::getPlayerErrorCode()
{
	return this->m_playerErrorCode;
}

bool FXTrack::hasPlayerError()
{
	return this->getPlayerErrorCode() != 0;
}

void FXTrack::setPlayed(const bool &played)
{
	this->m_played = played;
}

bool FXTrack::hasPlayed()
{
	return this->m_played;
}

