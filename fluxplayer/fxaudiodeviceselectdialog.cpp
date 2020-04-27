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

	if (parent == nullptr)
		this->setWindowFlag(Qt::WindowStaysOnTopHint);
	
	this->populateSampleRateCombobox();
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

void FXAudioDeviceSelectDialog::populateSampleRateCombobox()
{
	int sampleRates[7] = { 22050, 32000, 44100, 48000, 96000, 176400, 192000 };
	for (short int i = 0; i < (sizeof(sampleRates) / sizeof(*sampleRates)); i++)
		ui.comboBox_sampleRate->insertItem(i, QString("%1 Hz").arg(sampleRates[i]), sampleRates[i]);
	ui.comboBox_sampleRate->setCurrentIndex(3);
}

void FXAudioDeviceSelectDialog::populateDeviceCombobox(const FXAudioDriver &driver)
{
	switch (driver) {
		case FXAudioDriver::WASAPI:
			this->populateWASAPIDeviceCombobox();
			break;

		case FXAudioDriver::DSOUND:
			this->populateDSOUNDDeviceCombobox();
			break;
	}
}

void FXAudioDeviceSelectDialog::populateWASAPIDeviceCombobox()
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

void FXAudioDeviceSelectDialog::populateDSOUNDDeviceCombobox()
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
		ui.checkBox_exclusiveMode->setEnabled(true);
		this->populateWASAPIDeviceCombobox();
	} else {
		ui.checkBox_exclusiveMode->setEnabled(false);
		ui.checkBox_exclusiveMode->setCheckState(Qt::Unchecked);
		this->populateDSOUNDDeviceCombobox();
	}
}

void FXAudioDeviceSelectDialog::handleRefreshDevice()
{
	this->populateDeviceCombobox(this->getDriver());
}