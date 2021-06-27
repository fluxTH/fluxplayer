#include "fxtrackanalyzer.h"

#include <QDebug>
#include <QThread>

#include "bass.h"
#include "fxhelpers.h"

#include "fxmark.h"

FXTrackAnalyzer::FXTrackAnalyzer(FXTrack* track)
	: QObject(nullptr),
	m_track(track)
{
	this->setAutoDelete(true);
}

FXTrackAnalyzer::~FXTrackAnalyzer()
{
}

void CALLBACK R128DspProc(HDSP, DWORD, void* buffer, DWORD, void* user) {
	ebur128_add_frames_float(
		reinterpret_cast<ebur128_state*>(user),
		reinterpret_cast<float*>(buffer),
		480 // temp
	);
}

void FXTrackAnalyzer::run()
{
	this->m_track->mutex.lock();
	QString filepath = this->m_track->getFileInfo()->filePath();
	this->m_track->mutex.unlock();

	FXMark mark(filepath);

	if (mark.analyzerShouldRun()) {
		wchar_t* filename = (wchar_t*)filepath.utf16();
		HSTREAM streamHandle = BASS_StreamCreateFile(
			FALSE,
			filename,
			0,
			0,
			BASS_STREAM_DECODE | BASS_UNICODE
		);

		float length = 0.05;
		bool success = false;

		float histogramLength = 0.1;
		float histogramRounds = histogramLength / length;
		quint16 count = 0;
		float levelSum = 0;

		QList<quint8> histogram;

		if (streamHandle != 0) {
			// Calculate normalization
			BASS_CHANNELINFO info;
			BASS_ChannelGetInfo(streamHandle, &info);
			BASS_ChannelSetAttribute(streamHandle, BASS_ATTRIB_GRANULE, 480);

			HDSP dspR128 = 0;
			ebur128_state* r128_state = ebur128_init(
				info.chans,
				info.freq,
				EBUR128_MODE_I
			);

			if (r128_state != nullptr) {
				dspR128 = BASS_ChannelSetDSP(streamHandle, &R128DspProc, r128_state, 0);
				if (dspR128 == 0) {
					qDebug() << "DSP R128 failed";
				}
			}
			else {
				qDebug() << "EBU-R128 failed to initialize";
			}

			// Calculate cue points
			float level;

			quint32 in = 0;
			quint32 fadeIn = 0;
			quint32 next = 0;
			quint32 fadeOut = 0;
			quint32 out = 0;

			quint32 i = 0;
			while (true) {
				if (BASS_ChannelGetLevelEx(streamHandle, &level, length, BASS_LEVEL_MONO)) {
					levelSum += level;

					float dblevel = h_float2db(level);

					if (in == 0 && dblevel >= -50) {
						in = i;
					}

					if (fadeIn == 0 && dblevel >= -30) {
						fadeIn = i;
					}

					if (in != 0 && fadeIn != 0 && dblevel >= -18) {
						next = i;
					}

					if (in != 0 && fadeIn != 0 && dblevel >= -30) {
						fadeOut = i;
					}

					if (next != 0 && dblevel >= -50) {
						out = i;
					}
				}
				else {
					if (BASS_ErrorGetCode() == BASS_ERROR_ENDED) {
						// Success
						//qDebug() << "Read ended";
						success = true;
					}
					break;
				}

				i++;
				count++;

				if (count >= histogramRounds) {
					const quint8 dynamicRange = 96;
					const quint8 maxData = std::numeric_limits<quint8>::max();
					quint8 data = h_max(h_min((h_float2db(levelSum / count) / dynamicRange * -1) * maxData, 0), maxData);
					histogram.append(data);
					count = 0;
					levelSum = 0;
				}
			}

			mark.setInMark(in * length);
			mark.setFadeInMark(fadeIn * length);
			mark.setNextMark(next * length);
			mark.setFadeOutMark(fadeOut * length);
			mark.setOutMark(out * length);

			mark.setHistogram(histogram, histogramLength * 1000);

			/*qDebug() <<
				"in:" << (double)in*length <<
				"fadein:" << (double)fadeIn*length <<
				"next:" << (double)next*length <<
				"fadeout:" << (double)fadeOut*length <<
				"out:" << (double)out*length;*/

			if (success) {
				mark.setDuration(BASS_ChannelBytes2Seconds(
					streamHandle,
					BASS_ChannelGetLength(streamHandle, BASS_POS_BYTE)
				));

				double loudness;
				if (r128_state != nullptr) {
					if (dspR128 != 0) {
						ebur128_loudness_global(r128_state, &loudness);
						mark.setLoudness(loudness);
					}

					ebur128_destroy(&r128_state);
				}

				mark.updateTrack(this->m_track);
				mark.writeMarkFile();

				emit cuePointsAnalyzed(this->m_track);
			}
			else {
				qDebug() << "Analyze failed!";
			}
		}
		else {
			qDebug() << "Analyzer failed, bass error:" << BASS_ErrorGetCode();
		}
	}
	else {
		qDebug() << "Analyzer did not run";
		mark.updateTrack(this->m_track);
		emit cuePointsAnalyzed(this->m_track);
	}
}