#include "fxmark.h"

#include <QDebug>

#include <QPointer>
#include <QDateTime>
#include <QFile>
#include <QtEndian>

FXMark::FXMark(const std::string& audioPath) :
	m_hashId(0),
	m_audioPath(audioPath),
	m_timeCreated(0),
	m_timeModified(0),
	m_duration(0),
	m_loudness(0),

	m_loadSuccess(false)
{
	// Check if audio file exists

	this->calculateHashId();

	// Temp: move to own method
	QString tmpDir = "_fpdata";

	QDir markDir = QDir::temp();
	if (!markDir.cd(tmpDir)) {
		markDir.mkdir(tmpDir);
		markDir.cd(tmpDir);
	}
	this->m_markDir = markDir;

	if (this->loadMarkFile()) {
		// TODO: Check if fxm file exists then load data
		qDebug() << "Load mark file success";
		this->m_loadSuccess = true;
	}
	else {
		// else create one
		qDebug() << "Load mark file failed";
	}
}

void FXMark::setMark(const std::string& key, const float& value) {
	this->m_marks[key] = value;
}

bool FXMark::calculateHashId()
{
	QFile audioFile(QString::fromStdString(this->m_audioPath));
	if (audioFile.open(QIODevice::ReadOnly)) {
		bool success = false;

		XXH64_state_t* xhState = XXH64_createState();
		if (xhState == NULL)
			return false;

		do {
			/* Initialize state with selected seed */
			if (XXH64_reset(xhState, this->HASH_SEED) == XXH_ERROR)
				break;

			/* Feed the state with input data, any size, any number of times */
			size_t const xhBufferSize = 1024;
			bool hashError = false;
			while (!audioFile.atEnd()) {
				QByteArray buf = audioFile.read(xhBufferSize);
				if (XXH64_update(xhState, buf.data(), buf.length()) == XXH_ERROR) {
					hashError = true;
					break;
				}
			}

			// Append Duration
			if (XXH64_update(xhState, (void*)&this->m_duration, sizeof(this->m_duration)) == XXH_ERROR) {
				hashError = true;
				break;
			}

			if (hashError) break;

			/* Get the hash */
			XXH64_hash_t const xh = XXH64_digest(xhState);

			this->m_hashId = xh;
			success = true;
		} while (0);

		XXH64_freeState(xhState);
		return success;
	}

	return false;
}

void FXMark::updateTimestamps()
{
	if (this->m_timeCreated == 0)
		this->m_timeCreated = QDateTime::currentSecsSinceEpoch();

	if (this->m_timeModified == 0)
		this->m_timeModified = QDateTime::currentSecsSinceEpoch();
}

QString FXMark::getMarkFileName() const
{
	return QString("%1.fxm").arg(
		QString::number(this->m_hashId, 16)
	);
}

QString FXMark::getMarkFilePath() const
{
	return this->m_markDir.filePath(this->getMarkFileName());
}

bool FXMark::analyzerShouldRun()
{
	return !this->m_loadSuccess;
}

void FXMark::setInMark(const float& position)
{
	this->setMark("i", position);
}

void FXMark::setFadeInMark(const float& position)
{
	this->setMark("f", position);
}

void FXMark::setNextMark(const float& position)
{
	this->setMark("n", position);
}

void FXMark::setFadeOutMark(const float& position)
{
	this->setMark("F", position);
}

void FXMark::setOutMark(const float& position)
{
	this->setMark("o", position);
}

void FXMark::setDuration(const float& duration)
{
	this->m_duration = duration;
}

void FXMark::setLoudness(const float& loudness) {
	this->m_loudness = loudness;
}

void FXMark::setHistogram(const QList<quint8>& histogram, const quint16& resolution)
{
	this->m_histogram = histogram;
	this->m_histogramResolution = resolution;
}

void FXMark::updateTrack(FXTrack* track)
{
	QMutexLocker lock(&track->mutex);

	if (this->m_marks.find("i") != this->m_marks.end())
		track->setCuePoint("TrimIn", this->m_marks["i"]);

	if (this->m_marks.find("f") != this->m_marks.end())
		track->setCuePoint("FadeIn", this->m_marks["f"]);

	if (this->m_marks.find("n") != this->m_marks.end())
		track->setCuePoint("PlayNext", this->m_marks["n"]);

	if (this->m_marks.find("F") != this->m_marks.end())
		track->setCuePoint("FadeOut", this->m_marks["F"]);

	if (this->m_marks.find("o") != this->m_marks.end())
		track->setCuePoint("TrimOut", this->m_marks["o"]);

	if (this->m_loudness != 0)
		track->setMetadata("Loudness", this->m_loudness);

	if (this->m_duration != 0)
		track->setMetadata("Duration", this->m_duration);
}

/*

	Markfile Layout:
	0  1  2  3  4  5..x 
	F  x [v][mlen][meta]

	0  1  2  3  4..x
	H  s [hres][data]

*/

bool FXMark::loadMarkFile()
{
	QFile markFile(this->getMarkFilePath());
	if (markFile.exists()) {
		if (markFile.open(QIODevice::ReadOnly)) {
			{
				QByteArray header = markFile.read(2);
				if (strcmp(header.data(), "Fx") != 0)
					return false;
			}

			{
				unsigned char version = *markFile.read(1).data();
				if (version != this->MARK_VERSION)
					return false;
			}

			quint16 metaLength = reinterpret_cast<quint16>(markFile.read(2).data());
			if (metaLength > 0) {
				{
					QByteArray metaData = markFile.read(metaLength);

					msgpack::object_handle oh = msgpack::unpack(metaData.data(), metaData.size());
					msgpack::object markobj = oh.get();

					*this = markobj.convert(*this);
				}

				// TODO: Load histogram

				return true;
			}
		}
	}

	return false;
}

void FXMark::operator=(const FXMark& copyFrom)
{
	this->m_hashId = copyFrom.m_hashId;
	this->m_audioPath = copyFrom.m_audioPath;

	this->m_timeCreated = copyFrom.m_timeCreated;
	this->m_timeModified = copyFrom.m_timeModified;

	this->m_duration = copyFrom.m_duration;
	this->m_loudness = copyFrom.m_loudness;
	this->m_marks = copyFrom.m_marks;

	this->m_histogramResolution = copyFrom.m_histogramResolution;
	this->m_histogram = copyFrom.m_histogram;

	this->m_markDir = copyFrom.m_markDir;
}

bool FXMark::writeMarkFile()
{
	this->updateTimestamps();

	QFile markFile(this->getMarkFilePath());
	if (markFile.open(QIODevice::WriteOnly)) {
		QByteArray payload;

		// Write Fx Header + version (3 bytes)
		char header[] = { 'F', 'x' };
		payload.append(header, 2);
		payload.append(this->MARK_VERSION, 1);

		// Prepare meta msgpack payload
		msgpack::sbuffer metaBuf;
		msgpack::pack(metaBuf, *this);

		// Write metaLength (2 bytes)
		quint16 metaLength = metaBuf.size();
		payload.append((char*)&metaLength, sizeof(quint16));

		// Write metadata payload
		payload.append(metaBuf.data(), metaBuf.size());

		// Write Hs Header (2 bytes)
		char histinfo[] = { 'H', 's' };
		payload.append(histinfo, 2);

		// Write histogram resolution (2 bytes)
		payload.append(
			reinterpret_cast<char*>(&this->m_histogramResolution),
			sizeof(this->m_histogramResolution)
		);

		foreach(auto val, this->m_histogram) {
			char* buf = (char*)&val;
			payload.append(buf, sizeof(val));
		}

		return markFile.write(payload) > 0;
	}

	return false;
}