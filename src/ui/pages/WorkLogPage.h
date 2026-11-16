// Kaeru — Work Log page (spec §8.2, WP-05)
// Recording controls, project selection, today's total, recovery notice.
#pragma once
#include "app/Application.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;

namespace kaeru {

class StatusBadge;

class WorkLogPage final : public QWidget {
    Q_OBJECT
public:
    explicit WorkLogPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onStart();
    void onPause();
    void onResume();
    void onStop();
    void onProjectChanged(int index);

private:
    void rebuildProjects();
    void showStatus(const QString& message, bool error);

    Application* m_app = nullptr;
    StatusBadge* m_badge = nullptr;
    QLabel* m_sessionInfo = nullptr;
    QLabel* m_totals = nullptr;
    QLabel* m_notice = nullptr;
    QComboBox* m_project = nullptr;
    QPushButton* m_start = nullptr;
    QPushButton* m_pause = nullptr;
    QPushButton* m_resume = nullptr;
    QPushButton* m_stop = nullptr;
    bool m_updating = false;
};

} // namespace kaeru