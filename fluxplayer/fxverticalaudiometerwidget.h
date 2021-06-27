#pragma once

#include <QWidget>
#include <QRect>

#include "fxplayer.h"


class FXVerticalAudioMeterWidget : public QWidget
{
	Q_OBJECT

public:
	FXVerticalAudioMeterWidget(QWidget *parent);
	~FXVerticalAudioMeterWidget();

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	void setLevels(const float levels[]);
	void setLevel(const short int &channel, const double &level);
	void setLevelDecibel(const short int &channel, const double &db_level);

	void setShortTermLevelDecibel(const double &db_level);
	void setNormalizationLevelDecibel(const double &db_level);

	void render();

protected:
	void showEvent(QShowEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	const short int m_warnLevel = -18;
	const short int m_alertLevel = -6;
	const quint16 m_attackTimeMs = 5;
	const quint16 m_releaseTimeMs = 50;

	const QColor m_borderColor = QColor(40, 40, 40);
	const QColor m_levelMarkerColor = Qt::darkGray;
	const QColor m_normalLevelColor = QColor(54, 239, 68);
	const QColor m_warnLevelColor = QColor(244, 208, 68);
	const QColor m_alertLevelColor = QColor(245, 69, 68);
	const QColor m_peakHoldColor = Qt::yellow;

	float m_decayRate = 8; // dB / sec
	float m_shortTermDecayRate = 12; // dB / sec

	double m_volumeRatio = 1.0;
	double m_volumeClickedRatio = 0.0;
	int m_volumeClickedY = 0;
	bool m_editingVolume = false;

	QSize m_size;
	QRectF m_barRect;
	float m_barRange = 40; // dB

	unsigned short int m_numChannels = 2; // change later

	float m_shortTermLevel = -1000;
	QList<float> m_dbLevels;
	QList<float> m_peakHold;

	float m_normalizationLevel;

	QList<qint64> m_lastLevelTime;
	qint64 m_lastShortTermDecayTime;
	QList<qint64> m_lastDecayTime;
	QList<qint64> m_lastPeakHold;

signals:
	void volumeRatioChanged(double);

public slots:
	void handleNormalizationChanged(double);
	void handleResetVolumeClick();
};
