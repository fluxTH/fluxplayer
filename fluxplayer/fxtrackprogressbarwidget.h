#pragma once

#include <QWidget>
#include <QPainter>


class FXTrackProgressBarWidget : public QWidget
{
	Q_OBJECT

public:
	FXTrackProgressBarWidget(QWidget *parent);
	~FXTrackProgressBarWidget();

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	void setPlaying(const bool &playing);
	void setAudioDuration(const double &duration);
	void setAudioPosition(const double &position);
	void setShowDurationAsCountdown(const bool &countdown);
	void setProgressTitle(const QString &text);

	void reset();

	bool isAudioValid();
	double getPositionRatio();

protected:
	void showEvent(QShowEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	const unsigned short int m_borderSize = 1;
	const unsigned short int m_borderRadius = 5;
	const unsigned short int m_textXPad = 5;
	const double m_wheelStep = 120.0;
	const unsigned short int m_wheelPad = 2; // sec

	QSize m_size;
	QSize m_barSize;

	// Base color
	const QColor m_backgroundColor = Qt::black;
	const QColor m_borderColor = Qt::blue;

	// Bar color
	const QColor m_progressPlayingColor[2] = { QColor(80, 120, 20), QColor(50, 70, 30) };
	const QColor m_progressFirstWarnColor[2] = { QColor(150, 20, 20), QColor(70, 10, 20) };
	const QColor m_progressSecondWarnColor[2] = { QColor(200, 0, 10), QColor(120, 10, 20) };

	// Text color
	const QColor m_progressTitleColor = Qt::white;
	const QColor m_progressTimeColor = Qt::white;
	const QColor m_progressTimePlayingColor = QColor(150, 230, 80);
	const QColor m_progressTimeSeekingColor = Qt::yellow;

	QString m_progressTitle;
	double m_audioDuration = -1;
	double m_audioPosition = -1;

	double m_seekPreviewPosition = -1;
	
	bool m_playing = false;
	bool m_showDurationAsCountdown = true;
	int m_mousePressedX = -1;

signals:
	void seekToTime(double);
	void seekRelativeTime(double);
};
