#include "ReportsPage.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace kaeru {

ReportsPage::ReportsPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    root->addWidget(m_summary);

    m_export = new QPushButton(tr("Export CSV"), this);
    m_export->setEnabled(false);
    root->addWidget(m_export);
    root->addStretch(1);

    refresh();
}

void ReportsPage::refresh()
{
    m_summary->setText(tr("Daily and weekly totals, filters and CSV export arrive with the Reports milestone."));
}

} // namespace kaeru
