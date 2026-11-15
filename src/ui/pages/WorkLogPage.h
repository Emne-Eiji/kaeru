// Kaeru — Work Log page (spec §8.2, delivered with WP-05)
#pragma once
#include "app/Application.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QTableWidget;

namespace kaeru {

class StatusBadge;

class WorkLogPage final : public QWidget {
    Q_OBJECT
public:
    explicit WorkLogPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    Application* m_app = nullptr;
    StatusBadge* m_badge = nullptr;
    QLabel* m_sessionInfo = nullptr;
    QLabel* m_totals = nullptr;
};

} // namespace kaeru
