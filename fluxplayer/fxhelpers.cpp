#include "fxhelpers.h"

QString FXHelpers::formatSecToTime(const float &total_sec, const bool &show_decimals)
{
	bool neg = total_sec < 0;
	unsigned int sec, min, hr, t_sec, t_min, t_hr;

	sec = abs(total_sec);

	float decimal;
	if (show_decimals)
		decimal = abs(total_sec) - sec;

	min = sec / 60;
	hr = min / 60;

	t_sec = sec % 60;
	t_min = min % 60;
	t_hr = hr % 60;

	QString result;
	if (t_hr == 0)
		result = QString("%1:%2%3")
					.arg(t_min)
					.arg(t_sec, 2, 10, QChar('0'))
					.arg(show_decimals ? QString(".%1").arg(floor(decimal * 10), 1, 'f', 0) : "");
	else
		result = QString("%1:%2:%3%4")
					.arg(t_hr)
					.arg(t_min, 2, 10, QChar('0'))
					.arg(t_sec, 2, 10, QChar('0'))
					.arg(show_decimals ? QString(".%1").arg(floor(decimal * 10), 1, 'f', 0) : "");

	return neg ? "-" + result : result;
}