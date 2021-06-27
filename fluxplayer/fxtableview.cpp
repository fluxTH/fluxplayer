#include "fxtableview.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QDebug>

#include "fxplaylistmodel.h"


FXTableView::FXTableView(QWidget *parent)
	: QTableView(parent)
{
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	//this->setSelectionMode(QAbstractItemView::SingleSelection);
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	this->setAlternatingRowColors(true);
	//this->setFocusPolicy(Qt::NoFocus);
	//this->setStyleSheet("QTableView { outline: 0; }");

	/*QPalette palette = this->palette();
	palette.setBrush(QPalette::Highlight, QBrush(QColor(200, 255, 255)));
	palette.setBrush(QPalette::HighlightedText, QBrush(Qt::black));
	this->setPalette(palette);*/

	this->setDragEnabled(true);
	this->setDropIndicatorShown(true);
	this->setDragDropMode(QAbstractItemView::DragDrop);
	this->setDragDropOverwriteMode(false);
	this->setAcceptDrops(true);

	QObject::connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(handleDoubleClickedAt(const QModelIndex&)));
}

FXTableView::~FXTableView()
{
}

void FXTableView::setModel(QAbstractItemModel *model)
{
	QTableView::setModel(model);

	QHeaderView *horizontalHeader = this->horizontalHeader();
	horizontalHeader->setHighlightSections(false);
	horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);
	horizontalHeader->setSectionResizeMode(2, QHeaderView::Stretch);
	horizontalHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);

	QHeaderView *verticalHeader = this->verticalHeader();
	verticalHeader->setDefaultSectionSize(25);
	verticalHeader->sectionResizeMode(QHeaderView::Fixed);
	verticalHeader->hide();
}

QStyleOptionViewItem FXTableView::viewOptions() const
{
	QStyleOptionViewItem option = QTableView::viewOptions();
	option.decorationAlignment = Qt::AlignHCenter | Qt::AlignBottom;
	option.decorationPosition = QStyleOptionViewItem::Top;
	return option;
}

void FXTableView::keyPressEvent(QKeyEvent *event)
{
	QTableView::keyPressEvent(event);

	QModelIndexList selectedIndexes = this->selectedIndexes();
	switch (event->key()) {
		case Qt::Key_Return:
		case Qt::Key_Enter:
			if (selectedIndexes.count() == this->model()->columnCount()) {
				dynamic_cast<FXPlaylistModel*>(this->model())->playAt(selectedIndexes.first().row());
				this->clearSelection();
			}
			break;

		case Qt::Key_Escape:
			this->clearSelection();
			break;

		case Qt::Key_Insert:
			qDebug() << "Insert";
			break;

		case Qt::Key_Delete:
			if (!selectedIndexes.isEmpty()) {
				FXPlaylistModel *model = dynamic_cast<FXPlaylistModel*>(this->model());

				QVector<unsigned int> toDelete;
				foreach (QModelIndex selected, selectedIndexes) {
					const unsigned int row = selected.row();
					if (toDelete.indexOf(row) == -1)
						toDelete.append(row);
				}

				/*
				QVector<unsigned int> toDelete;
				for (unsigned int i = 0; i < selectedIndexes.count() / model->columnCount(); i++) {
					unsigned int index = i * model->columnCount();
					qDebug() << i << index;
					toDelete.append(selectedIndexes.value(i).row());
				}
				qDebug() << toDelete;
				*/

				model->deleteRange(toDelete);
				this->clearSelection();
			}
			break;
	}
}

// Slots
void FXTableView::handleDoubleClickedAt(const QModelIndex &index) {
	dynamic_cast<FXPlaylistModel*>(this->model())->playAt(index.row());
	this->clearSelection();
}