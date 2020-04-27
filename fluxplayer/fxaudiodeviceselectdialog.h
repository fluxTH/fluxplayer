#pragma once

#include <QDialog>
#include "ui_fxaudiodeviceselectdialog.h"

#include "fxhelpers.h"

#include "bass.h"
#include "basswasapi.h"

class FXAudioDeviceSelectDialog : public QDialog
{
	Q_OBJECT

public:
	FXAudioDeviceSelectDialog(QWidget *parent = Q_NULLPTR);
	~FXAudioDeviceSelectDialog();

	int getSampleRate();
	int getOutputDeviceIndex();
	short int getChannelLayout();
	FXAudioDriver getDriver();

private:
	Ui::FXAudioDeviceSelectDialog ui;

	void populateSampleRateCombobox();
	void populateDeviceCombobox(const FXAudioDriver &driver);
	void populateWASAPIDeviceCombobox();
	void populateDSOUNDDeviceCombobox();

private slots:
	void handleDriverChange(int);
	void handleRefreshDevice();
};
