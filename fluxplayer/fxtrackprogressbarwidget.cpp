#include "fxtrackprogressbarwidget.h"

#include <QPainter>
#include <QResizeEvent>
#include <QDebug>

#include "fxhelpers.h"


FXTrackProgressBarWidget::FXTrackProgressBarWidget(QWidget *parent)
	: QWidget(parent)
{
	
}

FXTrackProgressBarWidget::~FXTrackProgressBarWidget()
{
}

QSize FXTrackProgressBarWidget::minimumSizeHint() const
{
	return QSize(100, 25);
}

QSize FXTrackProgressBarWidget::sizeHint() const
{
	//QSize frameSize = this->frameSize();
	//qDebug() << frameSize;
	return QSize(200, 25);
}

void FXTrackProgressBarWidget::setPlaying(const bool & playing)
{
	this->m_playing = playing;
	this->update();
}

void FXTrackProgressBarWidget::setAudioDuration(const double &duration)
{
	this->m_audioDuration = duration;
	this->update();
}

void FXTrackProgressBarWidget::setAudioPosition(const double &position)
{
	this->m_audioPosition = position;
	this->update();
}

void FXTrackProgressBarWidget::setShowDurationAsCountdown(const bool &countdown)
{
	this->m_showDurationAsCountdown = countdown;
	this->update();
}

void FXTrackProgressBarWidget::setProgressTitle(const QString &text)
{
	this->m_progressTitle = text;
	this->update();
}

void FXTrackProgressBarWidget::reset()
{
	this->m_audioPosition = -1;
	this->m_audioDuration = -1;
	this->m_progressTitle = "";

	this->update();
}

bool FXTrackProgressBarWidget::isAudioValid()
{
	return !(this->m_audioDuration == -1 || this->m_audioPosition == -1);
}

double FXTrackProgressBarWidget::getPositionRatio()
{
	if (!this->isAudioValid())
		return 0.0;

	double ratio = this->m_audioPosition / this->m_audioDuration;
	if (ratio < 0.0)
		return 0.0;
	else if (ratio > 1.0)
		return 1.0;

	return ratio;
}

void FXTrackProgressBarWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
}

void FXTrackProgressBarWidget::paintEvent(QPaintEvent *)
{
	// Init painter
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);

	// Calculate progress background
	unsigned int frameWidth, frameHeight;
	frameWidth = this->size().width();
	frameHeight = this->size().height();

	QPainterPath bgPath;
	QRectF bgRect(
		this->m_borderSize + 0.5,
		this->m_borderSize + 0.5,
		frameWidth - this->m_borderSize - 1.5,
		frameHeight - this->m_borderSize - 1.5
	);
	bgPath.addRoundedRect(
		bgRect,
		this->m_borderRadius, this->m_borderRadius
	);

	// Set clip path
	p.setClipPath(bgPath);

	// Draw progress border
	QPen bgPen(this->m_borderColor, this->m_borderSize);
	p.setPen(bgPen);

	// Draw progress background
	p.fillPath(bgPath, this->m_backgroundColor);

	// Draw progress bar
	QLinearGradient progressGradient(
		this->m_barSize.width() / 2,
		0, 
		this->m_barSize.width() / 2,
		this->m_barSize.height()
	);

	double timeDiff = this->m_audioDuration - this->m_audioPosition;
	if (timeDiff <= 5) {
		// Bright Red
		progressGradient.setColorAt(0, this->m_progressSecondWarnColor[0]);
		progressGradient.setColorAt(1, this->m_progressSecondWarnColor[1]);
	}
	else if (timeDiff <= 10) {
		// Dark Red
		progressGradient.setColorAt(0, this->m_progressFirstWarnColor[0]);
		progressGradient.setColorAt(1, this->m_progressFirstWarnColor[1]);
	}
	else {
		// Playing
		progressGradient.setColorAt(0, this->m_progressPlayingColor[0]);
		progressGradient.setColorAt(1, this->m_progressPlayingColor[1]);
	}

	QPainterPath progressPath;
	progressPath.addRoundedRect(
		QRectF(
			this->m_borderSize + 0.5,
			this->m_borderSize + 0.5,
			(frameWidth * this->getPositionRatio()) - this->m_borderSize - 1.5,
			frameHeight - this->m_borderSize - 1.5
		),
		this->m_borderRadius, this->m_borderRadius
	);

	QLinearGradient previewGradient = progressGradient;
	QPainterPath previewPath;
	if (this->m_seekPreviewPosition != -1) {
		previewGradient.setColorAt(0, QColor(60, 60, 60));
		previewGradient.setColorAt(1, QColor(40, 40, 40));

		previewPath.addRoundedRect(
			QRectF(
				this->m_borderSize + 0.5,
				this->m_borderSize + 0.5,
				(frameWidth * this->m_seekPreviewPosition / this->m_audioDuration) - this->m_borderSize - 1.5,
				frameHeight - this->m_borderSize - 1.5
			),
			this->m_borderRadius, this->m_borderRadius
		);

		if (this->m_seekPreviewPosition >= this->m_audioPosition)
			p.fillPath(previewPath, QBrush(previewGradient));
		else
			progressPath = previewPath;
	}

	if (this->getPositionRatio() != 0.0)
		p.fillPath(progressPath, QBrush(progressGradient));

	//if (this->m_seekPreviewPosition < this->m_audioPosition)
	//	p.fillPath(previewPath, QBrush(previewGradient));

	// Draw time
	QPen timePen(this->m_playing ? this->m_progressTimePlayingColor : this->m_progressTimeColor);
	p.setPen(timePen);

	QFont timeFont("Calibri", this->m_barSize.height() / 2);
	p.setFont(timeFont);
	QRect timeRect(
		this->m_barSize.width() - this->m_textXPad,
		0,
		0,
		this->m_barSize.height()
	);

	int timeFlags = Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip;

	QString timeText;
	if (this->isAudioValid()) {
		double timePos;
		if (this->m_seekPreviewPosition == -1) {
			timePos = this->m_audioPosition;
		}
		else {
			timePos = this->m_seekPreviewPosition;
			p.setPen(QPen(this->m_progressTimeSeekingColor));
		}

		if (this->m_showDurationAsCountdown) {
			double diff = timePos - this->m_audioDuration;
			timeText = FXHelpers::formatSecToTime(diff, diff > -10 && diff <= 0);
		}
		else {
			timeText = QString("%1/%2").arg(FXHelpers::formatSecToTime(timePos), FXHelpers::formatSecToTime(this->m_audioDuration));
		}
	}
	else {
		timeText = "0:00";
	}

	p.drawText(timeRect, timeFlags, timeText);

	// Draw title text
	QFontMetrics timeFm(timeFont);

	int titleXStart = this->m_textXPad + 5;
	QRect textRect(
		titleXStart,
		0,
		this->m_barSize.width() - titleXStart - (this->m_textXPad * 2) - timeFm.horizontalAdvance(timeText),
		this->m_barSize.height()
	);

	int textFlags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine;
	p.setFont(QFont("Calibri", this->m_barSize.height() / 3));

	QPen titlePen(this->m_progressTitleColor);
	p.setPen(titlePen);
	p.drawText(textRect, textFlags, this->m_progressTitle);

	// Draw border
	if (this->m_playing) {
		p.setClipping(false);

		p.setPen(bgPen);
		p.drawPath(bgPath);
	}
}

void FXTrackProgressBarWidget::resizeEvent(QResizeEvent *event)
{
	this->m_size = event->size();

	// modify to only barsize
	this->m_barSize = event->size();
}

void FXTrackProgressBarWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	switch (event->button()) {
		case Qt::LeftButton:
			if (this->isAudioValid()) {
				double xRatio = static_cast<double>(event->x()) / this->m_barSize.width();
				if (abs(this->getPositionRatio() - xRatio) < 0.05) {
					this->setCursor(Qt::SizeHorCursor);
					this->m_mousePressedX = event->x();
				}
			}
			break;

		case Qt::RightButton:
			if (this->isAudioValid()) {
				emit seekToTime(static_cast<double>(event->x()) / this->m_barSize.width() * this->m_audioDuration);
			}
			break;
	}
}

void FXTrackProgressBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	this->setCursor(QCursor());
	this->m_seekPreviewPosition = -1;

	if (this->m_mousePressedX != -1) {
		int xDiff = abs(this->m_mousePressedX - event->x());
		this->m_mousePressedX = -1;

		if (this->isAudioValid()) {
			if (xDiff > 2) {
				double pos = static_cast<double>(h_filterPositive(event->x())) / this->m_barSize.width() * this->m_audioDuration;
				emit seekToTime(h_max(pos, this->m_audioDuration));
			}
		}
		else {
			this->update();
		}
	}
}

void FXTrackProgressBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	QWidget::mouseMoveEvent(event);

	if (this->m_mousePressedX != -1) {
		double pos = static_cast<double>(h_filterPositive(event->x())) / this->m_barSize.width() * this->m_audioDuration;
		this->m_seekPreviewPosition = h_max(pos, this->m_audioDuration);
		this->update();
	}
}

void FXTrackProgressBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	QWidget::mouseDoubleClickEvent(event);
}

void FXTrackProgressBarWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);

	if (this->isAudioValid()) {
		double steps = event->angleDelta().y() / this->m_wheelStep;
		emit seekRelativeTime(steps * this->m_wheelPad);
	}
}
