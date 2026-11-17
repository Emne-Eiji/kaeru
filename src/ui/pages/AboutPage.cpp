#include "AboutPage.h"

#include "app/UpdateChecker.h"
#include "ui/Icons.h"

#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace kaeru {

namespace {
const char* kRepositoryUrl = "https://github.com/Emne-Eiji/kaeru";
const char* kLicenseUrl = "https://www.gnu.org/licenses/gpl-3.0.html";

QLabel* linkLabel(const QString& text, const QUrl& url, QWidget* parent)
{
    auto* label = new QLabel(parent);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(false);
    label->setText(QStringLiteral("<a href=\"%1\">%2</a>")
                       .arg(url.toString().toHtmlEscaped(), text.toHtmlEscaped()));
    QObject::connect(label, &QLabel::linkActivated, label, [url](const QString&) {
        QDesktopServices::openUrl(url);
    });
    label->setCursor(Qt::PointingHandCursor);
    return label;
}
} // namespace

AboutPage::AboutPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setAccessibleName(tr("About"));
    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    // ---- Identity ----
    auto* identity = sectionCard(tr("About"));
    auto* identityLayout = qobject_cast<QVBoxLayout*>(identity->layout());
    auto* identityRow = new QHBoxLayout();
    identityRow->setSpacing(18);

    auto* artwork = new QLabel(identity);
    artwork->setPixmap(Icons::appIcon().pixmap(128, 128));
    artwork->setFixedSize(128, 128);
    artwork->setAlignment(Qt::AlignCenter);
    artwork->setAccessibleName(tr("Kaeru frog artwork"));
    identityRow->addWidget(artwork);

    auto* textColumn = new QVBoxLayout();
    textColumn->setSpacing(6);
    auto* name = new QLabel(QStringLiteral("Kaeru"), identity);
    QFont nameFont = name->font();
    nameFont.setPointSize(nameFont.pointSize() + 12);
    nameFont.setWeight(QFont::DemiBold);
    name->setFont(nameFont);
    textColumn->addWidget(name);
    auto* tagline = new QLabel(tr("Keep your computer awake on request."), identity);
    tagline->setObjectName(QStringLiteral("CardTitle"));
    textColumn->addWidget(tagline);
    auto* version = new QLabel(tr("Version %1").arg(QStringLiteral(KAERU_VERSION)), identity);
    version->setObjectName(QStringLiteral("Subtle"));
    version->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textColumn->addWidget(version);
    auto* description = new QLabel(
        tr("Kaeru asks your desktop to prevent automatic sleep and screen "
           "blanking for a finite duration or until you turn it off. Choosing "
           "a duration replaces the current request. It uses your desktop's "
           "own inhibition mechanisms and never changes global power settings "
           "or simulates input activity."),
        identity);
    description->setWordWrap(true);
    textColumn->addWidget(description);
    textColumn->addStretch(1);
    identityRow->addLayout(textColumn, 1);
    identityLayout->addLayout(identityRow);

    auto* localFirst = new QLabel(
        tr("Local-first: no accounts, no cloud service, no telemetry."), identity);
    localFirst->setObjectName(QStringLiteral("Subtle"));
    identityLayout->addWidget(localFirst);
    root->addWidget(identity);

    // ---- Project and license ----
    auto* project = sectionCard(tr("Project and license"));
    auto* projectLayout = qobject_cast<QVBoxLayout*>(project->layout());
    auto* repoText = new QLabel(
        tr("Source code, issues and releases:"), project);
    repoText->setWordWrap(true);
    projectLayout->addWidget(repoText);
    projectLayout->addWidget(linkLabel(QStringLiteral("github.com/Emne-Eiji/kaeru"),
                                       QUrl(QString::fromLatin1(kRepositoryUrl)), project));
    auto* licenseText = new QLabel(
        tr("Kaeru is free software licensed under the GNU General Public "
           "License, version 3 or later. There is no warranty."),
        project);
    licenseText->setWordWrap(true);
    projectLayout->addWidget(licenseText);
    projectLayout->addWidget(linkLabel(tr("GNU GPL v3.0 license text"),
                                       QUrl(QString::fromLatin1(kLicenseUrl)), project));
    root->addWidget(project);

    // ---- Update check ----
    auto* updates = sectionCard(tr("Updates"));
    auto* updateLayout = qobject_cast<QVBoxLayout*>(updates->layout());
    auto* updateIntro = new QLabel(
        tr("Checks the official GitHub Releases page for this repository. "
           "Kaeru only reports whether a newer published version exists and "
           "links to it; it does not download or install packages."),
        updates);
    updateIntro->setWordWrap(true);
    updateLayout->addWidget(updateIntro);

    auto* buttonRow = new QHBoxLayout();
    m_checkButton = new QPushButton(tr("Check for Updates"), updates);
    m_checkButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_checkButton->setAccessibleName(tr("Check for updates on GitHub"));
    connect(m_checkButton, &QPushButton::clicked, this, &AboutPage::checkForUpdates);
    buttonRow->addWidget(m_checkButton);
    buttonRow->addStretch(1);
    updateLayout->addLayout(buttonRow);

    m_updateStatus = new QLabel(
        tr("Not checked yet. No update status is claimed until a check succeeds."),
        updates);
    m_updateStatus->setObjectName(QStringLiteral("Subtle"));
    m_updateStatus->setWordWrap(true);
    m_updateStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    updateLayout->addWidget(m_updateStatus);

    m_releaseLink = new QLabel(updates);
    m_releaseLink->setTextFormat(Qt::RichText);
    m_releaseLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_releaseLink->setOpenExternalLinks(false);
    m_releaseLink->setVisible(false);
    m_releaseLink->setCursor(Qt::PointingHandCursor);
    connect(m_releaseLink, &QLabel::linkActivated, m_releaseLink,
            [](const QString& url) { QDesktopServices::openUrl(QUrl(url)); });
    updateLayout->addWidget(m_releaseLink);
    root->addWidget(updates);

    // ---- Acknowledgements ----
    auto* credits = sectionCard(tr("Acknowledgements"));
    auto* creditLayout = qobject_cast<QVBoxLayout*>(credits->layout());
    auto* creditsText = new QLabel(
        tr("Frog artwork © 2026 Emne, licensed GPL-3.0-or-later as part of the "
           "Kaeru project; the official artwork is used unchanged.\n"
           "Built with Qt 6. Desktop inhibition uses xdg-desktop-portal with "
           "a systemd-logind fallback. Thanks to the GNOME, XDG and Qt "
           "communities whose free software makes Kaeru possible."),
        credits);
    creditsText->setWordWrap(true);
    creditLayout->addWidget(creditsText);
    root->addWidget(credits);

    root->addStretch(1);
    scroll->setWidget(content);
    outer->addWidget(scroll);

    m_updates = new UpdateChecker(this);
    connect(m_updates, &UpdateChecker::finished,
            this, &AboutPage::onUpdateCheckFinished);
}

QWidget* AboutPage::sectionCard(const QString& title)
{
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("Card"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);
    auto* header = new QLabel(title, frame);
    header->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(header);
    return frame;
}

void AboutPage::checkForUpdates()
{
    m_checkButton->setEnabled(false);
    m_checkButton->setText(tr("Checking…"));
    m_updateStatus->setText(tr("Checking the official GitHub Releases page…"));
    m_releaseLink->setVisible(false);
    m_updates->check(QStringLiteral(KAERU_VERSION));
}

void AboutPage::onUpdateCheckFinished(const UpdateChecker::Result& result)
{
    m_checkButton->setEnabled(true);
    m_checkButton->setText(tr("Check for Updates"));
    m_updateStatus->setText(result.message);

    // Never show a release link or an encouraging "latest" claim after a
    // failed check. Only a successful comparison may present either.
    if (result.outcome == UpdateChecker::Outcome::UpdateAvailable) {
        m_releaseLink->setText(
            QStringLiteral("<a href=\"%1\">%2</a>")
                .arg(result.releaseUrl.toString().toHtmlEscaped(),
                     tr("Open the %1 release page").arg(result.latestVersion).toHtmlEscaped()));
        m_releaseLink->setVisible(true);
    } else {
        m_releaseLink->clear();
        m_releaseLink->setVisible(false);
    }

    if (!result.detail.isEmpty())
        m_updateStatus->setToolTip(result.detail);
    else
        m_updateStatus->setToolTip(QString());
}

} // namespace kaeru
