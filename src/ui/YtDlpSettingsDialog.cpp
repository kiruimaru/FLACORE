#include "YtDlpSettingsDialog.h"

#include "core/ytdlp/YtDlp.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
QString settingsString(const char *key, const QString &fallback = QString())
{
    return QSettings().value(QString::fromLatin1(key), fallback).toString();
}
} // namespace

YtDlpSettingsDialog::YtDlpSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Настройки yt-dlp"));
    setMinimumWidth(620);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(14);

    auto *heading = new QLabel(tr("Онлайн-библиотека"), this);
    heading->setObjectName(QStringLiteral("trackTitle"));
    layout->addWidget(heading);

    auto *description = new QLabel(
        tr("Настройте загрузчики, папку сохранения и формат аудио."), this);
    description->setObjectName(QStringLiteral("trackPath"));
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // yt-dlp
    auto *binaryRow = new QHBoxLayout();
    m_binaryEdit = new QLineEdit(settingsString("ytdlp/binary", YtDlp::resolveBinary()), this);
    m_binaryEdit->setObjectName(QStringLiteral("ytDlpBinaryPath"));
    m_binaryEdit->setPlaceholderText(tr("пусто = искать yt-dlp в PATH"));
    auto *binaryBrowse = new QPushButton(tr("Обзор…"), this);
    auto *binaryDetect = new QPushButton(tr("Найти"), this);
    binaryBrowse->setObjectName(QStringLiteral("binaryBrowseButton"));
    binaryDetect->setObjectName(QStringLiteral("binaryDetectButton"));
    binaryRow->addWidget(m_binaryEdit, 1);
    binaryRow->addWidget(binaryBrowse);
    binaryRow->addWidget(binaryDetect);
    form->addRow(tr("yt-dlp:"), binaryRow);

    // ffmpeg
    auto *ffmpegRow = new QHBoxLayout();
    m_ffmpegEdit = new QLineEdit(settingsString("ytdlp/ffmpegDir", YtDlp::resolveFfmpegDir()), this);
    m_ffmpegEdit->setObjectName(QStringLiteral("ffmpegDirectory"));
    m_ffmpegEdit->setPlaceholderText(tr("папка с ffmpeg — нужна для сохранения в FLAC/MP3"));
    auto *ffmpegBrowse = new QPushButton(tr("Обзор…"), this);
    ffmpegBrowse->setObjectName(QStringLiteral("ffmpegBrowseButton"));
    ffmpegRow->addWidget(m_ffmpegEdit, 1);
    ffmpegRow->addWidget(ffmpegBrowse);
    form->addRow(tr("ffmpeg:"), ffmpegRow);

    // Библиотека
    auto *libraryRow = new QHBoxLayout();
    m_libraryEdit = new QLineEdit(settingsString("ytdlp/libraryDir", YtDlp::libraryDir()), this);
    m_libraryEdit->setObjectName(QStringLiteral("libraryDirectory"));
    auto *libraryBrowse = new QPushButton(tr("Обзор…"), this);
    libraryBrowse->setObjectName(QStringLiteral("libraryBrowseButton"));
    libraryRow->addWidget(m_libraryEdit, 1);
    libraryRow->addWidget(libraryBrowse);
    form->addRow(tr("Библиотека:"), libraryRow);

    // Формат
    m_formatCombo = new QComboBox(this);
    m_formatCombo->setObjectName(QStringLiteral("audioFormat"));
    m_formatCombo->addItem(QStringLiteral("FLAC (без потерь)"), QStringLiteral("flac"));
    m_formatCombo->addItem(QStringLiteral("MP3 320 kbps"), QStringLiteral("mp3"));
    m_formatCombo->addItem(QStringLiteral("Opus"), QStringLiteral("opus"));
    m_formatCombo->addItem(QStringLiteral("M4A (AAC)"), QStringLiteral("m4a"));
    const QString currentFormat = settingsString("ytdlp/audioFormat", QStringLiteral("flac"));
    const int formatIndex = m_formatCombo->findData(currentFormat);
    m_formatCombo->setCurrentIndex(formatIndex >= 0 ? formatIndex : 0);
    form->addRow(tr("Формат сохранения:"), m_formatCombo);

    layout->addLayout(form);

    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("settingsHint"));
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet(QStringLiteral("color: palette(mid);"));
    layout->addWidget(m_hint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName(QStringLiteral("settingsButtons"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(binaryBrowse, &QPushButton::clicked, this, &YtDlpSettingsDialog::browseBinary);
    connect(binaryDetect, &QPushButton::clicked, this, &YtDlpSettingsDialog::detectYtDlp);
    connect(ffmpegBrowse, &QPushButton::clicked, this, &YtDlpSettingsDialog::browseFfmpeg);
    connect(libraryBrowse, &QPushButton::clicked, this, &YtDlpSettingsDialog::browseLibrary);
    connect(m_binaryEdit, &QLineEdit::textChanged, this, [this]() { updateHint(); });
    connect(m_ffmpegEdit, &QLineEdit::textChanged, this, [this]() { updateHint(); });

    updateHint();
}

QString YtDlpSettingsDialog::binaryPath() const
{
    return m_binaryEdit->text().trimmed();
}

QString YtDlpSettingsDialog::ffmpegDir() const
{
    return m_ffmpegEdit->text().trimmed();
}

QString YtDlpSettingsDialog::libraryDir() const
{
    return m_libraryEdit->text().trimmed();
}

QString YtDlpSettingsDialog::audioFormat() const
{
    return m_formatCombo->currentData().toString();
}

void YtDlpSettingsDialog::browseBinary()
{
#ifdef Q_OS_WIN
    const QString filter = tr("Исполняемые файлы (*.exe);;Все файлы (*)");
#else
    const QString filter = tr("Все файлы (*)");
#endif
    const QString file = QFileDialog::getOpenFileName(this, tr("Путь к yt-dlp"),
                                                      m_binaryEdit->text(), filter);
    if (!file.isEmpty())
        m_binaryEdit->setText(QDir::toNativeSeparators(file));
}

void YtDlpSettingsDialog::browseFfmpeg()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Папка с ffmpeg"),
                                                          m_ffmpegEdit->text());
    if (!dir.isEmpty())
        m_ffmpegEdit->setText(QDir::toNativeSeparators(dir));
}

void YtDlpSettingsDialog::browseLibrary()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Папка библиотеки"),
                                                          m_libraryEdit->text());
    if (!dir.isEmpty())
        m_libraryEdit->setText(QDir::toNativeSeparators(dir));
}

void YtDlpSettingsDialog::detectYtDlp()
{
    const QString found = QStandardPaths::findExecutable(QStringLiteral("yt-dlp"));
    if (found.isEmpty())
        m_hint->setText(tr("yt-dlp не найден в PATH. Установите: pip install -U yt-dlp"));
    else
        m_binaryEdit->setText(QDir::toNativeSeparators(found));
}

void YtDlpSettingsDialog::updateHint()
{
    QStringList lines;
    const QString binary = measureBinary();

    if (binary.startsWith(QLatin1String("<")))
        lines << tr("yt-dlp: %1").arg(binary.mid(1, binary.size() - 2));
    else
        lines << tr("yt-dlp: версия %1").arg(binary);

    const QString ffmpeg = m_ffmpegEdit->text().trimmed().isEmpty()
                               ? QStandardPaths::findExecutable(QStringLiteral("ffmpeg"))
                               : m_ffmpegEdit->text().trimmed();
    if (ffmpeg.isEmpty())
        lines << tr("ffmpeg: не найден — сохранение в FLAC/MP3 работать не будет "
                    "(скачивание и прослушивание работают без него)");
    else
        lines << tr("ffmpeg: %1").arg(QDir::toNativeSeparators(QFileInfo(ffmpeg).absoluteFilePath()));

    m_hint->setText(lines.join(QLatin1Char('\n')));
}

QString YtDlpSettingsDialog::measureBinary() const
{
    const QString path = m_binaryEdit->text().trimmed();
    const QString resolved = path.isEmpty()
                                 ? QStandardPaths::findExecutable(QStringLiteral("yt-dlp"))
                                 : path;
    if (resolved.isEmpty() || !QFileInfo::exists(resolved))
        return QStringLiteral("<не найден>");

    // Кэш: --version запускается один раз на путь, а не на каждое нажатие клавиши.
    const auto cached = m_versionCache.constFind(resolved);
    if (cached != m_versionCache.constEnd())
        return cached.value();

    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    probe.setProcessEnvironment(env);
    probe.start(resolved, QStringList{QStringLiteral("--version")});

    QString result;
    if (!probe.waitForStarted(3000))
        result = QStringLiteral("<не запускается>");
    else if (!probe.waitForFinished(5000))
        result = QStringLiteral("<не отвечает>");
    else {
        const QString out = QString::fromUtf8(probe.readAll()).trimmed();
        result = (probe.exitCode() == 0 && !out.isEmpty())
                     ? out.split(QLatin1Char('\n')).value(0).trimmed()
                     : QStringLiteral("<ошибка: %1>").arg(out.left(160));
    }

    m_versionCache.insert(resolved, result);
    return result;
}
