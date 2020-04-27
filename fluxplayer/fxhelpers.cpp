#include "fxhelpers.h"

QString FXHelpers::formatSecToTime(const int &total_sec)
{
	if (total_sec <= 0)
		return "0:00";

	short int min, hr, t_sec, t_min, t_hr;

	min = total_sec / 60;
	hr = min / 60;

	t_sec = total_sec % 60;
	t_min = min % 60;
	t_hr = hr % 60;

	if (t_hr == 0)
		return QString("%1:%2").arg(t_min).arg(t_sec, 2, 10, QChar('0'));

	return QString("%1:%2:%3").arg(t_hr).arg(t_min, 2, 10, QChar('0')).arg(t_sec, 2, 10, QChar('0'));
}