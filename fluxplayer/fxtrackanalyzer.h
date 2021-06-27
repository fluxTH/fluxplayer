#pragma once

#include <QObject>
#include <QRunnable>

#include "fxtrack.h"
#include "ebur128.h"


class FXTrackAnalyzer : public QObject, public QRunnable
{
	Q_OBJECT

public:
	FXTrackAnalyzer(FXTrack *track);
	~FXTrackAnalyzer();

	void run();

private:
	FXTrack *m_track;

signals:
	void cuePointsAnalyzed(FXTrack*);
	void normalizationAnalyzed(FXTrack*);
};
