#pragma once

#include <QWidget>
#include <QTimer>
#include "ui_fxplayercontrolwidget.h"

#include "fxplayer.h"

class FXPlayerControlWidget : public QWidget
{
	Q_OBJECT

public:
	FXPlayerControlWidget(QWidget *parent = Q_NULLPTR);
	~FXPlayerControlWidget();

	void attachPlayer(FXPlayer *player, const bool &forced = true);
	void detachPlayer();

private:
	Ui::FXPlayerControlWidget ui;
	QTimer *m_timer;

	bool m_isPlayerAttached;
	FXPlayer *m_currentPlayer;

	QMap<QString, QIcon> m_icons;

	bool m_posIsCountdown;

	void initializeComponents();
	void resetDisplay();

signals:
	void playNext();
	void playPrev();

public slots:
	void tick();
	void toggleCountdown(QString l);
	void toggleCountdown();
	void togglePlay();

	void handlePlayerPlayed(FXPlayer*);
	void handlePlayerStopped(FXPlayer*);

	void handleSeekToTime(double);
	void handleSeekRelativeTime(double);
};
