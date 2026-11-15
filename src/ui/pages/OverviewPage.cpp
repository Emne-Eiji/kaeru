#include "OverviewPage.h"
#include "ui/widgets/StatusBadge.h"

#include "app/CommandRouter.h"
#include "core/Duration.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace kaeru {

static QWidget* card(QVBoxLayout** outLayout, const QString& title)
{
    auto* frame = new QFrame;
    frame->setObjectName(QStringLiteral("Card"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);
    auto* header = new QLabel(title, frame);
    header->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(header);
    *outLayout = layout;
    return frame;
}

OverviewPage::OverviewPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(12);

    // ---- Keep Awake card ----
    QVBoxLayout* cardLayout = nullptr;
    QWidget* awakeCard = card(&cardLayout, tr("Keep Awake"));
    m_awakeBadge = new StatusBadge(awakeCard);
    cardLayout->addWidget(m_awakeBadge);

    m_awakeDetail = new QLabel(awakeCard);
    m_awakeDetail->setWordWrap(true);
    cardLayout->addWidget(m_awakeDetail);

    m_awakeRemaining = new QLabel(awakeCard);
    m_awakeRemaining->setObjectName(QStringLiteral("TimerLabel"));
    cardLayout->addWidget(m_awakeRemaining);

    m_awakeWarning = new QLabel(awakeCard);
    m_awakeWarning->setObjectName(QStringLiteral("WarningLabel"));
    m_awakeWarning->setWordWrap(true);
    m_awakeWarning->hide();
    cardLayout->addWidget(m_awakeWarning);

    auto* durationRow = new QHBoxLayout();
    durationRow->setSpacing(6);
    const QList<QPair<QString, qint64>> chips = {
        { tr("15m"), 15 * 60 * 1000 },
        { tr("30m"), 30 * 60 * 1000 },
        { tr("1h"), 60 * 60 * 1000 },
        { tr("2h"), 120 * 60 * 1000 },
    };
    for (const auto& chip : chips) {
        auto* b = new QPushButton(chip.first, awakeCard);
        b->setToolTip(tr("Keep awake for %1").arg(chip.first));
        connect(b, &QPushButton::clicked, this, [this, ms = chip.second] { onDurationChosen(ms); });
        durationRow->addWidget(b);
        m_durationChips.append(b);
    }
    auto* indefinite = new QPushButton(tr("Indefinitely"), awakeCard);
    indefinite->setToolTip(tr("Keep awake until you turn it off"));
    connect(indefinite, &QPushButton::clicked, this, [this] { onDurationChosen(0); });
    durationRow->addWidget(indefinite);
    durationRow->addStretch(1);
    cardLayout->addLayout(durationRow);

    auto* actionRow = new QHBoxLayout();
    m_toggleButton = new QPushButton(tr("Start Keep Awake"), awakeCard);
    m_toggleButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_toggleButton->setAccessibleName(tr("Start or stop Keep Awake"));
    connect(m_toggleButton, &QPushButton::clicked, this, &OverviewPage::onToggleKeepAwake);
    m_turnOffButton = new QPushButton(tr("Turn off now"), awakeCard);
    m_turnOffButton->setObjectName(QStringLiteral("DangerButton"));
    connect(m_turnOffButton, &QPushButton::clicked, this, &OverviewPage::onTurnOffNow);
    actionRow->addWidget(m_toggleButton);
    actionRow->addWidget(m_turnOffButton);
    actionRow->addStretch(1);
    cardLayout->addLayout(actionRow);
    outer->addWidget(awakeCard);

    // ---- Work Log + Focus row ----
    auto* row = new QHBoxLayout();
    row->setSpacing(12);

    QVBoxLayout* workLayout = nullptr;
    QWidget* workCard = card(&workLayout, tr("Work Log"));
    m_workBadge = new StatusBadge(workCard);
    workLayout->addWidget(m_workBadge);
    m_workDetail = new QLabel(workCard);
    m_workDetail->setWordWrap(true);
    workLayout->addWidget(m_workDetail);
    row->addWidget(workCard, 1);

    QVBoxLayout* focusLayout = nullptr;
    QWidget* focusCard = card(&focusLayout, tr("Focus"));
    m_focusBadge = new StatusBadge(focusCard);
    focusLayout->addWidget(m_focusBadge);
    m_focusDetail = new QLabel(focusCard);
    m_focusDetail->setWordWrap(true);
    focusLayout->addWidget(m_focusDetail);
    row->addWidget(focusCard, 1);

    outer->addLayout(row);
    outer->addStretch(1);

    connect(m_app->inhibition(), &InhibitionCoordinator::stateChanged, this, &OverviewPage::refresh);
    connect(m_app->appState(), &AppState::changed, this, &OverviewPage::refresh);
    connect(m_app->focus(), &FocusController::stateChanged, this, &OverviewPage::refresh);
    refresh();

    // 1s refresh for remaining-time display
    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, &OverviewPage::refresh);
    tick->start(1000);
}

void OverviewPage::refresh()
{
    const EffectiveAwakeState st = m_app->awakeState();
    switch (st.status) {
    case EffectiveStatus::Active:
        m_awakeBadge->setStatus(tr("Active"), QStringLiteral("ok"), true);
        break;
    case EffectiveStatus::PartiallyActive:
        m_awakeBadge->setStatus(tr("Partially active"), QStringLiteral("warn"), true);
        break;
    case EffectiveStatus::Activating:
        m_awakeBadge->setStatus(tr("Activating…"), QStringLiteral("info"), false);
        break;
    case EffectiveStatus::PermissionDenied:
        m_awakeBadge->setStatus(tr("Permission denied"), QStringLiteral("bad"), true);
        break;
    case EffectiveStatus::Failed:
        m_awakeBadge->setStatus(tr("Failed"), QStringLiteral("bad"), true);
        break;
    case EffectiveStatus::Unsupported:
        m_awakeBadge->setStatus(tr("Unsupported"), QStringLiteral("warn"), false);
        break;
    case EffectiveStatus::Inactive:
        m_awakeBadge->setStatus(tr("Off"), QStringLiteral("neutral"), false);
        break;
    }

    QStringList details;
    if (!st.reason.isEmpty())
        details << tr("Reason: %1").arg(st.reason);
    if (!st.backend.isEmpty())
        details << tr("Backend: %1").arg(st.backend);
    if (!st.limitation.isEmpty())
        details << tr("Limitation: %1").arg(st.limitation);
    if (!st.errorDetail.isEmpty())
        details << st.errorDetail;
    m_awakeDetail->setText(details.join(QStringLiteral("\n")));

    if (st.remainingMs >= 0) {
        m_awakeRemaining->setText(formatRemainingMs(st.remainingMs));
        m_awakeRemaining->show();
    } else {
        m_awakeRemaining->hide();
    }

    const bool active = st.status == EffectiveStatus::Active || st.status == EffectiveStatus::PartiallyActive;
    m_toggleButton->setText(active ? tr("Stop Keep Awake") : tr("Start Keep Awake"));
    m_turnOffButton->setVisible(active);
    m_turnOffButton->setEnabled(active);

    m_awakeWarning->setVisible(false);

    // Work Log card (recording delivered with WP-05)
    const bool workEnabled = m_app->appState()->isModuleEnabled(Module::WorkLog);
    if (!workEnabled) {
        m_workBadge->setStatus(tr("Disabled"), QStringLiteral("neutral"), false);
        m_workDetail->setText(tr("Work Log is off. History is kept; start tracking in Settings."));
    } else {
        m_workBadge->setStatus(tr("Not recording"), QStringLiteral("neutral"), false);
        m_workDetail->setText(tr("Manual work tracking arrives with the Work Log milestone."));
    }

    // Focus card
    const bool focusEnabled = m_app->appState()->isModuleEnabled(Module::Focus);
    if (!focusEnabled) {
        m_focusBadge->setStatus(tr("Disabled"), QStringLiteral("neutral"), false);
        m_focusDetail->setText(tr("Focus module is off."));
    } else {
        const FocusController* f = m_app->focus();
        const qint64 remaining = f->remainingMs();
        if (f->phase() == FocusController::Phase::Idle) {
            m_focusBadge->setStatus(tr("Idle"), QStringLiteral("neutral"), false);
            m_focusDetail->setText(tr("No focus session running."));
        } else {
            m_focusBadge->setStatus(f->phaseLabel(), QStringLiteral("info"), true);
            m_focusDetail->setText(tr("Remaining: %1").arg(formatRemainingMs(remaining)));
        }
    }
}

void OverviewPage::onToggleKeepAwake()
{
    CommandRouter router(m_app);
    const EffectiveAwakeState st = m_app->awakeState();
    const bool active = st.status == EffectiveStatus::Active || st.status == EffectiveStatus::PartiallyActive;
    if (active)
        router.awakeOff();
    else
        router.awakeOn(QVariantMap {
            { QStringLiteral("durationMs"), 60 * 60 * 1000 },
            { QStringLiteral("mode"), QStringLiteral("both") } });
    refresh();
}

void OverviewPage::onTurnOffNow()
{
    CommandRouter(m_app).awakeOff();
    refresh();
}

void OverviewPage::onDurationChosen(qint64 ms)
{
    CommandRouter router(m_app);
    if (ms == 0) {
        // Indefinite: warn inline (spec §6.1) before starting.
        m_awakeWarning->setText(tr("Keep awake will stay on until you turn it off."));
        m_awakeWarning->show();
        m_app->resumeAutomaticRules();
        m_app->awakeIndefinite(QString());
    } else {
        m_app->resumeAutomaticRules();
        m_app->awakeOn(ms, QStringLiteral("both"));
    }
    refresh();
}

} // namespace kaeru
