#ifndef LIBRARYPAGE_H
#define LIBRARYPAGE_H

#include <QListWidget>
#include <QWidget>

#include "ui/widgets/TrackDelegate.h"

class QLabel;

/**
 * Список треков, принимающий файлы перетаскиванием и умеющий фильтроваться
 * поиском из шапки. Состояние модели остаётся в Playlist — страница только
 * отображает и сообщает о действиях пользователя.
 */
class TrackListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit TrackListWidget(QWidget *parent = nullptr);

signals:
    void filesDropped(const QStringList &paths);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

class LibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryPage(QWidget *parent = nullptr);

    TrackListWidget *list() const { return m_list; }
    TrackDelegate *delegate() const { return m_delegate; }

    void addTrack(const QString &title, const QString &artist, const QString &durationText,
                  const QString &album = QString());
    void removeRow(int row);
    void clearRows();
    void setPlayingRow(int row);
    void setFilter(const QString &text);
    void updateCountLabel(int visible, int total);

signals:
    void openFilesRequested();
    void removeRequested(int row);
    void clearRequested();
    void trackActivated(int row);
    void filesDropped(const QStringList &paths);

private:
    void refreshEmptyState();

    TrackListWidget *m_list = nullptr;
    TrackDelegate *m_delegate = nullptr;
    QLabel *m_empty = nullptr;
    QLabel *m_count = nullptr;
    QString m_title;
};

#endif // LIBRARYPAGE_H