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
	QSize frameSize = this->frameSize();
	qDebug() << frameSize;
	return frameSize;
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

void FXTrackProgressBarWidget::showEvent(QShowEvent * event)
{
	this->m_clipPath = QPainterPath();
	this->m_clipPath.addRoundedRect(
		QRectF(
			this->m_borderSize,
			this->m_borderSize - 0.5,
			this->m_size.width() - this->m_borderSize - 0.5,
			this->m_size.height() - this->m_borderSize + 0.5
		),
		this->m_borderRadius, this->m_borderRadius
	);
}

void FXTrackProgressBarWidget::paintEvent(QPaintEvent *)
{
	// Init painter
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);
	p.setClipPath(this->m_clipPath);

	// Calculate progress background
	unsigned int frameWidth, frameHeight;
	frameWidth = this->m_size.width();
	frameHeight = this->m_size.height();

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

	// Draw progress border
	QPen bgPen(this->m_borderColor, this->m_borderSize);
	p.setPen(bgPen);

	// Draw progress background
	p.fillPath(bgPath, this->m_backgroundColor);

	// Draw progress bar
	QLinearGradient progressGradient(
		this->m_size.width() / 2, 
		0, 
		this->m_size.width() / 2, 
		this->m_size.height()
	);

	double timeDiff = this->m_audioDuration - this->m_audioPosition;
	if (timeDiff <= 6) {
		// Bright Red
		progressGradient.setColorAt(0, QColor(200, 0, 10));
		progressGradient.setColorAt(1, QColor(120, 10, 20));
	}
	else if (timeDiff <= 11) {
		// Dark Red
		progressGradient.setColorAt(0, QColor(150, 20, 20));
		progressGradient.setColorAt(1, QColor(70, 10, 20));
	}
	else {
		// Playing
		progressGradient.setColorAt(0, QColor(80, 120, 20));
		progressGradient.setColorAt(1, QColor(50, 70, 30));
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

	p.fillPath(progressPath, QBrush(progressGradient));

	//if (this->m_seekPreviewPosition < this->m_audioPosition)
	//	p.fillPath(previewPath, QBrush(previewGradient));

	// Draw text
	QPen fgPen(Qt::white);
	p.setPen(fgPen);

	QRect textRect(
		this->m_textXPad + 5,
		0,
		0,
		this->m_size.height()
	);

	int textFlags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip;
	p.setFont(QFont("Calibri", this->m_size.height() / 3));
	p.drawText(textRect, textFlags, this->m_progressTitle);

	// Draw time
	p.setFont(QFont("Calibri", this->m_size.height() / 2));
	QRect timeRect(
		this->m_size.width() - this->m_textXPad,
		0,
		0,
		this->m_size.height()
	);

	int timeFlags = Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip;
	double timePos;
	if (this->m_seekPreviewPosition == -1) {
		timePos = this->m_audioPosition;
	}
	else {
		timePos = this->m_seekPreviewPosition;
		p.setPen(QPen(Qt::yellow));
	}

	QString timeText;
	if (this->m_showDurationAsCountdown) {
		double diff = this->m_audioDuration - timePos;
		timeText = QString("%1%2").arg(diff == 0 ? "" : "-", FXHelpers::formatSecToTime(diff));
	}
	else {
		timeText = QString("%1 / %2").arg(FXHelpers::formatSecToTime(timePos), FXHelpers::formatSecToTime(this->m_audioDuration));
	}

	p.drawText(timeRect, timeFlags, timeText);

	// Draw border
	if (this->m_playing) {
		p.setPen(bgPen);
		p.drawPath(bgPath);
	}

	//p.fillPath(this->m_clipPath, Qt::green);

}

void FXTrackProgressBarWidget::resizeEvent(QResizeEvent *event)
{
	this->m_size = event->size();

	// modify to only barsize
	this->m_barSize = event->size();

	this->m_clipPath.clear();
	this->m_clipPath.addRoundedRect(
		QRectF(
			this->m_borderSize,
			this->m_borderSize - 0.5,
			this->m_size.width() - this->m_borderSize - 0.5,
			this->m_size.height() - this->m_borderSize + 0.5
		),
		this->m_borderRadius, this->m_borderRadius
	);
}

void FXTrackProgressBarWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	if (event->button() == Qt::LeftButton) {
		if (this->isAudioValid()) {
			this->m_mousePressing = true;

			this->setCursor(Qt::SizeHorCursor);
			this->m_seekPreviewPosition = static_cast<double>(event->x()) / this->m_barSize.width() * this->m_audioDuration;
			this->update();
		}
	}
}

void FXTrackProgressBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	this->setCursor(QCursor());
	this->m_seekPreviewPosition = -1;

	if (this->m_mousePressing) {
		this->m_mousePressing = false;

		if (this->isAudioValid()) {
			double ratio = static_cast<double>(event->x()) / this->m_barSize.width() * this->m_audioDuration;
			emit seekTo(ratio);
		}
		else {
			this->update();
		}
	}
}

void FXTrackProgressBarWidget::mouseMoveEvent(QMouseEvent *event)
{
	QWidget::mouseMoveEvent(event);

	if (this->m_mousePressing) {
		this->m_seekPreviewPosition = static_cast<double>(event->x()) / this->m_barSize.width() * this->m_audioDuration;
		this->update();
	}
}
