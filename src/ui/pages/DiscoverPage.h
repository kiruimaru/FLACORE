#ifndef DISCOVERPAGE_H
#define DISCOVERPAGE_H

#include <QHash>
#include <QList>
#include <QWidget>

#include "core/online/Charts.h"

class QComboBox;
class QLabel;
class QListWidget;
class QScrollArea;
class QVBoxLayout;

class ChartsFetcher;
class CoverLoader;

/**
 * «Обзор» — живая подборка: полки с чартами из открытых источников
 * (Deezer по жанрам, Apple Music по странам, чарт SoundCloud через yt-dlp).
 *
 * Страница ничего не скачивает сама и не решает, что играть: она показывает
 * данные и сообщает наружу playRequested()/saveRequested(), а маршрутизацию
 * в yt-dlp делает MainWindow.
 */
class DiscoverPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiscoverPage(QWidget *parent = nullptr);

    struct Shelf {
        QString id;
        QString title;
        QString subtitle;
        QList<ChartTrack> tracks;
    };

    /// Идентификаторы полок.
    static QString deezerShelfId() { return QStringLiteral("deezer"); }
    static QString appleShelfId() { return QStringLiteral("apple"); }
    static QString soundCloudShelfId() { return QStringLiteral("soundcloud"); }

    /// Добавить (или заменить) полку — используется и загрузкой, и тестами.
    void addShelf(const Shelf &shelf);
    int shelfCount() const;
    /// Список полки по id (пусто, если полки нет).
    QListWidget *shelfList(const QString &id) const;
    /// Сколько в полке настоящих строк: подсказка «данных нет» не считается.
    int shelfTrackCount(const QString &id) const;
    /// Текст состояния под полкой.
    void setShelfStatus(const QString &id, const QString &text, bool error = false);
    void setJobStatus(const QString &text);

    /// Загрузить всё (Deezer, Apple и SoundCloud-чарт).
    void refreshAll();

signals:
    void playRequested(const ChartTrack &track);
    void saveRequested(const ChartTrack &track);
    void previewRequested(const ChartTrack &track);
    /// Просьба прочитать чарт SoundCloud через yt-dlp (выполняет MainWindow).
    void soundCloudChartRequested(const QString &url);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    QListWidget *createShelf(const QString &id, const QString &title, const QString &subtitle,
                             QWidget *controls);
    void fillShelf(const QString &id, const QList<ChartTrack> &tracks, const QString &subtitle);
    /// Текст вместо строк, когда данных нет: полка не выглядит пустой карточкой.
    void setShelfHint(QListWidget *list, const QString &text);
    void requestSoundCloudChart();
    void openContextMenuFor(QListWidget *list, const QPoint &position);
    ChartTrack trackAt(QListWidget *list, int row) const;

    CoverLoader *m_covers = nullptr;
    ChartsFetcher *m_fetcher = nullptr;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_content = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
    QComboBox *m_deezerGenre = nullptr;
    QComboBox *m_appleCountry = nullptr;
    QComboBox *m_appleFeed = nullptr;
    QComboBox *m_soundCloudGenre = nullptr;
    QLabel *m_jobStatus = nullptr;
    QHash<QString, QListWidget *> m_lists;
    QHash<QString, QLabel *> m_statuses;
    bool m_loadedOnce = false;
};

#endif // DISCOVERPAGE_H