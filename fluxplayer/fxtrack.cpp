#include "fxtrack.h"

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/mp4/mp4file.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/frames/attachedpictureframe.h>

#include <QDebug>


FXTrack::FXTrack(QObject *parent, QString filePath)
	: QObject(parent),
	m_file(QFileInfo(filePath)),
	m_playerErrorCode(0),
	m_played(false)
{
	this->m_player = nullptr;
	this->m_coverPixmap = nullptr;

	// TODO: Test if file exists, etc

	// Read Tag
	this->readTags();
}

FXTrack::~FXTrack()
{
	if (this->m_coverPixmap != nullptr)
		this->deleteCoverPixmap();
}

QFileInfo* FXTrack::getFileInfo()
{
	return &this->m_file;
}

void FXTrack::readTags()
{
	TagLib::FileRef fileRef((wchar_t*)this->getFileInfo()->filePath().utf16());

	if (!fileRef.isNull()) {
		if (fileRef.tag()) {
			TagLib::Tag *tag = fileRef.tag();

			this->setMetadata("Title", TStringToQString(tag->title()));
			this->setMetadata("Artist", TStringToQString(tag->artist()));
		}

		if (fileRef.audioProperties()) {
			TagLib::AudioProperties *audioProps = fileRef.audioProperties();

			this->setMetadata("Bitrate", audioProps->bitrate());
			this->setMetadata("SampleRate", audioProps->sampleRate());
			this->setMetadata("Channels", audioProps->channels());
			this->setMetadata("Duration", audioProps->length());
		}
	}
	else {
		// TODO: Handle stream may not be playable
		qDebug() << "Cannot load metadata!" << this->m_metadata;
	}

	emit textUpdated(this);
}

void FXTrack::readCoverPicture()
{
#ifndef _DEBUG
	if (this->m_coverPixmap == nullptr) {
		QString extension = this->getFileInfo()->suffix();
		wchar_t *fileName = (wchar_t*)this->getFileInfo()->filePath().utf16();

		if (extension == "flac") {
			TagLib::FLAC::File tagFile(fileName);

			if (tagFile.pictureList().size() > 0) {
				TagLib::ByteVector coverData = tagFile.pictureList().front()->data();
				this->m_coverPixmap = new QPixmap;
				this->m_coverPixmap->loadFromData(reinterpret_cast<uchar*>(coverData.data()), coverData.size());
			}
		}
		else if (extension == "m4a" || extension == "mp4") {
			TagLib::MP4::File tagFile(fileName);

			TagLib::MP4::CoverArtList coverList = tagFile.tag()->item("covr").toCoverArtList();
			if (coverList.size() > 0) {
				TagLib::ByteVector coverData = coverList.front().data();
				this->m_coverPixmap = new QPixmap;
				this->m_coverPixmap->loadFromData(reinterpret_cast<uchar*>(coverData.data()), coverData.size());
			}
		}
		else if (extension == "mp3" || extension == "mp2" || extension == "mp1") {
			TagLib::MPEG::File tagFile(fileName);

			if (tagFile.hasID3v2Tag()) {
				TagLib::ID3v2::FrameList frames = tagFile.ID3v2Tag()->frameList("APIC");

				if (!frames.isEmpty()) {
					TagLib::ByteVector coverData = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front())->picture();
					this->m_coverPixmap = new QPixmap;
					this->m_coverPixmap->loadFromData(reinterpret_cast<uchar*>(coverData.data()), coverData.size());
				}
			}
		}
	}
#endif
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

double FXTrack::getRuntime()
{
	if (this->hasCuePoints()) {
		// Be sure to update to support other cuepoints
		// Trim in might be -1
		return this->getCuePoint("PlayNext") - this->getCuePoint("TrimIn");
	}

	return this->getMetadata("Duration", 0).toInt();
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
	this->deleteCoverPixmap();
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

bool FXTrack::hasPlayMetadata()
{
	// Modify later to reflect configuration
	return this->hasCuePoints() && this->hasNormalization();
}

bool FXTrack::hasCuePoints()
{
	// temp
	return this->getCuePoint("PlayNext") != -1.0;
}

bool FXTrack::hasNormalization()
{
	return !isnan(this->getMetadata("Loudness", NAN).toDouble());
}

double FXTrack::getNormalizationGain()
{
	double loudness = this->getMetadata("Loudness", NAN).toDouble();
	if (isnan(loudness)) {
		return 0.0;
	}

	// temp
	return (-14) - loudness;
}

QPixmap* FXTrack::getCoverPixmap()
{
	return this->m_coverPixmap;
}

void FXTrack::deleteCoverPixmap()
{
	if (this->m_coverPixmap != nullptr) {
		qDebug() << "Pixmap delete";
		delete this->m_coverPixmap;
		this->m_coverPixmap = nullptr;
	}
}

void FXTrack::setCuePoint(const QString &name, const double &pos)
{
	this->m_cuePoints[name] = pos;
}

double FXTrack::getCuePoint(const QString &name)
{
	return this->m_cuePoints.value(name, -1.0);
}

