#pragma once

#include <QTableView>

class FXTableView : public QTableView
{
	Q_OBJECT

public:
	FXTableView(QWidget *parent);
	~FXTableView();

	void setModel(QAbstractItemModel *model) override;
	void keyPressEvent(QKeyEvent *event) override;
	QStyleOptionViewItem viewOptions() const override;

private slots:
	void handleDoubleClickedAt(const QModelIndex&);
};
