#pragma once

#include <QDialog>
#include <QStringList>
#include <QLineEdit>

class QListWidget;
class QPushButton;
class QSpinBox;
class QCheckBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(
        const QStringList& musicFolders,
        bool autoplay,
        int coverSize,
        const QStringList& trackFormat,
        bool iconButtons,
        bool trackNumbers,
        const QString& lastfmUsername,
        const QString& lastfmSessionKey,
        QWidget* parent = nullptr
    );

    QString getlastfmUsername() const { return lastfmUsernameEdit->text(); }
    QString getlastfmSessionKey() const { return lastfmSessionKey; }

    int selectedCoverSize() const;

    bool selectedIconButtons() const;
    bool selectedTrackNumbers() const;

    QStringList selectedFolders() const;
    QStringList selectedTrackFormat() const;
Q_SIGNALS:
    void lastfmLoggedIn(const QString& sessionKey, const QString& username);

    // safe to ignore
    void lastfmLoggedOut();
    void rescanRequested();
private Q_SLOTS:
    void addFolder();
    void removeSelectedFolder();
private:
    QString lastfmSessionKey;
    QListWidget* foldersList = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* removeButton = nullptr;
    QPushButton* rescanButton = nullptr;
    QSpinBox* coverSizeSpin = nullptr;
    QCheckBox* coverCheck = nullptr;
    QCheckBox* artistCheck = nullptr;
    QCheckBox* albumCheck = nullptr;
    QCheckBox* trackCheck = nullptr;
    QCheckBox* iconButtonsCheck = nullptr;
    QCheckBox* trackNumbersCheck = nullptr;
    QLineEdit* lastfmUsernameEdit = nullptr;
    QLineEdit* lastfmPasswordEdit = nullptr;
};