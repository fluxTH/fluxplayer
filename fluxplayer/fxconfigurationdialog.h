#pragma once

#include <QDialog>
#include "ui_fxconfigurationdialog.h"

class FXConfigurationDialog : public QDialog
{
	Q_OBJECT

public:
	FXConfigurationDialog(QWidget *parent = Q_NULLPTR);
	~FXConfigurationDialog();

private:
	Ui::FXConfigurationDialog ui;
};
