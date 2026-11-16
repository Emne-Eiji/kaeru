#include "FocusPage.h"
#include "ui/widgets/StatusBadge.h"

#include "app/Settings.h"
#include "core/Duration.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace kaeru {

FocusPage::FocusPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(24);

    // ---- Left: timer and controls ----
    auto* left = new QVBoxLayout();
    left->setSpacing(12);

    m_badge = new StatusBadge(this);
    left->addWidget(m_badge);

    m_timer = new QLabel(QStringLiteral("00:00"), this);
    m_timer->setObjectName(QStringLiteral("TimerLabel"));
    left->addWidget(m_timer);

    m_phaseInfo = new QLabel(this);
    m_phaseInfo->setObjectName(QStringLiteral("Subtle"));
    m_phaseInfo->setWordWrap(true);
    left->addWidget(m_phaseInfo);

    auto* controls = new QGridLayout();
    controls->setSpacing(6);
    m_start = new QPushButton(tr("Start"), this);
    m_pause = new QPushButton(tr("Pause"), this);
    m_resume = new QPushButton(tr("Resume"), this);
    m_skip = new QPushButton(tr("Skip interval"), this);
    m_extend = new QPushButton(tr("Extend +5 min"), this);
    m_stop = new QPushButton(tr("Stop"), this);
    controls->addWidget(m_start, 0, 0);
    controls->addWidget(m_pause, 0, 1);
    controls->addWidget(m_resume, 0, 2);
    controls->addWidget(m_skip, 1, 0);
    controls->addWidget(m_extend, 1, 1);
    controls->addWidget(m_stop, 1, 2);
    left->addLayout(controls);
    left->addStretch(1);

    // ---- Right: routine + options ----
    auto* right = new QVBoxLayout();
    right->setSpacing(8);

    auto* routineLabel = new QLabel(tr("Routine"), this);
    routineLabel->setObjectName(QStringLiteral("CardTitle"));
    right->addWidget(routineLabel);

    m_routine = new QComboBox(this);
    m_routine->addItems({ tr("Pomodoro"), tr("Custom countdown") });
    m_routine->setAccessibleName(tr("Focus routine"));
    right->addWidget(m_routine);

    auto* customRow = new QHBoxLayout();
    customRow->addWidget(new QLabel(tr("Custom duration (minutes)"), this));
    m_customMinutes = new QSpinBox(this);
    m_customMinutes->setRange(1, 480);
    m_customMinutes->setValue(25);
    customRow->addWidget(m_customMinutes);
    customRow->addStretch(1);
    right->addLayout(customRow);

    auto* defaultsLabel = new QLabel(tr("Pomodoro defaults (editable)"), this);
    defaultsLabel->setObjectName(QStringLiteral("CardTitle"));
    right->addWidget(defaultsLabel);

    auto* grid = new QGridLayout();
    grid->addWidget(new QLabel(tr("Work (min)"), this), 0, 0);
    m_workMinutes = new QSpinBox(this);
    m_workMinutes->setRange(1, 240);
    grid->addWidget(m_workMinutes, 0, 1);
    grid->addWidget(new QLabel(tr("Short break (min)"), this), 1, 0);
    m_shortBreak = new QSpinBox(this);
    m_shortBreak->setRange(1, 120);
    grid->addWidget(m_shortBreak, 1, 1);
    grid->addWidget(new QLabel(tr("Long break (min)"), this), 2, 0);
    m_longBreak = new QSpinBox(this);
    m_longBreak->setRange(1, 240);
    grid->addWidget(m_longBreak, 2, 1);
    grid->addWidget(new QLabel(tr("Long break after N work intervals"), this), 3, 0);
    m_longEvery = new QSpinBox(this);
    m_longEvery->setRange(2, 12);
    grid->addWidget(m_longEvery, 3, 1);
    right->addLayout(grid);

    auto* optionsLabel = new QLabel(tr("Options"), this);
    optionsLabel->setObjectName(QStringLiteral("CardTitle"));
    right->addWidget(optionsLabel);

    m_keepAwakeWork = new QCheckBox(tr("Keep awake during focus (work intervals)"), this);
    m_keepAwakeBreaks = new QCheckBox(tr("Keep awake during breaks"), this);
    m_recordWorkLog = new QCheckBox(tr("Record focus time in Work Log"), this);
    m_autoNext = new QCheckBox(tr("Automatically start next interval"), this);
    right->addWidget(m_keepAwakeWork);
    right->addWidget(m_keepAwakeBreaks);
    right->addWidget(m_recordWorkLog);
    right->addWidget(m_autoNext);
    right->addStretch(1);

    root->addLayout(left, 1);
    root->addLayout(right, 1);

    // Wire controls
    connect(m_start, &QPushButton::clicked, this, &FocusPage::onStart);
    connect(m_pause, &QPushButton::clicked, this, &FocusPage::onPause);
    connect(m_resume, &QPushButton::clicked, this, &FocusPage::onResume);
    connect(m_skip, &QPushButton::clicked, this, &FocusPage::onSkip);
    connect(m_extend, &QPushButton::clicked, this, &FocusPage::onExtend);
    connect(m_stop, &QPushButton::clicked, this, &FocusPage::onStop);
    for (auto* box : { m_keepAwakeWork, m_keepAwakeBreaks, m_recordWorkLog, m_autoNext })
        connect(box, &QCheckBox::toggled, this, &FocusPage::onOptionChanged);
    connect(m_workMinutes, &QSpinBox::valueChanged, this, &FocusPage::onOptionChanged);
    connect(m_shortBreak, &QSpinBox::valueChanged, this, &FocusPage::onOptionChanged);
    connect(m_longBreak, &QSpinBox::valueChanged, this, &FocusPage::onOptionChanged);
    connect(m_longEvery, &QSpinBox::valueChanged, this, &FocusPage::onOptionChanged);

    connect(m_app->focus(), &FocusController::stateChanged, this, &FocusPage::refresh);
    connect(m_app->appState(), &AppState::changed, this, [this] { refresh(); });

    // Seed the routine widgets from persisted preferences (the controller is
    // seeded from the same Settings by Application at startup). Seeding must
    // NOT write settings back — block the change signals.
    Settings* s = m_app->settings();
    {
        const QSignalBlocker blockWork(m_workMinutes);
        const QSignalBlocker blockShort(m_shortBreak);
        const QSignalBlocker blockLong(m_longBreak);
        const QSignalBlocker blockEvery(m_longEvery);
        const QSignalBlocker blockAuto(m_autoNext);
        const QSignalBlocker blockKAWork(m_keepAwakeWork);
        const QSignalBlocker blockKABreaks(m_keepAwakeBreaks);
        const QSignalBlocker blockRecord(m_recordWorkLog);
        m_customMinutes->setValue(s->focusWorkMinutes());
        m_workMinutes->setValue(s->focusWorkMinutes());
        m_shortBreak->setValue(s->focusShortBreakMinutes());
        m_longBreak->setValue(s->focusLongBreakMinutes());
        m_longEvery->setValue(s->focusLongBreakEvery());
        m_autoNext->setChecked(s->focusAutoNext());
        m_keepAwakeWork->setChecked(s->focusKeepAwakeDuringWork());
        m_keepAwakeBreaks->setChecked(s->focusKeepAwakeDuringBreaks());
        m_recordWorkLog->setChecked(s->focusRecordInWorkLog());
    }

    refresh();
}

void FocusPage::applyOptionsToController()
{
    FocusController* f = m_app->focus();
    const FocusController::Options& current = f->options();
    FocusController::Options o;
    o.workMs = m_workMinutes->value() * 60 * 1000;
    o.shortBreakMs = m_shortBreak->value() * 60 * 1000;
    o.longBreakMs = m_longBreak->value() * 60 * 1000;
    o.longBreakEvery = m_longEvery->value();
    o.autoNext = m_autoNext->isChecked();
    o.keepAwakeDuringWork = m_keepAwakeWork->isChecked();
    o.keepAwakeDuringBreaks = m_keepAwakeBreaks->isChecked();
    o.recordInWorkLog = m_recordWorkLog->isChecked();
    o.notifyOnInterval = m_app->settings()->notificationsEnabled();
    o.soundOnNotify = m_app->settings()->soundEnabled();
    // Quiet hours are configured in Settings; keep whatever is active there.
    o.quietHoursEnabled = current.quietHoursEnabled;
    o.quietFromMinutes = current.quietFromMinutes;
    o.quietToMinutes = current.quietToMinutes;
    f->setOptions(o);

    // Persist settings
    m_app->settings()->setFocusWorkMinutes(m_workMinutes->value());
    m_app->settings()->setFocusShortBreakMinutes(m_shortBreak->value());
    m_app->settings()->setFocusLongBreakMinutes(m_longBreak->value());
    m_app->settings()->setFocusLongBreakEvery(m_longEvery->value());
    m_app->settings()->setFocusAutoNext(m_autoNext->isChecked());
    m_app->settings()->setFocusKeepAwakeDuringWork(m_keepAwakeWork->isChecked());
    m_app->settings()->setFocusKeepAwakeDuringBreaks(m_keepAwakeBreaks->isChecked());
    m_app->settings()->setFocusRecordInWorkLog(m_recordWorkLog->isChecked());
}

void FocusPage::refresh()
{
    const FocusController* f = m_app->focus();
    const qint64 remaining = f->remainingMs();
    m_timer->setText(remaining >= 0 ? formatRemainingMs(remaining) : QStringLiteral("—"));
    m_badge->setStatus(f->phaseLabel(), f->phase() == FocusController::Phase::Idle ? QStringLiteral("neutral") : QStringLiteral("info"),
        f->phase() != FocusController::Phase::Idle);

    QStringList info;
    if (f->mode() == FocusController::Mode::Pomodoro)
        info << tr("Work intervals completed: %1").arg(f->completedWorkIntervals());
    if (f->phase() == FocusController::Phase::Paused)
        info << tr("Paused — time is not accruing.");
    m_phaseInfo->setText(info.join(QStringLiteral(" · ")));

    const bool running = f->phase() != FocusController::Phase::Idle;
    m_start->setEnabled(!running);
    m_pause->setEnabled(running && f->phase() != FocusController::Phase::Paused);
    m_resume->setEnabled(f->phase() == FocusController::Phase::Paused);
    m_skip->setEnabled(running);
    m_extend->setEnabled(running && f->phase() != FocusController::Phase::Paused);
    m_stop->setEnabled(running || f->phase() == FocusController::Phase::Paused);
}

void FocusPage::onStart()
{
    FocusController* f = m_app->focus();
    applyOptionsToController();
    if (m_routine->currentIndex() == 0)
        f->startPomodoro();
    else
        f->startCustom(m_customMinutes->value() * 60 * 1000);
    refresh();
}

void FocusPage::onPause() { m_app->focus()->pause(); refresh(); }
void FocusPage::onResume() { m_app->focus()->resume(); refresh(); }
void FocusPage::onStop() { m_app->focus()->stop(); refresh(); }
void FocusPage::onSkip() { m_app->focus()->skipInterval(); refresh(); }
void FocusPage::onExtend() { m_app->focus()->extendInterval(5 * 60 * 1000); refresh(); }

void FocusPage::onOptionChanged()
{
    applyOptionsToController();
}

} // namespace kaeru
