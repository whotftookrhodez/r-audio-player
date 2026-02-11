#include "settingsdialog.h"
#include "settings.h"
#include "folderdialog.h"

#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include <QNetworkAccessManager>
#include <QCryptographicHash>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

int SettingsDialog::selectedCoverSize() const
{
    return coverSizeSpin->value();
}

QString SettingsDialog::selectedBackgroundImagePath() const
{
    return backgroundImageEdit->text().trimmed();
}

bool SettingsDialog::selectedIconButtons() const
{
    return iconButtonsCheck->isChecked();
}

bool SettingsDialog::selectedCoverNewWindow() const
{
    return coverNewWindowCheck->isChecked();
}

bool SettingsDialog::selectedTrackNumbers() const
{
    return trackNumbersCheck->isChecked();
}

SettingsDialog::SettingsDialog(
    const QStringList& musicFolders,
    bool /* autoplay */,
    int coverSize,
    const QStringList& trackFormat,
    const QString& backgroundImagePath,
    bool iconButtons,
    bool coverNewWindow,
    bool trackNumbers,
    const QString& lastfmUsername,
    const QString& lastfmSessionKey,
    QWidget* parent)
    : QDialog(parent),
    lastfmSessionKey(lastfmSessionKey)
{
    setWindowTitle("settings");
    setMinimumSize(582, 436);

    QPoint cursorPos = QCursor::pos();
    QScreen* screen = QGuiApplication::screenAt(cursorPos);

    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen)
    {
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
    removeButton->setEnabled(false);

    rescanButton = new QPushButton("rescan", this);

    connect(
        addButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::addFolder
    );

    connect(
        foldersList->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this]()
        {
            removeButton->setEnabled(!foldersList->selectedItems().isEmpty());
        }
    );

    connect(
        removeButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::removeSelectedFolder
    );

    connect(
        rescanButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::rescanRequested
    );

    auto folderButtons = new QHBoxLayout;
    folderButtons->addWidget(addButton);
    folderButtons->addWidget(removeButton);
    folderButtons->addWidget(rescanButton);

    coverSizeSpin = new QSpinBox(this);
    coverSizeSpin->setRange(35, 105);
    coverSizeSpin->setPrefix("cover size (35-105): ");
    coverSizeSpin->setSuffix(" px");
    coverSizeSpin->setMinimumWidth(coverSizeSpin->fontMetrics().horizontalAdvance("cover size (35-105): 105 px"));
    coverSizeSpin->setValue(coverSize);

    coverCheck = new QCheckBox("cover", this);
    artistCheck = new QCheckBox("artist", this);
    albumCheck = new QCheckBox("album", this);
    trackCheck = new QCheckBox("track", this);

    coverCheck->setChecked(trackFormat.contains("cover"));
    artistCheck->setChecked(trackFormat.contains("artist"));
    albumCheck->setChecked(trackFormat.contains("album"));
    trackCheck->setChecked(trackFormat.contains("track"));

    backgroundImageEdit = new QLineEdit(this);
    backgroundImageEdit->setPlaceholderText("background image path");
#if defined(Q_OS_WIN)
    backgroundImageEdit->setMinimumWidth(backgroundImageEdit->fontMetrics().horizontalAdvance("C:/Users/User/Pictures/picture.png")); // same length slashes, less work
#elif defined(Q_OS_MACOS)
    backgroundImageEdit->setMinimumWidth(backgroundImageEdit->fontMetrics().horizontalAdvance("/Users/username/Pictures"));
#elif defined(Q_OS_LINUX)
    backgroundImageEdit->setMinimumWidth(backgroundImageEdit->fontMetrics().horizontalAdvance("/home/username/Pictures/picture.png"));
#endif
    backgroundImageEdit->setText(backgroundImagePath);

    iconButtonsCheck = new QCheckBox("icon buttons", this);
    iconButtonsCheck->setChecked(iconButtons);

    coverNewWindowCheck = new QCheckBox("maximize cover to pip window", this);
    coverNewWindowCheck->setChecked(coverNewWindow);

    trackNumbersCheck = new QCheckBox("track number", this);
    trackNumbersCheck->setChecked(trackNumbers);

    auto formatLayout = new QHBoxLayout;
    //formatLayout->addStretch();
    formatLayout->addWidget(coverSizeSpin);
    formatLayout->addWidget(coverCheck);
    formatLayout->addWidget(artistCheck);
    formatLayout->addWidget(albumCheck);
    formatLayout->addWidget(trackCheck);
    //formatLayout->addStretch();

    auto controlsLayout = new QHBoxLayout;
    //controlsLayout->addStretch();
    controlsLayout->addWidget(backgroundImageEdit);
    controlsLayout->addWidget(iconButtonsCheck);
    controlsLayout->addWidget(coverNewWindowCheck);
    controlsLayout->addWidget(trackNumbersCheck);
    //controlsLayout->addStretch();

    auto credentialsLayout = new QVBoxLayout;
    lastfmUsernameEdit = new QLineEdit(this);
    lastfmUsernameEdit->setPlaceholderText("username");
    lastfmUsernameEdit->setText(lastfmUsername);

    lastfmPasswordEdit = new QLineEdit(this);
    lastfmPasswordEdit->setPlaceholderText("password");
    lastfmPasswordEdit->setEchoMode(QLineEdit::Password);

    credentialsLayout->addWidget(new QLabel("link last.fm:", this));
    credentialsLayout->addWidget(lastfmUsernameEdit);
    credentialsLayout->addWidget(lastfmPasswordEdit);

    auto loginBtn = new QPushButton("log in", this);
    auto logoutBtn = new QPushButton("log out", this);

    auto lastfmBtns = new QHBoxLayout;
    lastfmBtns->addWidget(loginBtn);
    lastfmBtns->addWidget(logoutBtn);

    loginBtn->setEnabled(false);
    logoutBtn->setEnabled(!lastfmSessionKey.isEmpty());
    credentialsLayout->addLayout(lastfmBtns);

    auto checkFields = [this, loginBtn]()
        {
            const bool enable = !lastfmUsernameEdit->text().isEmpty()
                && !lastfmPasswordEdit->text().isEmpty();

            loginBtn->setEnabled(enable);
        };

    connect(
        lastfmUsernameEdit,
        &QLineEdit::textChanged,
        this,
        checkFields
    );

    connect(
        lastfmPasswordEdit,
        &QLineEdit::textChanged,
        this,
        checkFields
    );

    auto* nam = new QNetworkAccessManager(this);

    connect(
        loginBtn,
        &QPushButton::clicked,
        this,
        [this, nam, loginBtn, logoutBtn]()
        {
            const QString username = lastfmUsernameEdit->text();
            const QString password = lastfmPasswordEdit->text();

            QMap<QString, QString> params;

            params["method"] = "auth.getMobileSession";
            params["username"] = username;
            params["password"] = password;
            params["api_key"] = Settings::LASTFM_API_KEY;

            QString sigBase;

            for (auto it = params.begin(); it != params.end(); ++it)
            {
                sigBase += it.key() + it.value();
            }

            sigBase += Settings::LASTFM_API_SECRET;

            const QString apiSig = QCryptographicHash::hash(
                sigBase.toUtf8(),
                QCryptographicHash::Md5
            ).toHex();

            QString postData;

            for (auto it = params.begin(); it != params.end(); ++it)
            {
                if (!postData.isEmpty())
                {
                    postData += "&";
                }

                postData += it.key() + "=" + QUrl::toPercentEncoding(it.value());
            }

            postData += "&api_sig=" + apiSig;
            postData += "&format=json";

            QNetworkRequest req(QUrl("https://ws.audioscrobbler.com/2.0/"));

            req.setHeader(
                QNetworkRequest::ContentTypeHeader,
                "application/x-www-form-urlencoded"
            );

            QNetworkReply* reply = nam->post(
                req,
                postData.toUtf8()
            );

            connect(
                reply,
                &QNetworkReply::finished,
                this,
                [this, reply, loginBtn, logoutBtn]()
                {
                    reply->deleteLater();

                    if (reply->error() != QNetworkReply::NoError)
                    {
                        QMessageBox::critical(this,
                            "last.fm",
                            "network error:\n" + reply->errorString()
                        );

                        return;
                    }

                    const auto json = QJsonDocument::fromJson(reply->readAll());
                    const auto obj = json.object();

                    if (obj.contains("error"))
                    {
                        QMessageBox::critical(
                            this,
                            "last.fm login failed:",
                            obj["message"].toString()
                        );

                        return;
                    }

                    const auto session = obj["session"].toObject();
                    this->lastfmSessionKey = session["key"].toString();

                    lastfmLoggedIn(
                        this->lastfmSessionKey,
                        lastfmUsernameEdit->text()
                    );

                    QMessageBox::information(
                        this,
                        "last.fm",
                        "logged in"
                    );

                    logoutBtn->setEnabled(true);
                }
            );
        }
    );

    connect(
        logoutBtn,
        &QPushButton::clicked,
        this,
        [this, loginBtn, logoutBtn]()
        {
            this->lastfmSessionKey.clear();

            lastfmUsernameEdit->clear();
            lastfmPasswordEdit->clear();

            logoutBtn->setEnabled(false);

            Q_EMIT lastfmLoggedOut();
        }
    );

    auto buttons = new QDialogButtonBox(this);

    auto saveBtn = buttons->addButton("save", QDialogButtonBox::AcceptRole);
    auto cancelBtn = buttons->addButton("cancel", QDialogButtonBox::RejectRole);

    connect(
        saveBtn,
        &QPushButton::clicked,
        this,
        &QDialog::accept
    );

    connect(
        cancelBtn,
        &QPushButton::clicked,
        this,
        &QDialog::reject
    );

    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("music folders:", this));
    layout->addWidget(foldersList);
    layout->addLayout(folderButtons);
    layout->addSpacing(6);
    layout->addWidget(new QLabel("track display format:", this));
    layout->addLayout(formatLayout);
    layout->addSpacing(6);
    layout->addWidget(new QLabel("controls:", this));
    layout->addLayout(controlsLayout);
    layout->addSpacing(6);
    layout->addLayout(credentialsLayout);
    layout->addStretch();
    layout->addWidget(buttons);
}

void SettingsDialog::addFolder()
{
    FolderDialog dlg(
        selectedFolders(),
        this
    );

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    foldersList->clear();
    foldersList->addItems(dlg.selectedFolders());
}

void SettingsDialog::removeSelectedFolder()
{
    if (auto* item = foldersList->currentItem(); item
        && item->isSelected())
    {
        delete item;
    }
}

QStringList SettingsDialog::selectedFolders() const
{
    QStringList result;

    for (int i = 0; i < foldersList->count(); ++i)
    {
        result << foldersList->item(i)->text();
    }

    return result;
}

QStringList SettingsDialog::selectedTrackFormat() const
{
    QStringList fmt;

    if (coverCheck->isChecked())
    {
        fmt << "cover";
    }

    if (artistCheck->isChecked())
    {
        fmt << "artist";
    }

    if (albumCheck->isChecked())
    {
        fmt << "album";
    }

    if (trackCheck->isChecked())
    {
        fmt << "track";
    }

    return fmt;
}