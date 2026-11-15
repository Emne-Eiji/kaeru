#include "Theme.h"

#include <QApplication>
#include <QPalette>

namespace kaeru {

Theme& Theme::instance()
{
    static Theme t;
    return t;
}

void Theme::setDark(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    emit changed(m_dark);
}

void Theme::apply(const QString& preference, ColorScheme systemScheme)
{
    bool dark;
    if (preference == QStringLiteral("light"))
        dark = false;
    else if (preference == QStringLiteral("dark"))
        dark = true;
    else
        dark = systemScheme == ColorScheme::Dark; // "system"

    QPalette pal;
    if (dark) {
        pal.setColor(QPalette::Window, QColor(0x1e, 0x1f, 0x22));
        pal.setColor(QPalette::Base, QColor(0x26, 0x27, 0x2b));
        pal.setColor(QPalette::AlternateBase, QColor(0x2c, 0x2d, 0x31));
        pal.setColor(QPalette::Text, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::WindowText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::Button, QColor(0x30, 0x31, 0x35));
        pal.setColor(QPalette::ButtonText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::Highlight, QColor(0x2e, 0x7d, 0x32));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipBase, QColor(0x30, 0x31, 0x35));
        pal.setColor(QPalette::ToolTipText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::PlaceholderText, QColor(0x9a, 0x9a, 0x9a));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x77, 0x77, 0x77));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x77, 0x77, 0x77));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x77, 0x77, 0x77));
    } else {
        pal.setColor(QPalette::Window, QColor(0xfa, 0xfa, 0xfa));
        pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::AlternateBase, QColor(0xf2, 0xf3, 0xf4));
        pal.setColor(QPalette::Text, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::WindowText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::Button, QColor(0xf0, 0xf0, 0xf1));
        pal.setColor(QPalette::ButtonText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::Highlight, QColor(0x2e, 0x7d, 0x32));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x90, 0x97));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0xa5, 0xaa, 0xb0));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0xa5, 0xaa, 0xb0));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xa5, 0xaa, 0xb0));
    }
    qApp->setPalette(pal);
    qApp->setStyleSheet(styleSheet(dark));
    setDark(dark);
}

QString Theme::styleSheet(bool dark) const
{
    const QString bg = dark ? QStringLiteral("#1e1f22") : QStringLiteral("#fafafa");
    const QString card = dark ? QStringLiteral("#26272b") : QStringLiteral("#ffffff");
    const QString border = dark ? QStringLiteral("#3a3b40") : QStringLiteral("#e3e5e8");
    const QString text = dark ? QStringLiteral("#e6e6e6") : QStringLiteral("#24292f");
    const QString subtle = dark ? QStringLiteral("#a6a8ad") : QStringLiteral("#5f6672");
    const QString accent = dark ? QStringLiteral("#66bb6a") : QStringLiteral("#2e7d32");
    const QString hover = dark ? QStringLiteral("#33343a") : QStringLiteral("#eceef0");
    const QString pressed = dark ? QStringLiteral("#3a3b42") : QStringLiteral("#e0e3e6");
    const QString danger = dark ? QStringLiteral("#ef8177") : QStringLiteral("#c0392b");

    QString qss = QStringLiteral(
        "QWidget { font-size: 13px; }"
        "QMainWindow, QDialog { background: %bg; }"
        "#Card { background: %card; border: 1px solid %border; border-radius: 8px; }"
        "#CardTitle { font-weight: 600; color: %text; }"
        "#Subtle { color: %subtle; }"
        "#TimerLabel { font-size: 40px; font-weight: 300; color: %text; }"
        "QListWidget#Nav { background: transparent; border: none; }"
        "QListWidget#Nav::item { padding: 8px 12px; border-radius: 6px; margin: 2px 6px; color: %text; }"
        "QListWidget#Nav::item:selected { background: %accent; color: #ffffff; font-weight: 600; }"
        "QListWidget#Nav::item:hover:!selected { background: %hover; }"
        "QPushButton { padding: 6px 14px; border-radius: 6px; background: %hover; border: 1px solid %border; color: %text; }"
        "QPushButton:hover { background: %pressed; }"
        "QPushButton:pressed { background: %pressed; }"
        "QPushButton:disabled { color: %subtle; background: transparent; border-color: %border; }"
        "QPushButton#PrimaryButton { background: %accent; border: none; color: #ffffff; font-weight: 600; padding: 8px 20px; }"
        "QPushButton#PrimaryButton:disabled { background: %hover; color: %subtle; }"
        "QPushButton#DangerButton { border-color: %danger; color: %danger; }"
        "QLabel#WarningLabel { color: %danger; }"
        "QCheckBox, QRadioButton { color: %text; spacing: 6px; }"
        "QComboBox, QLineEdit, QSpinBox, QDateEdit, QTimeEdit { background: %card; border: 1px solid %border; border-radius: 6px; padding: 4px 8px; color: %text; }"
        "QTableView, QTreeView, QListView { background: %card; border: 1px solid %border; color: %text; }"
        "QHeaderView::section { background: %hover; border: none; padding: 4px 8px; color: %subtle; }"
        "QToolTip { background: %card; color: %text; border: 1px solid %border; }"
        "QMenu { background: %card; border: 1px solid %border; color: %text; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: %accent; color: #ffffff; }"
        "QMenu::item:disabled { color: %subtle; }");

    qss.replace(QStringLiteral("%bg"), bg)
        .replace(QStringLiteral("%card"), card)
        .replace(QStringLiteral("%border"), border)
        .replace(QStringLiteral("%text"), text)
        .replace(QStringLiteral("%subtle"), subtle)
        .replace(QStringLiteral("%accent"), accent)
        .replace(QStringLiteral("%hover"), hover)
        .replace(QStringLiteral("%pressed"), pressed)
        .replace(QStringLiteral("%danger"), danger);
    return qss;
}

} // namespace kaeru
