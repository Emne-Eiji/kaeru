// Kaeru — Focus page (spec §8.2, delivered with WP-04)
#pragma once
#include "app/Application.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace kaeru {

class StatusBadge;

class FocusPage final : public QWidget {
    Q_OBJECT
public:
    explicit FocusPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onStart();
    void onPause();
    void onResume();
    void onStop();
    void onSkip();
    void onExtend();
    void onOptionChanged();

private:
    void applyOptionsToController();

    Application* m_app = nullptr;
    QLabel* m_timer = nullptr;
    StatusBadge* m_badge = nullptr;
    QLabel* m_phaseInfo = nullptr;

    QComboBox* m_routine = nullptr;
    QSpinBox* m_customMinutes = nullptr;

    QPushButton* m_start = nullptr;
    QPushButton* m_pause = nullptr;
    QPushButton* m_resume = nullptr;
    QPushButton* m_skip = nullptr;
    QPushButton* m_extend = nullptr;
    QPushButton* m_stop = nullptr;

    QCheckBox* m_keepAwakeWork = nullptr;
    QCheckBox* m_keepAwakeBreaks = nullptr;
    QCheckBox* m_recordWorkLog = nullptr;
    QCheckBox* m_autoNext = nullptr;

    QSpinBox* m_workMinutes = nullptr;
    QSpinBox* m_shortBreak = nullptr;
    QSpinBox* m_longBreak = nullptr;
    QSpinBox* m_longEvery = nullptr;
};

} // namespace kaeru
