#include "OverviewPage.h"
#include "ui/CustomDurationDialog.h"
#include "ui/widgets/StatusBadge.h"

#include "core/AwakeDuration.h"
#include "core/Duration.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace kaeru {

static QLabel* subtleLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Subtle"));
    label->setWordWrap(true);
    return label;
}

OverviewPage::OverviewPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(12);

    auto* heading = new QLabel(tr("Keep Awake"), this);
    QFont headingFont = heading->font();
    headingFont.setPointSize(headingFont.pointSize() + 7);
    headingFont.setWeight(QFont::DemiBold);
    heading->setFont(headingFont);
    outer->addWidget(heading);
    outer->addWidget(subtleLabel(
        tr("Asks your desktop to prevent automatic sleep and screen blanking while Kaeru runs. "
           "It never changes your power settings and never simulates activity."),
        this));

    // ---- Single status/action card ----
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(10);

    m_awakeBadge = new StatusBadge(card);
    cardLayout->addWidget(m_awakeBadge);

    m_awakeRemaining = new QLabel(card);
    m_awakeRemaining->setObjectName(QStringLiteral("TimerLabel"));
    m_awakeRemaining->setAccessibleName(tr("Keep Awake remaining time"));
    cardLayout->addWidget(m_awakeRemaining);

    m_awakeDetail = new QLabel(card);
    m_awakeDetail->setObjectName(QStringLiteral("Subtle"));
    m_awakeDetail->setWordWrap(true);
    m_awakeDetail->setMinimumHeight(36);
    cardLayout->addWidget(m_awakeDetail);

    auto* separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    cardLayout->addWidget(separator);

    cardLayout->addWidget(new QLabel(tr("Keep awake for:"), card));

    auto* durationRow = new QHBoxLayout();
    durationRow->setSpacing(6);
    const QList<QPair<QString, AwakeDuration::Kind>> chips = {
        { tr("15 minutes"), AwakeDuration::Kind::Minutes15 },
        { tr("30 minutes"), AwakeDuration::Kind::Minutes30 },
        { tr("1 hour"), AwakeDuration::Kind::Hour1 },
        { tr("2 hours"), AwakeDuration::Kind::Hours2 },
    };
    for (const auto& chip : chips) {
        auto* b = new QPushButton(chip.first, card);
        b->setToolTip(tr("Keep awake for %1").arg(chip.first));
        b->setAccessibleName(tr("Keep awake for %1").arg(chip.first));
        connect(b, &QPushButton::clicked, this, [this, kind = chip.second] {
            m_app->selectAwakeDuration(AwakeDuration::standard(kind));
            refresh();
        });
        durationRow->addWidget(b);
    }
    auto* indefinite = new QPushButton(tr("Indefinitely"), card);
    indefinite->setToolTip(tr("Keep awake until you turn it off"));
    indefinite->setAccessibleName(tr("Keep awake until turned off"));
    connect(indefinite, &QPushButton::clicked, this, [this] {
        m_app->selectAwakeDuration(AwakeDuration::standard(AwakeDuration::Kind::Indefinite));
        refresh();
    });
    durationRow->addWidget(indefinite);
    auto* custom = new QPushButton(tr("Custom…"), card);
    custom->setToolTip(tr("Choose a custom finite duration"));
    custom->setAccessibleName(tr("Keep awake for a custom duration"));
    connect(custom, &QPushButton::clicked, this, &OverviewPage::onCustomDuration);
    durationRow->addWidget(custom);
    durationRow->addStretch(1);
    cardLayout->addLayout(durationRow);

    m_lastSelection = new QLabel(card);
    m_lastSelection->setObjectName(QStringLiteral("Subtle"));
    m_lastSelection->setWordWrap(true);
    cardLayout->addWidget(m_lastSelection);

    cardLayout->addWidget(subtleLabel(
        tr("Choosing a duration replaces the current Keep Awake request; durations never stack."),
        card));

    auto* actionRow = new QHBoxLayout();
    m_turnOffButton = new QPushButton(tr("Turn off Keep Awake"), card);
    m_turnOffButton->setObjectName(QStringLiteral("DangerButton"));
    m_turnOffButton->setAccessibleName(tr("Turn off Keep Awake now"));
    connect(m_turnOffButton, &QPushButton::clicked, this, &OverviewPage::onTurnOffNow);
    actionRow->addWidget(m_turnOffButton);
    actionRow->addStretch(1);
    cardLayout->addLayout(actionRow);

    outer->addWidget(card);
    outer->addWidget(subtleLabel(
        tr("Keep Awake prevents automatic sleep. Logging out, shutting down, or a low battery "
           "can still end the session."),
        this));
    outer->addStretch(1);

    connect(m_app->inhibition(), &InhibitionCoordinator::stateChanged, this, &OverviewPage::refresh);
    connect(m_app->appState(), &AppState::changed, this, &OverviewPage::refresh);
    connect(m_app->settings(), &Settings::changed, this, &OverviewPage::refresh);
    refresh();

    // 1 s refresh for the remaining-time display.
    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, &OverviewPage::refresh);
    tick->start(1000);
}

void OverviewPage::refresh()
{
    const EffectiveAwakeState st = m_app->awakeState();
    const bool active = st.status == EffectiveStatus::Active
        || st.status == EffectiveStatus::PartiallyActive;
    const bool busy = active || st.status == EffectiveStatus::Activating;

    switch (st.status) {
    case EffectiveStatus::Active:
        m_awakeBadge->setStatus(tr("On"), QStringLiteral("ok"), true);
        break;
    case EffectiveStatus::PartiallyActive:
        m_awakeBadge->setStatus(tr("On (limited)"), QStringLiteral("warn"), true);
        break;
    case EffectiveStatus::Activating:
        m_awakeBadge->setStatus(tr("Starting…"), QStringLiteral("info"), false);
        break;
    case EffectiveStatus::PermissionDenied:
        m_awakeBadge->setStatus(tr("Permission denied"), QStringLiteral("bad"), true);
        break;
    case EffectiveStatus::Failed:
        m_awakeBadge->setStatus(tr("Failed"), QStringLiteral("bad"), true);
        break;
    case EffectiveStatus::Unsupported:
        m_awakeBadge->setStatus(tr("Not supported"), QStringLiteral("warn"), false);
        break;
    case EffectiveStatus::Inactive:
        m_awakeBadge->setStatus(tr("Off"), QStringLiteral("neutral"), false);
        break;
    }

    if (st.status == EffectiveStatus::Inactive) {
        m_awakeDetail->setText(st.limitation.isEmpty()
                ? tr("Nothing is being prevented right now.")
                : st.limitation);
    } else if (st.status == EffectiveStatus::Activating) {
        m_awakeDetail->setText(tr("Waiting for the desktop to confirm the request…"));
    } else if (active) {
        QString detail = tr("Automatic sleep and screen blanking are being prevented.");
        if (!st.backend.isEmpty())
            detail += QStringLiteral(" ") + tr("Backend: %1.").arg(st.backend);
        if (!st.limitation.isEmpty())
            detail += QStringLiteral(" ") + st.limitation;
        m_awakeDetail->setText(detail);
    } else {
        QString detail = st.errorDetail;
        if (!detail.isEmpty() && !st.limitation.isEmpty())
            detail += QStringLiteral("\n") + st.limitation;
        if (detail.isEmpty())
            detail = tr("The request could not be confirmed.");
        m_awakeDetail->setText(detail);
    }

    if (st.remainingMs >= 0) {
        m_awakeRemaining->setText(formatRemainingMs(st.remainingMs));
    } else if (m_app->manualAwakeIndefinite() && busy) {
        m_awakeRemaining->setText(tr("Until you turn it off"));
    } else if (busy) {
        m_awakeRemaining->setText(tr("On"));
    } else {
        m_awakeRemaining->setText(tr("Not running"));
    }

    m_turnOffButton->setVisible(busy);
    m_turnOffButton->setEnabled(busy);

    const AwakeDuration last = m_app->lastAwakeDuration();
    m_lastSelection->setText(tr("Tray left-click starts the last selected duration: %1.")
                                 .arg(last.label()));
}

void OverviewPage::onCustomDuration()
{
    const AwakeDuration last = m_app->lastAwakeDuration();
    CustomDurationDialog dialog(this, last.isCustom() ? last.customMs : 90LL * 60 * 1000);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_app->selectAwakeDuration(dialog.selection());
    refresh();
}

void OverviewPage::onTurnOffNow()
{
    m_app->awakeOff();
    refresh();
}

} // namespace kaeru
