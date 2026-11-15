// Kaeru — Diagnostics page (spec §8.2, uiux §8)
#include "DiagnosticsPage.h"
#include "app/Paths.h"

#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kaeru {

DiagnosticsPage::DiagnosticsPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setAccessibleName(tr("Diagnostics"));
    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto* intro = new QLabel(
        tr("This page reports what Kaeru can actually do on this system — "
           "available, unavailable, denied, degraded or failed — with plain-language "
           "detail. Submitted requests are never counted as active inhibition."),
        content);
    intro->setWordWrap(true);
    root->addWidget(intro);

    auto* capCard = new QFrame(content);
    capCard->setObjectName(QStringLiteral("Card"));
    auto* capLayout = new QVBoxLayout(capCard);
    capLayout->setContentsMargins(16, 12, 16, 12);
    capLayout->setSpacing(8);
    auto* capTitle = new QLabel(tr("Capability matrix"), capCard);
    capTitle->setObjectName(QStringLiteral("CardTitle"));
    capLayout->addWidget(capTitle);

    m_capabilityTable = new QTableWidget(0, 3, capCard);
    m_capabilityTable->setHorizontalHeaderLabels({ tr("Capability"), tr("Status"), tr("Detail") });
    m_capabilityTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_capabilityTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_capabilityTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_capabilityTable->verticalHeader()->hide();
    m_capabilityTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_capabilityTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_capabilityTable->setAccessibleName(tr("Capability matrix"));
    capLayout->addWidget(m_capabilityTable);
    root->addWidget(capCard);

    auto* bindCard = new QFrame(content);
    bindCard->setObjectName(QStringLiteral("Card"));
    auto* bindLayout = new QVBoxLayout(bindCard);
    bindLayout->setContentsMargins(16, 12, 16, 12);
    bindLayout->setSpacing(8);
    auto* bindTitle = new QLabel(tr("Backend bindings (live)"), bindCard);
    bindTitle->setObjectName(QStringLiteral("CardTitle"));
    bindLayout->addWidget(bindTitle);
    m_bindings = new QLabel(bindCard);
    m_bindings->setWordWrap(true);
    m_bindings->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bindLayout->addWidget(m_bindings);
    root->addWidget(bindCard);

    auto* pathCard = new QFrame(content);
    pathCard->setObjectName(QStringLiteral("Card"));
    auto* pathLayout = new QVBoxLayout(pathCard);
    pathLayout->setContentsMargins(16, 12, 16, 12);
    pathLayout->setSpacing(8);
    auto* pathTitle = new QLabel(tr("Locations"), pathCard);
    pathTitle->setObjectName(QStringLiteral("CardTitle"));
    pathLayout->addWidget(pathTitle);
    m_paths = new QLabel(pathCard);
    m_paths->setWordWrap(true);
    m_paths->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLayout->addWidget(m_paths);
    root->addWidget(pathCard);

    root->addStretch(1);
    scroll->setWidget(content);
    outerLayout->addWidget(scroll);

    connect(m_app->capabilities(), &CapabilityRegistry::updated, this, &DiagnosticsPage::refresh);
    connect(m_app->inhibition(), &InhibitionCoordinator::stateChanged, this, &DiagnosticsPage::refresh);

    refresh();
}

void DiagnosticsPage::refresh()
{
    refreshCapabilities();
    refreshBindings();
    m_paths->setText(tr("Config: %1\nData: %2\nCache: %3")
                         .arg(Paths::configDir(), Paths::dataDir(), Paths::cacheDir()));
}

void DiagnosticsPage::refreshCapabilities()
{
    const QVector<CapabilityInfo> caps = m_app->capabilities()->all();
    m_capabilityTable->setRowCount(caps.size());
    for (int row = 0; row < caps.size(); ++row) {
        const CapabilityInfo& c = caps.at(row);
        auto* nameItem = new QTableWidgetItem(c.title.isEmpty() ? c.id : c.title);
        nameItem->setToolTip(c.id);
        auto* statusItem = new QTableWidgetItem(c.status);
        statusItem->setData(Qt::AccessibleTextRole, tr("Status %1 for %2").arg(c.status, c.id));
        auto* detailItem = new QTableWidgetItem(c.detail);
        if (!c.backend.isEmpty())
            detailItem->setToolTip(tr("backend: %1").arg(c.backend));
        m_capabilityTable->setItem(row, 0, nameItem);
        m_capabilityTable->setItem(row, 1, statusItem);
        m_capabilityTable->setItem(row, 2, detailItem);
    }
}

void DiagnosticsPage::refreshBindings()
{
    const QVariantMap diag = m_app->diagnosticsJson();
    const QVariantList bindings = diag.value(QStringLiteral("bindings")).toList();
    if (bindings.isEmpty()) {
        m_bindings->setText(tr("No backend bindings right now (nothing is being inhibited)."));
        return;
    }
    QStringList lines;
    for (const QVariant& v : bindings) {
        const QVariantMap b = v.toMap();
        lines << tr("%1 — %2 (%3)%4")
                     .arg(b.value(QStringLiteral("backend")).toString(),
                          b.value(QStringLiteral("state")).toString(),
                          b.value(QStringLiteral("id")).toString(),
                          b.value(QStringLiteral("error")).toString().isEmpty()
                              ? QString()
                              : QStringLiteral(" — %1").arg(b.value(QStringLiteral("error")).toString()));
    }
    m_bindings->setText(lines.join(QLatin1Char('\n')));
}

} // namespace kaeru
