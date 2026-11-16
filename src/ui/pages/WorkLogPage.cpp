// Kaeru — Work Log page (spec §8.2, WP-05)
#include "WorkLogPage.h"
#include "ui/widgets/StatusBadge.h"

#include "app/Settings.h"
#include "app/WorkSessionManager.h"
#include "core/Duration.h"
#include "storage/ProjectRepository.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace kaeru {

WorkLogPage::WorkLogPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    // ---- Status card ----
    auto* statusCard = new QFrame(this);
    statusCard->setObjectName(QStringLiteral("Card"));
    auto* statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(16, 12, 16, 12);
    statusLayout->setSpacing(8);
    auto* title = new QLabel(tr("Work Log"), statusCard);
    title->setObjectName(QStringLiteral("CardTitle"));
    statusLayout->addWidget(title);

    m_badge = new StatusBadge(statusCard);
    statusLayout->addWidget(m_badge);

    m_sessionInfo = new QLabel(statusCard);
    m_sessionInfo->setWordWrap(true);
    statusLayout->addWidget(m_sessionInfo);

    m_totals = new QLabel(statusCard);
    m_totals->setObjectName(QStringLiteral("Subtle"));
    statusLayout->addWidget(m_totals);

    m_notice = new QLabel(statusCard);
    m_notice->setObjectName(QStringLiteral("WarningLabel"));
    m_notice->setWordWrap(true);
    m_notice->hide();
    statusLayout->addWidget(m_notice);
    root->addWidget(statusCard);

    // ---- Controls ----
    auto* controlCard = new QFrame(this);
    controlCard->setObjectName(QStringLiteral("Card"));
    auto* controlLayout = new QHBoxLayout(controlCard);
    controlLayout->setContentsMargins(16, 12, 16, 12);
    controlLayout->setSpacing(6);
    controlLayout->addWidget(new QLabel(tr("Project:"), controlCard));
    m_project = new QComboBox(controlCard);
    m_project->setMinimumWidth(180);
    m_project->setAccessibleName(tr("Work log project"));
    connect(m_project, &QComboBox::currentIndexChanged, this, &WorkLogPage::onProjectChanged);
    controlLayout->addWidget(m_project);

    m_start = new QPushButton(tr("Start"), controlCard);
    m_start->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_start, &QPushButton::clicked, this, &WorkLogPage::onStart);
    controlLayout->addWidget(m_start);
    m_pause = new QPushButton(tr("Pause"), controlCard);
    connect(m_pause, &QPushButton::clicked, this, &WorkLogPage::onPause);
    controlLayout->addWidget(m_pause);
    m_resume = new QPushButton(tr("Resume"), controlCard);
    connect(m_resume, &QPushButton::clicked, this, &WorkLogPage::onResume);
    controlLayout->addWidget(m_resume);
    m_stop = new QPushButton(tr("Stop & save"), controlCard);
    connect(m_stop, &QPushButton::clicked, this, &WorkLogPage::onStop);
    controlLayout->addWidget(m_stop);
    controlLayout->addStretch(1);
    root->addWidget(controlCard);

    root->addStretch(1);

    connect(m_app->work(), &WorkSessionManager::stateChanged, this, &WorkLogPage::refresh);
    rebuildProjects();
    refresh();
}

void WorkLogPage::rebuildProjects()
{
    m_updating = true;
    const QString current = m_project->currentText();
    m_project->clear();
    if (m_app->storageReady() && m_app->projects()) {
        const QList<ProjectRecord> projects = m_app->projects()->list(false, nullptr);
        for (const ProjectRecord& p : projects)
            m_project->addItem(p.name);
        if (m_project->findText(current) >= 0)
            m_project->setCurrentText(current);
        else if (m_app->settings()) {
            const int def = m_project->findText(m_app->settings()->defaultProject());
            if (def >= 0)
                m_project->setCurrentIndex(def);
        }
    }
    m_updating = false;
}

void WorkLogPage::onProjectChanged(int index)
{
    Q_UNUSED(index)
    if (m_updating || !m_app->work() || !m_app->work()->isRecording())
        return;
    QString err;
    if (!m_app->work()->switchProject(m_project->currentText(), &err))
        showStatus(err, true);
}

void WorkLogPage::refresh()
{
    WorkSessionManager* w = m_app->work();
    if (!w || !m_app->storageReady()) {
        m_badge->setStatus(tr("Unavailable"), QStringLiteral("bad"), false);
        m_sessionInfo->setText(tr("Work Log storage is unavailable. See Diagnostics."));
        m_start->setEnabled(false);
        m_pause->setEnabled(false);
        m_resume->setEnabled(false);
        m_stop->setEnabled(false);
        return;
    }
    const SessionRecord s = w->session();
    const bool active = w->isActive();
    const bool recording = w->isRecording();

    if (active) {
        m_badge->setStatus(recording ? tr("Recording") : tr("Paused"),
                           recording ? QStringLiteral("ok") : QStringLiteral("warn"), true);
        m_sessionInfo->setText(tr("Session #%1 — started %2")
                                   .arg(s.id)
                                   .arg(s.startedUtc.toLocalTime().toString(QStringLiteral("HH:mm"))));
    } else {
        m_badge->setStatus(tr("Idle"), QStringLiteral("neutral"), false);
        m_sessionInfo->setText(tr("Not recording. Start a session to track work."));
    }
    m_totals->setText(tr("Today: %1").arg(formatDurationMs(w->todayWorkMs())));

    m_start->setEnabled(!active);
    m_pause->setEnabled(recording);
    m_resume->setEnabled(active && !recording);
    m_stop->setEnabled(active);

    if (!m_app->recoveryNotes().isEmpty() && m_notice->isHidden())
        showStatus(m_app->recoveryNotes().join(QStringLiteral("\n")), false);
}

void WorkLogPage::onStart()
{
    QString err;
    if (!m_app->work()->start(m_project->currentText(), &err))
        showStatus(err, true);
    refresh();
}

void WorkLogPage::onPause()
{
    QString err;
    if (!m_app->work()->pause(QStringLiteral("manual"), &err))
        showStatus(err, true);
    refresh();
}

void WorkLogPage::onResume()
{
    QString err;
    if (!m_app->work()->resume(&err))
        showStatus(err, true);
    refresh();
}

void WorkLogPage::onStop()
{
    QString err;
    if (!m_app->work()->stop(&err))
        showStatus(err, true);
    refresh();
}

void WorkLogPage::showStatus(const QString& message, bool error)
{
    m_notice->setText(message);
    m_notice->setObjectName(error ? QStringLiteral("WarningLabel") : QStringLiteral("Subtle"));
    m_notice->show();
}

} // namespace kaeru