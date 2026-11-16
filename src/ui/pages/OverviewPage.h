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
    void onDurationChosen(qint64 ms);
    void onTurnOffNow();

private:
    Application* m_app = nullptr;
    StatusBadge* m_awakeBadge = nullptr;
    QLabel* m_awakeDetail = nullptr;
    QLabel* m_awakeRemaining = nullptr;
    QPushButton* m_turnOffButton = nullptr;
};

} // namespace kaeru
