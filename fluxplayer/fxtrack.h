#pragma once

#include <QObject>
#include <QFileInfo>
#include <QMap>
#include <QVariant>
#include <QMutex>
#include <QPixmap>

class FXPlayer;

class FXTrack : public QObject
{
	Q_OBJECT

public:
	FXTrack(QObject* parent, QString filePath);
	~FXTrack();

	QMutex mutex;

	QFileInfo* getFileInfo();

	void readTags();
	void readCoverPicture();

	QVariant getMetadata(const QString& key, const QVariant& value = QVariant());
	QString getTitle();
	QString getArtist(const QString& unknownText = QString("( Unknown )"));
	double getRuntime();

	void setMetadata(const QString& key, const QVariant& value);

	bool isLoadedToPlayer();
	void setPlayer(FXPlayer* player);
	FXPlayer* getPlayer();
	void removePlayer();

	void setPlayerErrorCode(const int& code);
	int& getPlayerErrorCode();
	bool hasPlayerError();

	void setPlayed(const bool& played);
	bool hasPlayed();

	bool hasPlayMetadata();
	bool hasCuePoints();
	bool hasNormalization();

	double getNormalizationGain();

	QPixmap* getCoverPixmap();
	void deleteCoverPixmap();

	void setCuePoint(const QString& name, const double& pos);
	double getCuePoint(const QString& name);

private:
	QFileInfo m_file;
	QMap<QString, QVariant> m_metadata;
	QMap<QString, double> m_cuePoints;
	QPixmap* m_coverPixmap;

	FXPlayer* m_player;
	int m_playerErrorCode;

	bool m_played;

signals:
	void textUpdated(FXTrack*);
};
