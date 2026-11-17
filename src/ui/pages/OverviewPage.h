// Kaeru — Keep Awake page (initial release): one clear state, one action model
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
    void onCustomDuration();
    void onTurnOffNow();

private:
    Application* m_app = nullptr;
    StatusBadge* m_awakeBadge = nullptr;
    QLabel* m_awakeDetail = nullptr;
    QLabel* m_awakeRemaining = nullptr;
    QLabel* m_lastSelection = nullptr;
    QPushButton* m_turnOffButton = nullptr;
};

} // namespace kaeru
