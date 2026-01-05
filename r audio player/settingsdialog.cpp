#include "settingsdialog.h"
#include "folderdialog.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

#include <QScreen>
#include <QGuiApplication>

SettingsDialog::SettingsDialog(const QStringList& musicFolders, bool /* autoplay */, const QStringList& trackFormat, QWidget* parent) : QDialog(parent)
{
    setWindowTitle("settings");
    setMinimumSize(582, 436);

    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        const QSize windowSize(
            available.width() / 2.2,
            available.height() / 2.2);

        resize(windowSize);
        move(available.center() - rect().center());
    }

    foldersList = new QListWidget(this);
    foldersList->addItems(musicFolders);

    addButton = new QPushButton("add folder(s)", this);
    removeButton = new QPushButton("remove selected", this);
    rescanButton = new QPushButton("rescan", this);

    connect(addButton, &QPushButton::clicked, this,
        &SettingsDialog::addFolder);
    connect(removeButton, &QPushButton::clicked, this,
        &SettingsDialog::removeSelectedFolder);
    connect(rescanButton, &QPushButton::clicked, this,
        &SettingsDialog::rescanRequested);

    auto folderButtons = new QHBoxLayout;
    folderButtons->addWidget(addButton);
    folderButtons->addWidget(removeButton);
    folderButtons->addWidget(rescanButton);

    coverCheck = new QCheckBox("cover", this);
    artistCheck = new QCheckBox("artist", this);
    albumCheck = new QCheckBox("album", this);
    trackCheck = new QCheckBox("track", this);

    coverCheck->setChecked(trackFormat.contains("cover"));
    artistCheck->setChecked(trackFormat.contains("artist"));
    albumCheck->setChecked(trackFormat.contains("album"));
    trackCheck->setChecked(trackFormat.contains("track"));

    auto formatLayout = new QHBoxLayout;
    formatLayout->addStretch();
    formatLayout->addWidget(coverCheck);
    formatLayout->addWidget(artistCheck);
    formatLayout->addWidget(albumCheck);
    formatLayout->addWidget(trackCheck);
    formatLayout->addStretch();

    auto buttons = new QDialogButtonBox(this);

    auto saveBtn = buttons->addButton("save", QDialogButtonBox::AcceptRole);
    auto cancelBtn = buttons->addButton("cancel", QDialogButtonBox::RejectRole);

    connect(saveBtn, &QPushButton::clicked, this,
        &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this,
        &QDialog::reject);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("music folders:", this));
    layout->addWidget(foldersList);
    layout->addLayout(folderButtons);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("track display format:", this));
    layout->addLayout(formatLayout);
    layout->addStretch();
    layout->addWidget(buttons);
}

void SettingsDialog::addFolder()
{
    FolderDialog dlg(selectedFolders(), this);

    if (dlg.exec() != QDialog::Accepted)
        return;

    foldersList->clear();
    foldersList->addItems(dlg.selectedFolders());
}

void SettingsDialog::removeSelectedFolder()
{
    delete foldersList->currentItem();
}

QStringList SettingsDialog::selectedFolders() const
{
    QStringList result;

    for (int i = 0; i < foldersList->count(); ++i)
        result << foldersList->item(i)->text();

    return result;
}

QStringList SettingsDialog::selectedTrackFormat() const
{
    QStringList fmt;

    if (coverCheck->isChecked()) fmt << "cover";
    if (artistCheck->isChecked()) fmt << "artist";
    if (albumCheck->isChecked()) fmt << "album";
    if (trackCheck->isChecked()) fmt << "track";

    return fmt;
}