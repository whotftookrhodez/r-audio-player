#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>

class QFileSystemModel;
class QTreeView;
class QPushButton;

class FolderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FolderDialog(
        const QStringList& existingFolders,
        QWidget* parent = nullptr
    );

    QStringList selectedFolders() const;
private:
    struct FolderEntry
    {
        QString raw;
        QString norm;
    };

    QVector<FolderEntry> folders;

    void updateButtons();

    QFileSystemModel* model = nullptr;
    QTreeView* view = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* continueButton = nullptr;
};