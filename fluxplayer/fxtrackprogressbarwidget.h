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

private:
	const unsigned short int m_borderSize = 1;
	const unsigned short int m_borderRadius = 5;
	unsigned short int m_textXPad = 5;

	QSize m_size;
	QSize m_barSize;

	QColor m_backgroundColor = Qt::black;
	QColor m_borderColor = Qt::blue;

	QString m_progressTitle;
	double m_audioDuration = -1;
	double m_audioPosition = -1;

	double m_seekPreviewPosition = -1;
	
	bool m_playing = false;
	bool m_showDurationAsCountdown = true;
	bool m_mousePressing = false;

	QPainterPath m_clipPath;

signals:
	void seekTo(double);
};
