#pragma once

#include <QObject>
#include <QDialog>
#include <QStringList>

class QListWidget;
class QPushButton;
class QCheckBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(
        const QStringList& musicFolders,
        bool autoplay,
        const QStringList& trackFormat,
        QWidget* parent = nullptr
    );

    QStringList selectedFolders() const;
    QStringList selectedTrackFormat() const;
Q_SIGNALS:
    void rescanRequested();
private Q_SLOTS:
    void addFolder();
    void removeSelectedFolder();
private:
    QListWidget* foldersList = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* removeButton = nullptr;
    QPushButton* rescanButton = nullptr;
    QCheckBox* coverCheck = nullptr;
    QCheckBox* artistCheck = nullptr;
    QCheckBox* albumCheck = nullptr;
    QCheckBox* trackCheck = nullptr;
};