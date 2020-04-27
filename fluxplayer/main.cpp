#include "fluxplayer.h"
#include <QtWidgets/QApplication>

#include <QStyleFactory>
#include <QDebug>

#ifdef QT_NO_DEBUG 
#define QT_NO_DEBUG_OUTPUT
#endif

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("fluxdev");
	QCoreApplication::setApplicationName("fluxplayer");
	QCoreApplication::setApplicationVersion("1.0");

	QApplication app(argc, argv);
	app.setAttribute(Qt::AA_UseHighDpiPixmaps);
	//app.setStyle("fusion");

	FluxPlayer main_window;
	if (main_window.initialize()) {
		main_window.show();
		return app.exec();
	}
	
	return 0;
}
