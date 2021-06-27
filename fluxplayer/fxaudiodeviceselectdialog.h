#pragma once

#include <QDialog>
#include "ui_fxaudiodeviceselectdialog.h"

#include "fxhelpers.h"

#include "bass.h"
#include "basswasapi.h"
#include "bassasio.h"

class FXAudioDeviceSelectDialog : public QDialog
{
	Q_OBJECT

public:
	FXAudioDeviceSelectDialog(QWidget *parent = Q_NULLPTR);
	~FXAudioDeviceSelectDialog();

	int getSampleRate();
	int getOutputDeviceIndex();
	short int getChannelLayout();
	bool getWASAPIExclusiveMode();
	FXAudioDriver getDriver();

private:
	Ui::FXAudioDeviceSelectDialog ui;

	void populateSampleRateComboBox();
	void populateDeviceComboBox(const FXAudioDriver &driver);
	void populateWASAPIDeviceComboBox();
	void populateASIODeviceComboBox();
	void populateDSOUNDDeviceComboBox();

private slots:
	void handleDriverChange(int);
	void handleDeviceChange(QString);
	void handleRefreshDevice();
};
