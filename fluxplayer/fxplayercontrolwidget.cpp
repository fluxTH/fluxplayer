#include "fxplayercontrolwidget.h"

#include <QDebug>
#include <QTime>

#include "fxhelpers.h"
#include "fluxplayer.h"

FXPlayerControlWidget::FXPlayerControlWidget(QWidget *parent)
	: QWidget(parent),
	m_isPlayerAttached(false),
	m_posIsCountdown(true),
	m_currentPlayer(nullptr),
	TIMELABEL_MASK("<a href='#' style='color:black;text-decoration:none;font-weight:bold;'>%1</a>")
{
	ui.setupUi(this);
	this->initializeComponents();
}

void FXPlayerControlWidget::initializeComponents()
{
	// Initialize heartbeat timer
	this->m_timer = new QTimer(this);
	QObject::connect(this->m_timer, SIGNAL(timeout()), this, SLOT(tick()));
	this->m_timer->start(100);

	QObject::connect(ui.audioProgressBar, SIGNAL(seekTo(double)), this, SLOT(handleSeekTo(double)));

	QObject::connect(ui.btn_playPause, SIGNAL(clicked()), this, SLOT(togglePlay()));
	QObject::connect(ui.btn_fwd, SIGNAL(clicked()), this, SIGNAL(playNext()));
	QObject::connect(ui.btn_prev, SIGNAL(clicked()), this, SIGNAL(playPrev()));

	ui.label_audioCover->setText("<div style=\"color:#aaaaaa;text-align:center;\">(no cover)</div>");

	QStringList icons = {"play", "pause"};
	foreach(QString iconName, icons) {
		QIcon playIcon(QString(":/FluxPlayer/Resources/icons/%1.svg").arg(iconName));
		this->m_icons.insert(iconName, playIcon);
	}

	ui.btn_playPause->setIcon(this->m_icons["play"]);
	ui.btn_playPause->setIconSize(QSize(30, 30));

	// Setup tool btns
	QSize toolBtnSize(15, 15);
	ui.btn_fwd->setIcon(QIcon(":/FluxPlayer/Resources/icons/next.svg"));
	ui.btn_fwd->setIconSize(toolBtnSize);

	ui.btn_prev->setIcon(QIcon(":/FluxPlayer/Resources/icons/previous.svg"));
	ui.btn_prev->setIconSize(toolBtnSize);

	ui.btn_repeat->setIcon(QIcon(":/FluxPlayer/Resources/icons/loop.svg"));
	ui.btn_repeat->setIconSize(toolBtnSize);

	ui.btn_append->setIcon(QIcon(":/FluxPlayer/Resources/icons/arrows.svg"));
	ui.btn_append->setIconSize(toolBtnSize);
}

FXPlayerControlWidget::~FXPlayerControlWidget()
{
	delete this->m_timer;
}

void FXPlayerControlWidget::attachPlayer(FXPlayer *player, const bool &forced)
{
	if (forced || !this->m_isPlayerAttached) {

		this->m_currentPlayer = player;
		this->m_isPlayerAttached = true;

		ui.audioProgressBar->setAudioDuration(player->getTimeDuration() - 7);

		ui.label_playerName->setText(
			QString("<div style='font-size:20pt;text-align:center;'>%1</div>")
			.arg(*player->getName())
		);

		FXTrack *track = player->getTrack();

		ui.audioProgressBar->setProgressTitle(track->getFileInfo()->fileName());

		QString unknownText("<span style='color:#666666;'>( Unknown )</span>");
		ui.label_InfoValue1->setText(QString("<b>%1</b>").arg(track->getTitle()));
		ui.label_InfoValue2->setText(QString("<b>%1</b>").arg(track->getArtist(unknownText)));

		BASS_CHANNELINFO info = player->getAudioStreamInfo();
		QString resText = QString("%1-bit ").arg(QString::number(info.origres));

		ui.label_audioInfo->setText(
			QString("%1 %2(%3ch), %4 kbps, %5 Hz")
			.arg(player->getAudioType())
			.arg(info.origres == 0 ? "" : resText)
			.arg(QString::number(info.chans))
			.arg(player->getAudioAvgBitrate(), 0, 'f', 0)
			.arg(QString::number(info.freq))
		);

		ui.label_audioDriver->setText(QString("<div style='color:#aaaaaa;text-align:right'>%1</div>").arg(player->getAudioDriver()));
	}
}

void FXPlayerControlWidget::detachPlayer()
{
	this->m_currentPlayer = nullptr;

	this->m_isPlayerAttached = false;
	ui.audioProgressBar->reset();
}


// Slots
void FXPlayerControlWidget::tick()
{
	if (this->m_isPlayerAttached) {
		ui.audioProgressBar->setAudioPosition(this->m_currentPlayer->getTimePosition());
	}
}

void FXPlayerControlWidget::toggleCountdown(QString l)
{
	this->toggleCountdown();
}

void FXPlayerControlWidget::toggleCountdown()
{
	this->m_posIsCountdown = !this->m_posIsCountdown;
	ui.audioProgressBar->setShowDurationAsCountdown(this->m_posIsCountdown);
}

void FXPlayerControlWidget::togglePlay()
{
	if (this->m_isPlayerAttached) {
		if (this->m_currentPlayer->isPlaying()) {
			// pause
			this->m_currentPlayer->pause();
		}
		else {
			// play
			this->m_currentPlayer->play();
		}
	}
}

void FXPlayerControlWidget::handlePlayerPlayed(FXPlayer *player)
{
	if (this->m_currentPlayer == player) {
		qDebug() << "player played!!!";
		ui.btn_playPause->setIcon(this->m_icons["pause"]);
		ui.audioProgressBar->setPlaying(true);
	}
}

void FXPlayerControlWidget::handlePlayerStopped(FXPlayer *player)
{
	if (this->m_currentPlayer == player) {
		qDebug() << "player stopped!!!";
		ui.btn_playPause->setIcon(this->m_icons["play"]);
		ui.audioProgressBar->setPlaying(false);
	}
}

void FXPlayerControlWidget::handleSeekTo(double sec)
{
	if (this->m_isPlayerAttached) {
		this->m_currentPlayer->seek(sec / this->m_currentPlayer->getTimeDuration());
	}
}
