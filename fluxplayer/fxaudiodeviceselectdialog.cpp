#include "fxaudiodeviceselectdialog.h"

#include <QLayout>
#include <QDebug>


FXAudioDeviceSelectDialog::FXAudioDeviceSelectDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->layout()->setSizeConstraint(QLayout::SetFixedSize);

	QObject::connect(ui.driverButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(handleDriverChange(int)));
	QObject::connect(ui.btn_refreshDevice, SIGNAL(clicked()), this, SLOT(handleRefreshDevice()));
	QObject::connect(ui.comboBox_device, SIGNAL(currentIndexChanged(QString)), this, SLOT(handleDeviceChange(QString)));

	if (parent == nullptr)
		this->setWindowFlag(Qt::WindowStaysOnTopHint);
	
	this->populateSampleRateComboBox();
	this->handleDriverChange(0);
	
}

FXAudioDeviceSelectDialog::~FXAudioDeviceSelectDialog()
{
}

int FXAudioDeviceSelectDialog::getSampleRate()
{
	return ui.comboBox_sampleRate->currentData().toInt();
}

int FXAudioDeviceSelectDialog::getOutputDeviceIndex()
{
	return ui.comboBox_device->currentData().toInt();
}

short int FXAudioDeviceSelectDialog::getChannelLayout()
{
	if (ui.radioButton_channelStereo->isChecked())
		return 2;

	if (ui.radioButton_channelMono->isChecked())
		return 1;

	return -1;
}

bool FXAudioDeviceSelectDialog::getWASAPIExclusiveMode()
{
	return ui.checkBox_exclusiveMode->isChecked();
}

FXAudioDriver FXAudioDeviceSelectDialog::getDriver()
{
	if (ui.radioButton_driverWASAPI->isChecked())
		return FXAudioDriver::WASAPI;

	if (ui.radioButton_driverASIO->isChecked())
		return FXAudioDriver::ASIO;

	if (ui.radioButton_driverDSOUND->isChecked())
		return FXAudioDriver::DSOUND;

	return FXAudioDriver::Invalid;
}

void FXAudioDeviceSelectDialog::populateSampleRateComboBox()
{
	int sampleRates[7] = { 22050, 32000, 44100, 48000, 96000, 176400, 192000 };
	for (short int i = 0; i < (sizeof(sampleRates) / sizeof(*sampleRates)); i++)
		ui.comboBox_sampleRate->insertItem(i, QString("%1 Hz").arg(sampleRates[i]), sampleRates[i]);
	ui.comboBox_sampleRate->setEnabled(true);
	ui.comboBox_sampleRate->setCurrentIndex(3);
}

void FXAudioDeviceSelectDialog::populateDeviceComboBox(const FXAudioDriver &driver)
{
	switch (driver) {
		case FXAudioDriver::WASAPI:
			this->populateWASAPIDeviceComboBox();
			break;

		case FXAudioDriver::ASIO:
			this->populateASIODeviceComboBox();
			break;

		case FXAudioDriver::DSOUND:
			this->populateDSOUNDDeviceComboBox();
			break;
	}
}

void FXAudioDeviceSelectDialog::populateWASAPIDeviceComboBox()
{
	this->setCursor(QCursor(Qt::WaitCursor));

	ui.comboBox_device->clear();

	ui.comboBox_device->addItem("( Default WASAPI Device )", -1);
	ui.comboBox_device->insertSeparator(1);

	BASS_WASAPI_DEVICEINFO info;
	short int a = 1;
	for (; BASS_WASAPI_GetDeviceInfo(a, &info); a++)
		if (!(info.flags & BASS_DEVICE_INPUT) && (info.flags & BASS_DEVICE_ENABLED))
			ui.comboBox_device->addItem(QString(info.name), a);

	if (a == 1 && BASS_ErrorGetCode() == BASS_ERROR_WASAPI) {
		// WASAPI not availiable
		ui.radioButton_driverWASAPI->setEnabled(false);
		ui.radioButton_driverDSOUND->setChecked(true);
		this->handleDriverChange(0);
	}

	this->setCursor(QCursor());
}

void FXAudioDeviceSelectDialog::populateASIODeviceComboBox()
{
	this->setCursor(QCursor(Qt::WaitCursor));

	ui.comboBox_device->clear();

	BASS_ASIO_DEVICEINFO info;
	int a = 0;
	for (; BASS_ASIO_GetDeviceInfo(a, &info); a++) {
		ui.comboBox_device->addItem(QString(info.name), a);
	}

	/*if (a == 1 && BASS_ErrorGetCode() == BASS_ERROR_WASAPI) {
		// WASAPI not availiable
		ui.radioButton_driverWASAPI->setEnabled(false);
		ui.radioButton_driverDSOUND->setChecked(true);
		this->handleDriverChange(0);
	}*/

	this->setCursor(QCursor());
}

void FXAudioDeviceSelectDialog::populateDSOUNDDeviceComboBox()
{
	this->setCursor(QCursor(Qt::WaitCursor));

	ui.comboBox_device->clear();

	ui.comboBox_device->addItem("( Default DirectSound Device )", -1);
	ui.comboBox_device->insertSeparator(1);

	BASS_DEVICEINFO info;
	for (short int a = 1; BASS_GetDeviceInfo(a, &info); a++)
		if (info.flags & BASS_DEVICE_ENABLED) // device is enabled
			ui.comboBox_device->addItem(QString(info.name), a);

	this->setCursor(QCursor());

}


// Slots
void FXAudioDeviceSelectDialog::handleDriverChange(int) {
	FXAudioDriver currentDriver = this->getDriver();
	if (currentDriver == FXAudioDriver::WASAPI) {
		this->populateWASAPIDeviceComboBox();

		ui.label_driverOptions->setHidden(false);
		ui.label_driverOptions->setText("WASAPI:");

		ui.options_WASAPI->setHidden(false);
		ui.options_ASIO->setHidden(true);
	}
	else if (currentDriver == FXAudioDriver::ASIO) {
		this->populateASIODeviceComboBox();

		ui.label_driverOptions->setHidden(false);
		ui.label_driverOptions->setText("ASIO:");

		ui.options_WASAPI->setHidden(true);
		ui.options_ASIO->setHidden(false);
	}
	else {
		this->populateDSOUNDDeviceComboBox();

		ui.label_driverOptions->setHidden(true);
		ui.options_WASAPI->setHidden(true);
		ui.options_ASIO->setHidden(true);
	}
}

void FXAudioDeviceSelectDialog::handleDeviceChange(QString deviceName)
{
	FXAudioDriver currentDriver = this->getDriver();
	if (currentDriver == FXAudioDriver::WASAPI && !this->getWASAPIExclusiveMode() && this->getOutputDeviceIndex() != -1) {
		this->setCursor(QCursor(Qt::WaitCursor));

		ui.comboBox_sampleRate->clear();

		BASS_WASAPI_DEVICEINFO info;
		short int a = 1;
		for (; BASS_WASAPI_GetDeviceInfo(a, &info); a++) {
			if (!(info.flags & BASS_DEVICE_INPUT) && (info.flags & BASS_DEVICE_ENABLED)) {
				if (info.name == deviceName) {
					ui.comboBox_sampleRate->addItem(QString("%1 Hz").arg(info.mixfreq), static_cast<int>(info.mixfreq));
					break;
				}
			}
		}

		this->setCursor(QCursor());
		ui.comboBox_sampleRate->setEnabled(false);
	}
	else {
		ui.comboBox_sampleRate->clear();
		this->populateSampleRateComboBox();
	}
}

void FXAudioDeviceSelectDialog::handleRefreshDevice()
{
	this->populateDeviceComboBox(this->getDriver());
}