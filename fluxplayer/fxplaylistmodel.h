#pragma once

#include <QAbstractTableModel>
#include <QThreadPool>
#include <QPointer>

#include "fxplayer.h"
#include "fxtrack.h"

class FXPlaylistModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	FXPlaylistModel(QObject *parent);
	~FXPlaylistModel();

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	Qt::DropActions supportedDropActions() const override;
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

	QMimeData* mimeData(const QModelIndexList& indexes) const override;

	bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
	bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

	bool addTrack(const QString &filePath);

	void setPlayerContainer(QList<FXPlayer*>* players);
	bool hasPlayerContainer();

	bool loadPlayers();
	bool loadPlayers(const bool &forceAttach);

	bool playStart();
	bool playNext(const bool &eject = true);
	bool playPrev(const bool &eject = true);
	bool playAt(const int &row, const bool &eject = true);
	bool playTrack(FXTrack *track);

	bool deleteAt(const unsigned int &row);
	bool deleteRange(const QVector<unsigned int> &rows);

	void ejectPlayer(FXPlayer *player);

	bool isActive() const;
	void setActive(const bool &active);

	double getTotalRuntime();

private:
	bool m_active;
	int m_currentTrack;

	QList<QPointer<FXTrack>> m_playlist;
	QList<FXPlayer*> *m_players;
	QThreadPool m_trackAnalyzerPool;

	FXPlayer *m_mainPlayer;

	FXPlayer* getNextFreePlayer();

	void updateRow(const int &index);
	bool shouldPlayNext() const;

	void analyzeTracks();
	bool analyzeTrack(FXTrack *track);

	void setMainPlayer(FXPlayer *player);
	void updateStatusText();

signals:
	void attachControlToPlayer(FXPlayer*, bool = true);
	void doEjectPlayer(FXPlayer*);

	void normalizationChanged(double);
	void statusBarTextChanged(QString);

public slots:
	void handleTrackTextUpdate(FXTrack*);

	void handlePlayNext();
	void handleFadeNext();
	void handlePlayPrev();
	void handlePlayAt(const QModelIndex&);

	void handlePlayerEjected(FXPlayer*);
	void handlePlayerEjected(FXPlayer*, const bool &reload);

	void handleCuePointsAnalyzed(FXTrack*);
};
