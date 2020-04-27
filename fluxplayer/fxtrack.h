#pragma once

#include <QObject>
#include <QFileInfo>
#include <QMap>
#include <QVariant>

#include "tag.h"
#include "fileref.h"


class FXPlayer;

class FXTrack : public QObject
{
	Q_OBJECT

public:
	FXTrack(QObject *parent, QString filePath);
	~FXTrack();

	QFileInfo* getFileInfo();
	
	void readTags();

	QVariant getMetadata(const QString &key, const QVariant &value = QVariant());
	QString getTitle();
	QString getArtist(const QString &unknownText = QString("( Unknown )"));

	void setMetadata(const QString &key, const QVariant &value);

	bool isLoadedToPlayer();
	void setPlayer(FXPlayer *player);
	FXPlayer* getPlayer();
	void removePlayer();

	void setPlayerErrorCode(const int& code);
	int& getPlayerErrorCode();
	bool hasPlayerError();

	void setPlayed(const bool &played);
	bool hasPlayed();

private:
	QFileInfo m_file;
	QMap<QString, QVariant> m_metadata;
	TagLib::FileRef m_tagFileRef;

	FXPlayer *m_player;
	int m_playerErrorCode;

	bool m_played;

signals:
	void textUpdated(FXTrack*);

};
