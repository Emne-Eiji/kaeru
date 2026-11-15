// Kaeru — Overview page (spec §8.3): state-first, immediate primary actions
#pragma once
#include "app/Application.h"

#include <QWidget>

class QLabel;
class QPushButton;

namespace kaeru {

class StatusBadge;

class OverviewPage final : public QWidget {
    Q_OBJECT
public:
    explicit OverviewPage(Application* app, QWidget* parent = nullptr);

private slots:
    void refresh();
    void onToggleKeepAwake();
    void onTurnOffNow();
    void onDurationChosen(qint64 ms);

private:
    QWidget* makeCard();

    Application* m_app = nullptr;
    StatusBadge* m_awakeBadge = nullptr;
    QLabel* m_awakeDetail = nullptr;
    QLabel* m_awakeRemaining = nullptr;
    QLabel* m_awakeWarning = nullptr;
    QPushButton* m_toggleButton = nullptr;
    QPushButton* m_turnOffButton = nullptr;
    QList<QPushButton*> m_durationChips;

    StatusBadge* m_workBadge = nullptr;
    QLabel* m_workDetail = nullptr;

    StatusBadge* m_focusBadge = nullptr;
    QLabel* m_focusDetail = nullptr;
};

} // namespace kaeru
