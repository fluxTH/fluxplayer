#pragma once

#include <QString>
#include <QList>
#include <QDir>
#include <map>

#include <xxhash.h>
#include <msgpack.hpp>

#include "fxtrack.h"

class FXMark {
public:
	static const quint8 MARK_VERSION = 1;
	static const quint64 HASH_SEED = 0xFEEDBABEDEADBEEF;

	MSGPACK_DEFINE(
		m_hashId,
		m_audioPath,
		m_timeCreated,
		m_timeModified,
		m_duration,
		m_loudness,
		m_marks
	);

	FXMark(const std::string& audioPath);
	FXMark(const QString& audioPath) : FXMark(audioPath.toStdString()) {};

	QString getMarkFileName() const;
	QString getMarkFilePath() const;

	bool analyzerShouldRun();

	void setInMark(const float& position);
	void setFadeInMark(const float& position);
	void setNextMark(const float& position);
	void setFadeOutMark(const float& position);
	void setOutMark(const float& position);

	void setDuration(const float& duration);
	void setLoudness(const float& loudness);
	void setHistogram(const QList<quint8>& histogram, const quint16& resolution);

	void updateTrack(FXTrack* track);
	bool writeMarkFile();
	bool loadMarkFile();

	void operator=(const FXMark& copyFrom);

private:
	XXH64_hash_t m_hashId;
	std::string m_audioPath;

	qint64 m_timeCreated;
	qint64 m_timeModified;

	float m_duration;
	float m_loudness;
	std::map<std::string, float> m_marks;

	quint16 m_histogramResolution;
	QList<quint8> m_histogram;

	QDir m_markDir;

	bool m_loadSuccess;

	void setMark(const std::string& key, const float& value);
	bool calculateHashId();
	void updateTimestamps();
};