#include "YtDlpSettingsDialog.h"

#include "YtDlpSettingsForm.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

YtDlpSettingsDialog::YtDlpSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Настройки yt-dlp"));
    setMinimumWidth(620);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 14);
    layout->setSpacing(14);

    m_form = new YtDlpSettingsForm(this);
    layout->addWidget(m_form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    if (QPushButton *ok = buttons->button(QDialogButtonBox::Ok))
        ok->setProperty("variant", "accent");
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString YtDlpSettingsDialog::binaryPath() const
{
    return m_form->binaryPath();
}

QString YtDlpSettingsDialog::ffmpegDir() const
{
    return m_form->ffmpegDir();
}

QString YtDlpSettingsDialog::libraryDir() const
{
    return m_form->libraryDir();
}

QString YtDlpSettingsDialog::audioFormat() const
{
    return m_form->audioFormat();
}