#include "fxplaylistmodel.h"

#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QBrush>

#include "bass.h"
#include "fxhelpers.h"


FXPlaylistModel::FXPlaylistModel(QObject *parent)
	: QAbstractTableModel(parent),
	m_players(nullptr),
	m_mainPlayer(nullptr),
	m_currentTrack(-1),
	m_active(true)
{

}

FXPlaylistModel::~FXPlaylistModel()
{
	this->m_active = false;
	qDeleteAll(this->m_playlist);
}

int FXPlaylistModel::rowCount(const QModelIndex & parent) const
{
	return this->m_playlist.count();
}

int FXPlaylistModel::columnCount(const QModelIndex & parent) const
{
	return 4;
}

QVariant FXPlaylistModel::data(const QModelIndex & index, int role) const
{
	int row = index.row();
	int col = index.column();

	switch (role) {
		case Qt::DisplayRole:
			switch (col) {
				case 0:
					if (this->m_playlist[row]->isLoadedToPlayer())
						return QString(*this->m_playlist[row]->getPlayer()->getName());

					if (row >= this->m_currentTrack) {
						if (this->m_playlist[row]->hasPlayerError())
							return QString();
						return QString::number(row + 1 - this->m_currentTrack);
					}

					return QString();
				case 1:
					return this->m_playlist[row]->getTitle();
				case 2:
					return this->m_playlist[row]->getArtist();
				case 3:
					return FXHelpers::formatSecToTime(this->m_playlist[row]->getMetadata("Duration", 0).toInt());
				default:
					return QString();
			}

		case Qt::FontRole:
			if ((col == 0 && this->m_playlist[row]->isLoadedToPlayer())
				|| row == this->m_currentTrack) {

				QFont font;
				font.setBold(true);
				return font;
			}

			if (row < this->m_currentTrack && !this->m_playlist[row]->isLoadedToPlayer()) {
				QFont font;
				font.setItalic(true);
				return font;
			}

			break;

		case Qt::ForegroundRole:
			if (this->m_playlist[row]->hasPlayerError())
				return QBrush(Qt::red);

			if (row < this->m_currentTrack) {
				if (this->m_playlist[row]->isLoadedToPlayer())
					return QBrush(Qt::darkGray);

				return QBrush(Qt::gray);
			}

			break;

		case Qt::BackgroundRole:
			if (row == this->m_currentTrack)
				return QBrush(Qt::yellow);

			if (row > this->m_currentTrack && this->m_playlist[row]->isLoadedToPlayer()) {
				return QBrush(QColor(255, 255, 225));
			}

			break;

		case Qt::TextAlignmentRole:
			switch (col) {
				case 0:
				case 3:
					return Qt::AlignCenter;
				default:
					return Qt::AlignLeft + Qt::AlignVCenter;
			}

		case Qt::DecorationRole:
			if (col == 0 && this->m_playlist[row]->hasPlayerError()) {
				return QIcon(":/FluxPlayer/Resources/icons/times.svg");
			}

			if (col == 0 && row < this->m_currentTrack && !this->m_playlist[row]->isLoadedToPlayer()) {
				if (this->m_playlist[row]->hasPlayed())
					return QIcon(":/FluxPlayer/Resources/icons/check.svg");

				return QIcon(":/FluxPlayer/Resources/icons/skipped.svg");
			}
			
			break;

	}

	return QVariant();
}

QVariant FXPlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal) {
		switch (role) {
			case Qt::DisplayRole:
				switch (section) {
					case 0:
						return QString("#");
					case 1:
						return QString("Title");
					case 2:
						return QString("Artist");
					case 3:
						return QString("Runtime");
				}
				break;

			case Qt::TextAlignmentRole:
				switch (section) {
					case 0:
					case 3:
						return Qt::AlignCenter;
					default:
						return Qt::AlignLeft + Qt::AlignVCenter;
				}
		}
	}
	return QVariant();
}

Qt::ItemFlags FXPlaylistModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

	if (index.isValid())
		return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;

	return Qt::ItemFlags();
}

Qt::DropActions FXPlaylistModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

bool FXPlaylistModel::insertRows(int position, int rows, const QModelIndex &index)
{
	return false;
}

bool FXPlaylistModel::removeRows(int position, int rows, const QModelIndex &index)
{
	return false;
}

bool FXPlaylistModel::addTrack(const QString &filePath)
{
	FXTrack *track = new FXTrack(this, filePath);
	this->m_playlist.append(track);

	QObject::connect(track, SIGNAL(textUpdated(FXTrack*)), this, SLOT(handleTrackTextUpdate(FXTrack*)));

	int row = this->m_playlist.indexOf(track);
	emit layoutChanged();
	this->updateRow(row);

	return true;
}

void FXPlaylistModel::setPlayerContainer(QList<FXPlayer*>* players)
{
	this->m_players = players;
}

bool FXPlaylistModel::hasPlayerContainer()
{
	return !(this->m_players == nullptr);
}

bool FXPlaylistModel::loadPlayers(const bool &forceAttach)
{
	if (this->hasPlayerContainer()) {
		qDebug() << "Load players triggered, current playlist index:" << this->m_currentTrack;

		for (int i = this->m_currentTrack < 0 ? 0 : this->m_currentTrack; 
				i < this->m_playlist.length(); i++) {

			FXTrack *track = this->m_playlist.at(i);
			if (i < this->m_currentTrack || track->isLoadedToPlayer())
				continue;

			// Get player
			//qDebug() << "Attempting to get player for" << i;
			FXPlayer *player = this->getNextFreePlayer();
			if (player == nullptr)
				break;

			// Load
			qDebug() << "Player" << *player->getName() << "will load track" << track->getFileInfo()->fileName();
			if (!player->loadTrack(track)) {
				track->setPlayerErrorCode(BASS_ErrorGetCode());
				int code = track->getPlayerErrorCode();
				qDebug() << code;
				continue;
			}

			// Update playlist
			if (this->m_currentTrack == -1) {
				this->m_currentTrack = i;
				this->playStart();
			}

			this->updateRow(i);

			// Attach
			if (forceAttach)
				this->m_mainPlayer = player;
			emit attachControlToPlayer(player, forceAttach);
		}

		return true;
	}

	return false;
}

bool FXPlaylistModel::playStart()
{
	if (this->m_currentTrack != -1) {
		return this->playAt(this->m_currentTrack);
	}

	return false;
}

bool FXPlaylistModel::playNext(const bool &eject)
{
	int trackIndex = this->m_currentTrack;
	return this->playAt(++trackIndex, eject);
}

bool FXPlaylistModel::playPrev(const bool &eject)
{
	int trackIndex = this->m_currentTrack;
	return this->playAt(--trackIndex, eject);
}

bool FXPlaylistModel::playAt(const int &row, const bool &eject)
{
	if (this->m_playlist.count() == 0 || row < 0 || !(row < this->m_playlist.count()) || !this->hasPlayerContainer())
		return false;

	qDebug() << "PlayAt" << row << ": eject" << eject;

	int prevIndex = this->m_currentTrack;

	this->m_currentTrack = row;
	FXTrack *track = this->m_playlist.value(row);

	if (eject) {
		if (prevIndex != -1) {
			FXTrack *prevTrack = this->m_playlist[prevIndex];
			if (prevTrack->isLoadedToPlayer())
				prevTrack->getPlayer()->stop();
		}

		// If not selected playing track
		if (row != prevIndex) {
			short int selectDistance = abs(row - prevIndex);
			int playerCount = this->m_players->count();

			if (selectDistance < playerCount) {
				// Possibly still in loaded player range
				if (row > prevIndex) {
					// Selected one of currently loaded player, reallocate top players
					for (short int i = prevIndex; i < prevIndex + selectDistance; i++) {
						FXTrack *track = this->m_playlist.value(i, nullptr);
						if (track != nullptr)
							if (track->isLoadedToPlayer())
								this->ejectPlayer(track->getPlayer());
					}
				}
				else {
					// Selected before first loaded player, reallocate bottom players
					short int start = prevIndex + (playerCount - 1);
					for (short int i = start; i > start - selectDistance; i--) {
						FXTrack *track = this->m_playlist.value(i, nullptr);
						if (track != nullptr)
							if (track->isLoadedToPlayer())
								this->ejectPlayer(track->getPlayer());
					}
				}
			}
			else {
				// Clicked elsewhere, eject all
				foreach(FXPlayer *player, *this->m_players) {
					this->ejectPlayer(player);
				}
			}

			this->loadPlayers();
		}
	}

	this->updateRow(row);
	return this->playTrack(track);
}

bool FXPlaylistModel::playTrack(FXTrack *track)
{
	if (track->isLoadedToPlayer()) {
		FXPlayer *player = track->getPlayer();
		this->m_mainPlayer = player;

		emit attachControlToPlayer(player);

		if (!player->isPlaying())
			return player->play();

		return true;
	}
	else {
		qWarning() << "Cannot play, track" << track << "not loaded to player!";
		if (track->hasPlayerError())
			return this->playNext();
	}

	return false;
}

bool FXPlaylistModel::deleteAt(const unsigned int &row)
{
	return this->deleteRange(QVector<unsigned int>() << row);
}

bool FXPlaylistModel::deleteRange(const QVector<unsigned int> &rows)
{
	int currentShift = 0;
	int deleted = 0;

	for (int i = rows.count() - 1; i >= 0; --i) {
		unsigned int row = rows[i];
		if (row == this->m_currentTrack)
			continue;

		if (row < this->m_currentTrack)
			currentShift--;

		FXTrack *track = this->m_playlist[row];
		this->m_playlist.removeAt(row);

		if (track->isLoadedToPlayer())
			emit doEjectPlayer(track->getPlayer());

		delete track;
		deleted++;
	}

	this->m_currentTrack += currentShift;
	emit this->layoutChanged();

	return deleted > 0;
}

void FXPlaylistModel::ejectPlayer(FXPlayer *player) {
	this->m_mainPlayer = nullptr;
	emit doEjectPlayer(player);
}

void FXPlaylistModel::updateRow(const int &index)
{
	emit dataChanged(this->createIndex(index, 0), this->createIndex(index, this->columnCount() - 1), { Qt::DisplayRole });
}

FXPlayer* FXPlaylistModel::getNextFreePlayer()
{
	foreach (FXPlayer *player, *this->m_players) {
		if (player->isFree())
			return player;
	}

	return nullptr;
}

bool FXPlaylistModel::shouldPlayNext()
{
	return this->m_active;
}

// Slots
void FXPlaylistModel::handleTrackTextUpdate(FXTrack *track) {
	int index = this->m_playlist.indexOf(track);
	if (index != -1)
		this->updateRow(index);
}

void FXPlaylistModel::handlePlayNext()
{
	qDebug() << "Play next activated!";
	this->playNext();
}

void FXPlaylistModel::handlePlayAt(const QModelIndex &index)
{
	int row = index.row();
	qDebug() << "Play at" << row << "activated";

	this->playAt(row);
}

void FXPlaylistModel::handleFadeNext()
{
	if (this->shouldPlayNext())
		this->playNext(false);
}

void FXPlaylistModel::handlePlayPrev()
{
	qDebug() << "Play prev activated!";
	this->playPrev();
}

void FXPlaylistModel::handlePlayerEjected(FXPlayer *player) {
	this->handlePlayerEjected(player, true);
}

void FXPlaylistModel::handlePlayerEjected(FXPlayer *player, const bool &reload)
{
	if (this->m_mainPlayer == player) {
		if (this->shouldPlayNext()) {
			if (!player->isPlaying())
				this->playNext();
		}
		else {
			// TODO: Make when finish playback and not continuing, cue the next track (dont play)
			//this->m_currentTrack++;
			//this->loadPlayers();
		}	
	}
	else {
		if (reload)
			this->loadPlayers();
	}
}