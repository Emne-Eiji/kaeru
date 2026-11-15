#include "WorkLogPage.h"
#include "ui/widgets/StatusBadge.h"

#include <QLabel>
#include <QVBoxLayout>

namespace kaeru {

WorkLogPage::WorkLogPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    m_badge = new StatusBadge(this);
    root->addWidget(m_badge);

    m_sessionInfo = new QLabel(this);
    m_sessionInfo->setWordWrap(true);
    root->addWidget(m_sessionInfo);

    m_totals = new QLabel(this);
    m_totals->setObjectName(QStringLiteral("Subtle"));
    root->addWidget(m_totals);
    root->addStretch(1);

    refresh();
}

void WorkLogPage::refresh()
{
    if (!m_app->appState()->isModuleEnabled(Module::WorkLog)) {
        m_badge->setStatus(tr("Disabled"), QStringLiteral("neutral"), false);
        m_sessionInfo->setText(tr("Work Log is off. Existing history is kept."));
        m_totals->setText(QString());
        return;
    }
    m_badge->setStatus(tr("Not recording"), QStringLiteral("neutral"), false);
    m_sessionInfo->setText(tr("Manual work tracking with projects, sessions and honest labels arrives with the Work Log milestone."));
    m_totals->setText(tr("Today: 0m recorded"));
}

} // namespace kaeru
