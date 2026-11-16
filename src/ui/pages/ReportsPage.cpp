// Kaeru — Reports page (spec §8.2, §6.4, WP-06)
#include "ReportsPage.h"

#include "core/Duration.h"
#include "storage/CsvExport.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kaeru {

ReportsPage::ReportsPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto* title = new QLabel(tr("Reports"), this);
    title->setObjectName(QStringLiteral("CardTitle"));
    root->addWidget(title);

    // ---- Filters ----
    auto* filterCard = new QFrame(this);
    filterCard->setObjectName(QStringLiteral("Card"));
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(16, 12, 16, 12);
    filterLayout->setSpacing(6);
    filterLayout->addWidget(new QLabel(tr("From:"), filterCard));
    m_from = new QDateEdit(filterCard);
    m_from->setCalendarPopup(true);
    m_from->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_from->setDate(QDate::currentDate().addDays(-6));
    connect(m_from, &QDateEdit::dateChanged, this, &ReportsPage::refresh);
    filterLayout->addWidget(m_from);
    filterLayout->addWidget(new QLabel(tr("To:"), filterCard));
    m_to = new QDateEdit(filterCard);
    m_to->setCalendarPopup(true);
    m_to->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_to->setDate(QDate::currentDate());
    connect(m_to, &QDateEdit::dateChanged, this, &ReportsPage::refresh);
    filterLayout->addWidget(m_to);
    filterLayout->addWidget(new QLabel(tr("Project:"), filterCard));
    m_project = new QComboBox(filterCard);
    m_project->setMinimumWidth(160);
    m_project->setAccessibleName(tr("Project filter"));
    connect(m_project, &QComboBox::currentIndexChanged, this, &ReportsPage::refresh);
    filterLayout->addWidget(m_project);
    m_export = new QPushButton(tr("Export CSV…"), filterCard);
    m_export->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_export, &QPushButton::clicked, this, &ReportsPage::onExport);
    filterLayout->addWidget(m_export);
    filterLayout->addStretch(1);
    root->addWidget(filterCard);

    // ---- Totals table ----
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({ tr("Day"), tr("Work"), tr("Break/paused"), tr("Total") });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAccessibleName(tr("Daily totals"));
    root->addWidget(m_table, 1);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("Subtle"));
    m_summary->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_summary);

    rebuildProjectFilter();
    refresh();
}

void ReportsPage::rebuildProjectFilter()
{
    const QString current = m_project->currentText();
    m_project->clear();
    m_project->addItem(tr("All projects"));
    if (m_app->storageReady() && m_app->projects()) {
        const QList<ProjectRecord> projects = m_app->projects()->list(false, nullptr);
        for (const ProjectRecord& p : projects)
            m_project->addItem(p.name);
    }
    const int idx = m_project->findText(current);
    if (idx >= 0)
        m_project->setCurrentIndex(idx);
}

bool ReportsPage::currentRange(QDate* from, QDate* to) const
{
    *from = m_from->date();
    *to = m_to->date();
    if (!from->isValid() || !to->isValid() || *from > *to)
        return false;
    return true;
}

void ReportsPage::refresh()
{
    if (!m_app->storageReady() || !m_app->sessions() || !m_app->projects()) {
        m_table->setRowCount(0);
        m_summary->setText(tr("Work Log storage is unavailable."));
        m_export->setEnabled(false);
        return;
    }
    m_export->setEnabled(true);

    QDate from, to;
    if (!currentRange(&from, &to)) {
        m_table->setRowCount(0);
        m_summary->setText(tr("Invalid date range."));
        return;
    }
    const QDateTime fromUtc = QDateTime(from, QTime(0, 0), Qt::UTC);
    const QDateTime toUtc = QDateTime(to, QTime(23, 59, 59, 999), Qt::UTC);
    const qint64 offset = QDateTime::currentDateTime().offsetFromUtc();

    QString err;
    const QList<SessionRepository::DailyTotal> days =
        m_app->sessions()->dailyTotals(fromUtc, toUtc, int(offset), &err);
    m_table->setRowCount(days.size());
    qint64 workSum = 0, breakSum = 0;
    for (int row = 0; row < days.size(); ++row) {
        const SessionRepository::DailyTotal& d = days.at(row);
        auto* dayItem = new QTableWidgetItem(d.day.toString(QStringLiteral("yyyy-MM-dd (ddd)")));
        auto* workItem = new QTableWidgetItem(formatDurationMs(d.workMs));
        auto* breakItem = new QTableWidgetItem(formatDurationMs(d.breakMs));
        auto* totalItem = new QTableWidgetItem(formatDurationMs(d.workMs + d.breakMs));
        m_table->setItem(row, 0, dayItem);
        m_table->setItem(row, 1, workItem);
        m_table->setItem(row, 2, breakItem);
        m_table->setItem(row, 3, totalItem);
        workSum += d.workMs;
        breakSum += d.breakMs;
    }

    // Project breakdown (below the daily rows, same table).
    const QList<SessionRepository::ProjectTotal> byProject =
        m_app->sessions()->totalsByProject(fromUtc, toUtc, &err);
    int filteredSessions = 0;
    const QString selected = m_project->currentText();
    for (const SessionRepository::ProjectTotal& p : byProject) {
        if (selected != tr("All projects") && selected != p.name)
            continue;
        m_table->setRowCount(m_table->rowCount() + 1);
        const int row = m_table->rowCount() - 1;
        m_table->setItem(row, 0, new QTableWidgetItem(tr("Project: %1").arg(p.name)));
        m_table->setItem(row, 1, new QTableWidgetItem(formatDurationMs(p.workMs)));
        m_table->setItem(row, 2, new QTableWidgetItem(formatDurationMs(p.breakMs)));
        m_table->setItem(row, 3, new QTableWidgetItem(formatDurationMs(p.workMs + p.breakMs)));
        filteredSessions += p.sessionCount;
    }

    const SessionRepository::FocusTotal focus =
        m_app->sessions()->focusTotals(fromUtc, toUtc, &err);
    m_summary->setText(tr("Total: %1 work, %2 break/paused · Focus time (achieved): %3 · %4 session(s)")
                           .arg(formatDurationMs(workSum),
                                formatDurationMs(breakSum),
                                formatDurationMs(focus.achievedMs))
                           .arg(filteredSessions));
}

void ReportsPage::onExport()
{
    QDate from, to;
    if (!currentRange(&from, &to)) {
        QMessageBox::warning(this, tr("Kaeru"), tr("Invalid date range."));
        return;
    }
    const QDateTime fromUtc = QDateTime(from, QTime(0, 0), Qt::UTC);
    const QDateTime toUtc = QDateTime(to, QTime(23, 59, 59, 999), Qt::UTC);

    QString err;
    const QList<CsvExport::Row> rows =
        CsvExport::gather(*m_app->sessions(), *m_app->projects(), fromUtc, toUtc, &err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, tr("Kaeru"), err);
        return;
    }
    const QString suggested = QDir::homePath() + QLatin1Char('/')
        + CsvExport::fileNameFor(from, to);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Work Log CSV"), suggested,
        tr("CSV files (*.csv)"));
    if (path.isEmpty())
        return;
    int rowCount = 0;
    if (!CsvExport::write(CsvExport::serialize(rows), path, &err, &rowCount)) {
        QMessageBox::warning(this, tr("Kaeru"), err);
        return;
    }
    QMessageBox::information(this, tr("Export complete"),
        tr("Exported %1 row(s) to:\n%2").arg(rowCount).arg(path));
}

} // namespace kaeru