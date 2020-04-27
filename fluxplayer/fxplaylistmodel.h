#pragma once

#include <QAbstractTableModel>

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

	bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
	bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

	bool addTrack(const QString &filePath);

	void setPlayerContainer(QList<FXPlayer*>* players);
	bool hasPlayerContainer();

	bool loadPlayers(const bool &forceAttach = false);
	bool playStart();
	bool playNext(const bool &eject = true);
	bool playPrev(const bool &eject = true);
	bool playAt(const int &row, const bool &eject = true);
	bool playTrack(FXTrack *track);

	bool deleteAt(const unsigned int &row);
	bool deleteRange(const QVector<unsigned int> &rows);

	void ejectPlayer(FXPlayer *player);

private:
	bool m_active;
	int m_currentTrack;
	FXPlayer *m_mainPlayer;

	QList<FXTrack*> m_playlist;
	QList<FXPlayer*> *m_players;

	FXPlayer* getNextFreePlayer();

	void updateRow(const int &index);
	bool shouldPlayNext();

signals:
	void attachControlToPlayer(FXPlayer*, bool = true);
	void doEjectPlayer(FXPlayer*);

public slots:
	void handleTrackTextUpdate(FXTrack*);

	void handlePlayNext();
	void handleFadeNext();
	void handlePlayPrev();
	void handlePlayAt(const QModelIndex&);

	void handlePlayerEjected(FXPlayer*);
	void handlePlayerEjected(FXPlayer*, const bool &reload);
};
