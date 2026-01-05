#include <QDir>
#include <QScreen>
#include <QGuiApplication>
#include <QFileSystemModel>
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QItemSelectionModel>

#include "folderdialog.h"

namespace
{
    QString normalizePath(const QString& path)
    {
        QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(path));

#ifdef Q_OS_WIN
        normalized = normalized.toLower();
#endif

        if (!normalized.endsWith('/')) {
            normalized += '/';
        }

        return normalized;
    }

    bool isParentOf(const QString& parentNorm, const QString& childNorm)
    {
        return childNorm.startsWith(parentNorm);
    }
}

FolderDialog::FolderDialog(const QStringList& existingFolders, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("select music folder(s)");
    setModal(true);
    setMinimumSize(582, 436);

    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        const QSize windowSize(
            available.width() / 2.2,
            available.height() / 2.2);

        resize(windowSize);
        move(available.center() - rect().center());
    }

    model = new QFileSystemModel(this);
    model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    model->setRootPath(QString());

    view = new QTreeView(this);
    view->setModel(model);
    view->setSortingEnabled(true);
    view->sortByColumn(0, Qt::AscendingOrder);
    view->setExpandsOnDoubleClick(true);
    view->setUniformRowHeights(true);
    view->setRootIndex(QModelIndex());

    // 0 = name, 1 = size, 2 = type, 3 = date modified
    view->setColumnHidden(1, true);
    view->setColumnHidden(2, true);
    view->setColumnHidden(3, true);

    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    view->setStyleSheet(R"(
        /* base widgets */

        QDialog,
        QWidget {
            background-color: #1a1a1a;
            color: #e6e6e6;
        }

        /* tree view */

        QTreeView {
            background-color: #1a1a1a;
            color: #e6e6e6;
            border: 1px solid #333333;
            outline: none;
            alternate-background-color: #1a1a1a;
            show-drop-indicator: 0;
        }

        QTreeView::item {
            padding: 4px;
            border: none;
        }

        QTreeView::item:selected {
            background-color: #333333;
            color: #e6e6e6;
        }

        QTreeView::item:hover {
            background-color: #333333;
        }

        QTreeView::branch {
            background: #1a1a1a;
            border: none;
        }

        QTreeView::branch:selected {
            background: #333333;
        }

        /* header */

        QHeaderView {
            background-color: #1a1a1a;
        }

        QHeaderView::section {
            background-color: #1a1a1a;
            color: #e6e6e6;
            border: 1px solid #333333;
            padding: 4px;
        }

        QHeaderView::section:checked {
            background-color: #333333;
        }

        /* buttons */

        QPushButton {
            background-color: #1a1a1a;
            color: #e6e6e6;
            border: 1px solid #333333;
            padding: 6px 14px;
        }

        QPushButton:hover {
            background-color: #333333;
        }

        QPushButton:pressed {
            background-color: #333333;
        }

        QPushButton:disabled {
            color: #333333;
            border-color: #333333;
        }

        /* scrollbars */

        QScrollBar:vertical,
        QScrollBar:horizontal {
            background-color: #1a1a1a;
            border: 1px solid #333333;
        }

        QScrollBar::handle:vertical,
        QScrollBar::handle:horizontal {
            background-color: #333333;
            border: none;
            min-size: 20px;
        }

        QScrollBar::add-line,
        QScrollBar::sub-line,
        QScrollBar::add-page,
        QScrollBar::sub-page {
            background: none;
            border: none;
        }

        /* focus & selection cleanup */

        *:focus {
            outline: none;
            border: none;
        }
    )");

    addBtn = new QPushButton("add", this);
    continueBtn = new QPushButton("continue", this);

    auto* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(continueBtn);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(view);
    mainLayout->addLayout(buttonsLayout);

    for (const QString& raw : existingFolders) {
        folders.push_back({ raw, normalizePath(raw) });
    }

    connect(
        view->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &FolderDialog::updateButtons
    );

    connect(
        addBtn,
        &QPushButton::clicked,
        this,
        [this] {
            const QModelIndex index = view->currentIndex();

            if (!index.isValid() || !model->isDir(index)) {
                return;
            }

            const QString raw = model->filePath(index);

            QFileInfo fi(raw);

            if (!fi.isReadable()) {
                return;
            }

            const QString normalized = normalizePath(raw);

            for (int i = folders.size() - 1; i >= 0; --i) {
                if (isParentOf(normalized, folders[i].norm)) {
                    folders.removeAt(i);
                }
            }

            for (const auto& folder : folders) {
                if (isParentOf(folder.norm, normalized)) {
                    return;
                }
            }

            folders.push_back({ raw, normalized });
        });

    connect(
        continueBtn,
        &QPushButton::clicked,
        this,
        &QDialog::accept
    );

    updateButtons();
}

QStringList FolderDialog::selectedFolders() const
{
    QStringList result;

    for (const auto& folder : folders) {
        result << folder.raw;
    }

    return result;
}

void FolderDialog::updateButtons()
{
    const QModelIndex index = view->currentIndex();
    const bool validFolder = index.isValid()
        && model->isDir(index);

    addBtn->setEnabled(validFolder);
    continueBtn->setEnabled(true);
}