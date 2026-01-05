#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>

class QFileSystemModel;
class QPushButton;
class QTreeView;

class FolderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FolderDialog(const QStringList& existingFolders,
        QWidget* parent = nullptr
    );

    QStringList selectedFolders() const;
private:
    void updateButtons();

    struct FolderEntry
    {
        QString raw;
        QString norm;
    };

    QVector<FolderEntry> folders;

    QFileSystemModel* model{ nullptr };
    QTreeView* view{ nullptr };
    QPushButton* addBtn{ nullptr };
    QPushButton* continueBtn{ nullptr };
};