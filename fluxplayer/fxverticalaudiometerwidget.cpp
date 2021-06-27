#include "fxverticalaudiometerwidget.h"

#include <QResizeEvent>
#include <QPainter>
#include <QDateTime>
#include <QBitmap>
#include <QLinearGradient>
#include <QTimer>

#include <QDebug>

#include "fxhelpers.h"


FXVerticalAudioMeterWidget::FXVerticalAudioMeterWidget(QWidget *parent)
	: QWidget(parent),
	m_shortTermLevel(-1000),
	m_normalizationLevel(NAN),
	m_lastShortTermDecayTime(0)
{
	for (unsigned short ch = 0; ch < this->m_numChannels; ch++) {
		this->m_dbLevels.append(-1000);
		this->m_lastLevelTime.append(0);

		this->m_lastDecayTime.append(0);

		this->m_peakHold.append(-1000);
		this->m_lastPeakHold.append(0);
	}
}

FXVerticalAudioMeterWidget::~FXVerticalAudioMeterWidget()
{
}

QSize FXVerticalAudioMeterWidget::minimumSizeHint() const
{
	return QSize(50, 100);
}

QSize FXVerticalAudioMeterWidget::sizeHint() const
{
	return QSize(60, 200);
}

void FXVerticalAudioMeterWidget::setLevels(const float levels[])
{
	for (unsigned short ch = 0; ch < this->m_numChannels; ch++) {
		this->setLevel(ch, levels[ch]);
	}
}

void FXVerticalAudioMeterWidget::setLevel(const short int &ch, const double &level) 
{
	double db_level = h_float2db(level);

	if (db_level > this->m_dbLevels[ch]) {

		this->m_dbLevels[ch] = db_level;
		this->m_lastLevelTime[ch] = QDateTime::currentMSecsSinceEpoch();

		qint64 currentTime = QDateTime::currentSecsSinceEpoch();
		if (db_level > this->m_peakHold[ch] || currentTime - this->m_lastPeakHold[ch] >= 1) {
			this->m_peakHold[ch] = db_level;
			this->m_lastPeakHold[ch] = currentTime;
		}
	}
}

void FXVerticalAudioMeterWidget::setLevelDecibel(const short int &channel, const double &db_level)
{
	this->m_dbLevels[channel] = db_level;
}

void FXVerticalAudioMeterWidget::setShortTermLevelDecibel(const double &db_level)
{
	if (db_level > this->m_shortTermLevel) {
		this->m_shortTermLevel = db_level;
	}
}

void FXVerticalAudioMeterWidget::setNormalizationLevelDecibel(const double &db_level)
{
	this->m_normalizationLevel = db_level;
}

void FXVerticalAudioMeterWidget::render()
{
	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

	// Decay levels
	for (quint8 ch = 0; ch < this->m_numChannels; ch++) {
		//qint64 diff = h_min(currentTime - this->m_lastLevelTime[ch] - this->m_releaseTimeMs, 0);

		//if (this->m_lastLevelTime[ch] != 0 && this->m_dbLevels[ch] > this->m_barRange * -1) {
		//	float decay = diff * 0.001 * this->m_decayRate;
		//	qDebug() << diff << decay;
		//	this->m_dbLevels[ch] -= decay;
		//}
		if (currentTime - this->m_lastLevelTime[ch] < this->m_releaseTimeMs) {
			this->m_lastDecayTime[ch] = currentTime;
			continue;
		}

		if (this->m_lastDecayTime[ch] != 0 && this->m_dbLevels[ch] > this->m_barRange * -1) {
			// Decay
			float decay = this->m_decayRate / 1000 * h_min(currentTime - this->m_lastDecayTime[ch], 0);
			this->m_dbLevels[ch] -= decay;

			this->m_lastDecayTime[ch] = currentTime;
		}
	}

	// Decay short term
	if (this->m_lastShortTermDecayTime != 0 && this->m_shortTermLevel > this->m_barRange * -1) {
		// Decay
		float decay = this->m_shortTermDecayRate / 1000 * (currentTime - this->m_lastShortTermDecayTime);
		this->m_shortTermLevel -= decay;
	}
	this->m_lastShortTermDecayTime = currentTime;

	this->update();
}

void FXVerticalAudioMeterWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
}

void FXVerticalAudioMeterWidget::paintEvent(QPaintEvent *event)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	//p.setRenderHint(QPainter::TextAntialiasing);
	
	quint32 barX, barY, barWidth, barHeight;
	barX = this->m_barRect.x();
	barY = this->m_barRect.y();
	barWidth = this->m_barRect.width();
	barHeight = this->m_barRect.height();

	QBrush bgBrush(QColor(50, 50, 50));
	bgBrush.setStyle(Qt::Dense4Pattern);

	// Fill bg
	p.fillRect(this->m_barRect, QBrush(Qt::black));

	// Fill peak indicator bg
	QRect peakIndicatorRect(
		barX,
		barY - 10,
		barWidth,
		10
	);
	p.fillRect(peakIndicatorRect, QBrush(Qt::black));

	p.setPen(this->m_borderColor);
	p.drawRect(peakIndicatorRect);

	p.fillRect(this->m_barRect, bgBrush);

	if (!isnan(this->m_normalizationLevel)) {
		//QBrush normBrush(QColor(60, 60, 60));
		//normBrush.setStyle(Qt::Dense5Pattern);
		double normY = barHeight * (this->m_normalizationLevel * -1) / this->m_barRange;

		p.fillRect(
			QRect(
				barX,
				barY,
				barWidth,
				normY
			),
			QBrush(Qt::black)
		);

		p.setPen(Qt::NoPen);
		QPainterPath pathL;
		pathL.moveTo(barX, barY + normY + 4);
		pathL.lineTo(barX, barY + normY - 4);
		pathL.lineTo(barX + 4, barY + normY);
		pathL.lineTo(barX, barY + normY + 4);

		QPainterPath pathR;
		pathR.moveTo(barX + barWidth, barY + normY + 4);
		pathR.lineTo(barX + barWidth, barY + normY - 4);
		pathR.lineTo(barX + barWidth - 4, barY + normY);
		pathR.lineTo(barX + barWidth, barY + normY + 4);

		p.fillPath(pathL, QBrush(Qt::darkGray));
		p.fillPath(pathR, QBrush(Qt::darkGray));
	}

	double channelWidth = barWidth / this->m_numChannels;
	for (unsigned short int ch = 0; ch < this->m_numChannels; ch++) {
		if (isnan(this->m_dbLevels[ch]))
			continue;

		// Draw levels
		double normalLevelHeight = h_filterPositive(this->m_dbLevels[ch] + this->m_barRange) / this->m_barRange * barHeight;

		p.fillRect(
			QRectF(
				barX + ch * channelWidth,
				barY + barHeight - normalLevelHeight,
				channelWidth,
				normalLevelHeight
			), 
			QBrush(this->m_normalLevelColor)
		);

		if (this->m_dbLevels[ch] > this->m_warnLevel) {
			double warnEndY = (-1 * this->m_warnLevel) / this->m_barRange * barHeight;
			double warnRange = this->m_barRange + this->m_warnLevel;
			double warnLevelHeight = barHeight * h_filterPositive(h_min(this->m_dbLevels[ch], this->m_warnLevel) + this->m_barRange - warnRange) / this->m_barRange;
			p.fillRect(
				QRectF(
					barX + ch * channelWidth,
					barY + warnEndY - warnLevelHeight,
					channelWidth,
					warnLevelHeight
				),
				QBrush(this->m_warnLevelColor)
			);
		}

		if (this->m_dbLevels[ch] > this->m_alertLevel) {
			double alertEndY = (-1 * this->m_alertLevel) / this->m_barRange * barHeight;
			double alertRange = this->m_barRange + this->m_alertLevel;
			double alertLevelHeight = barHeight * h_filterPositive(h_min(this->m_dbLevels[ch], this->m_alertLevel) + this->m_barRange - alertRange) / this->m_barRange;
			p.fillRect(
				QRectF(
					barX + ch * channelWidth,
					barY + alertEndY - alertLevelHeight,
					channelWidth,
					alertLevelHeight
				),
				QBrush(this->m_alertLevelColor)
			);
		}

		if (ch != 0) {
			// Draw peak channel separator
			p.setPen(this->m_borderColor);
			p.drawLine(
				barX + ch * channelWidth,
				barY - 10,
				barX + ch * channelWidth,
				barY
			);

			// Draw channel separator
			p.setPen(Qt::black);
			p.drawLine(
				barX + ch * channelWidth,
				barY + 1,
				barX + ch * channelWidth,
				barY + barHeight - 1
			);
		}

		// Draw normalization text
		if (!isnan(this->m_normalizationLevel)) {
			double normY = barY + barHeight * (this->m_normalizationLevel * -1) / this->m_barRange;

			// check with height
			if (normY - barY >= 15) {
				p.setPen(this->m_levelMarkerColor);
				p.setFont(QFont("Arial", 5));
				p.drawText(
					QRectF(
						barX,
						normY - 15,
						barWidth,
						15
					),
					Qt::AlignCenter | Qt::TextDontClip,
					QString("%1 dB").arg(this->m_normalizationLevel, 0, 'f', 1)
				);
			}
		}

		// Draw peak hold
		double peakY = barY + barHeight - (barHeight * (h_filterPositive(this->m_peakHold[ch] + this->m_barRange) / this->m_barRange));
		p.setPen(this->m_peakHoldColor);
		p.drawLine(
			barX + ch * channelWidth + 1,
			peakY,
			barX + (ch + 1) * channelWidth - 1,
			peakY
		);

	}

	// Draw shortterm level
	double stLevelRatio = h_filterPositive(this->m_shortTermLevel + this->m_barRange) / this->m_barRange;
	p.fillRect(
		QRect(
			barX + 3,
			barY + barHeight - (barHeight * stLevelRatio) - 2,
			barWidth - 6,
			4
		),
		QBrush(Qt::blue)
	);

	// Draw markers
	p.setFont(QFont("Arial", 5));
	if (this->m_editingVolume) {
		p.setPen(Qt::white);
		for (unsigned short vol = 10; vol <= 100; vol += 10) {
			p.drawText(
				QRectF(
					this->m_barRect.x() + this->m_barRect.width() + 7,
					this->m_barRect.y() + (this->m_barRect.height() * ((100 - vol) / 100.0)) - 4,
					0,
					8
				),
				Qt::AlignLeft | Qt::AlignHCenter | Qt::TextDontClip,
				QString("%1").arg(vol)
			);
		}
	}
	else {
		p.setPen(Qt::darkGray);
		for (short int lv = 0; lv >= this->m_barRange * -1; lv -= 3) {
			p.drawText(
				QRectF(
					this->m_barRect.x() + this->m_barRect.width() + 7,
					this->m_barRect.y() + (this->m_barRect.height() * (lv * -1 / this->m_barRange)) - 4,
					0,
					8
				),
				Qt::AlignLeft | Qt::AlignHCenter | Qt::TextDontClip,
				lv % 6 == 0 ? QString("%1").arg(lv) : "-  "
			);
		}

		// Draw shortterm triangle
		double stTargetY = barY + 14 / this->m_barRange * barHeight;

		QPainterPath stPath;
		stPath.moveTo(2 + barX + barWidth, stTargetY);
		stPath.lineTo(2 + barX + barWidth + 4, stTargetY - 4);
		stPath.lineTo(2 + barX + barWidth + 4, stTargetY + 4);
		stPath.lineTo(2 + barX + barWidth, stTargetY);

		p.setPen(Qt::white);
		p.drawPath(stPath);
		p.fillPath(stPath, QBrush(Qt::blue));
	}

	// Draw Volume
	QLinearGradient volGradient(
		barX,
		barY,
		barX - 9,
		barY
	);
	volGradient.setColorAt(0, QColor(0, 160, 255, 150));
	volGradient.setColorAt(1, QColor(0, 160, 255, 0));

	double volHeight = barHeight * this->m_volumeRatio;
	p.fillRect(QRectF(
		barX - 9,
		barY + barHeight - volHeight,
		9,
		volHeight
	), volGradient);

	if (this->m_editingVolume) {
		p.fillRect(this->m_barRect, QColor(0, 0, 0, 150));
		QRectF volRect(
			barX,
			barY + barHeight - volHeight,
			barWidth,
			volHeight
		);

		p.fillRect(
			volRect,
			QColor(0, 160, 255, 200)
		);

		p.setPen(Qt::white);
		p.setFont(QFont("Calibri", 10, QFont::Bold));
		p.drawText(
			QRectF(
				volRect.x(),
				volRect.y() - (volRect.y() > barX + barHeight - 20 ? 20 : 0),
				volRect.width(),
				20
			),
			Qt::AlignCenter,
			QString("%1%").arg(this->m_volumeRatio * 100, 0, 'f', 0)
		);
	}

	// Draw border
	p.setPen(QPen(this->m_borderColor, 1));
	p.drawRect(this->m_barRect);

}

void FXVerticalAudioMeterWidget::resizeEvent(QResizeEvent *event)
{
	QSize size = event->size();
	unsigned short int padX = 10;
	unsigned short int padY = 10;

	this->m_barRect.setX(padX);
	this->m_barRect.setY(padY);
	this->m_barRect.setWidth(size.width() - 12 - padX);
	this->m_barRect.setHeight(size.height() - padY);
}

void FXVerticalAudioMeterWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	switch (event->button()) {
		case Qt::LeftButton:
			if (this->m_barRect.contains(event->pos())) {
				this->m_editingVolume = true;
				this->m_volumeClickedY = event->y();
				this->m_volumeClickedRatio = h_max(h_min(1.0 - this->m_volumeRatio, 0.0), 1.0);
			}
			break;

		case Qt::RightButton:
			if (this->m_barRect.contains(event->pos())) {
				double yRatio = (event->y() - this->m_barRect.y()) / this->m_barRect.height();

				this->m_volumeRatio = h_max(h_min(1.0 - yRatio, 0.0), 1.0);
				emit volumeRatioChanged(this->m_volumeRatio);

				QTimer::singleShot(400, this, SLOT(handleResetVolumeClick()));
				this->m_editingVolume = true;
			}
			break;
	}
}

void FXVerticalAudioMeterWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	if (this->m_editingVolume == true && this->m_volumeClickedY != 0) {
		this->handleResetVolumeClick();
	}
}

void FXVerticalAudioMeterWidget::mouseMoveEvent(QMouseEvent *event)
{
	QWidget::mouseMoveEvent(event);

	if (this->m_editingVolume == true && this->m_volumeClickedY != 0) {
		int yDiff = event->y() - this->m_volumeClickedY;

		float yRatio = ((this->m_barRect.height() * this->m_volumeClickedRatio) + yDiff) / this->m_barRect.height();
		this->m_volumeRatio = round(h_max(h_min(1.0 - yRatio, 0.0), 1.0) * 100) / 100;

		emit volumeRatioChanged(this->m_volumeRatio);
	}
}

void FXVerticalAudioMeterWidget::wheelEvent(QWheelEvent *event)
{
	QWidget::wheelEvent(event);
}

// Slots
void FXVerticalAudioMeterWidget::handleNormalizationChanged(double norm_db) {
	this->m_normalizationLevel = norm_db;
	this->update();
}

void FXVerticalAudioMeterWidget::handleResetVolumeClick()
{
	this->m_editingVolume = false;
	this->m_volumeClickedY = 0;
	this->m_volumeClickedRatio = 0.0;
}