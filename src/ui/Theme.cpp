#include "Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

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

    // GTK/Adwaita Qt platform themes ignore large parts of QPalette and QSS
    // on some widgets. Fusion is Qt's own native style and makes the chosen
    // appearance apply consistently to dialogs, menus, inputs and tables.
    if (!qApp->style() || !qApp->style()->objectName()
             .contains(QStringLiteral("fusion"), Qt::CaseInsensitive)) {
        if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion")))
            QApplication::setStyle(fusion);
    }

    QPalette pal;
    if (dark) {
        pal.setColor(QPalette::Window, QColor(0x1e, 0x1f, 0x22));
        pal.setColor(QPalette::Base, QColor(0x26, 0x27, 0x2b));
        pal.setColor(QPalette::AlternateBase, QColor(0x2c, 0x2d, 0x31));
        pal.setColor(QPalette::Text, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::WindowText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::Button, QColor(0x30, 0x31, 0x35));
        pal.setColor(QPalette::ButtonText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::Light, QColor(0x3a, 0x3b, 0x40));
        pal.setColor(QPalette::Midlight, QColor(0x33, 0x34, 0x38));
        pal.setColor(QPalette::Mid, QColor(0x2c, 0x2d, 0x31));
        pal.setColor(QPalette::Dark, QColor(0x18, 0x19, 0x1b));
        pal.setColor(QPalette::Shadow, QColor(0x10, 0x11, 0x12));
        pal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Highlight, QColor(0x2e, 0x7d, 0x32));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Link, QColor(0x8b, 0xd4, 0x8e));
        pal.setColor(QPalette::LinkVisited, QColor(0xb0, 0xe0, 0xb2));
        pal.setColor(QPalette::ToolTipBase, QColor(0x30, 0x31, 0x35));
        pal.setColor(QPalette::ToolTipText, QColor(0xe6, 0xe6, 0xe6));
        pal.setColor(QPalette::PlaceholderText, QColor(0x9a, 0x9a, 0x9a));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x8b, 0x8d, 0x93));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x8b, 0x8d, 0x93));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x8b, 0x8d, 0x93));
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(0x8b, 0x8d, 0x93));
    } else {
        pal.setColor(QPalette::Window, QColor(0xfa, 0xfa, 0xfa));
        pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::AlternateBase, QColor(0xf2, 0xf3, 0xf4));
        pal.setColor(QPalette::Text, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::WindowText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::Button, QColor(0xf0, 0xf0, 0xf1));
        pal.setColor(QPalette::ButtonText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Midlight, QColor(0xf7, 0xf7, 0xf8));
        pal.setColor(QPalette::Mid, QColor(0xd9, 0xdc, 0xe0));
        pal.setColor(QPalette::Dark, QColor(0xb8, 0xbd, 0xc4));
        pal.setColor(QPalette::Shadow, QColor(0x8c, 0x93, 0x9c));
        pal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Highlight, QColor(0x2e, 0x7d, 0x32));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Link, QColor(0x1a, 0x73, 0xc8));
        pal.setColor(QPalette::LinkVisited, QColor(0x6a, 0x3e, 0xb4));
        pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipText, QColor(0x24, 0x29, 0x2f));
        pal.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x90, 0x97));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x8c, 0x93, 0x9c));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x8c, 0x93, 0x9c));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x8c, 0x93, 0x9c));
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(0x8c, 0x93, 0x9c));
    }
    qApp->setPalette(pal);
    qApp->setStyleSheet(styleSheet(dark));
    setDark(dark);
}

QString Theme::styleSheet(bool dark) const
{
    const QString bg = dark ? QStringLiteral("#1e1f22") : QStringLiteral("#fafafa");
    const QString card = dark ? QStringLiteral("#26272b") : QStringLiteral("#ffffff");
    const QString alt = dark ? QStringLiteral("#2c2d31") : QStringLiteral("#f2f3f4");
    const QString border = dark ? QStringLiteral("#3a3b40") : QStringLiteral("#dfe2e6");
    const QString text = dark ? QStringLiteral("#e6e6e6") : QStringLiteral("#24292f");
    const QString subtle = dark ? QStringLiteral("#a6a8ad") : QStringLiteral("#5f6672");
    const QString accent = dark ? QStringLiteral("#3f9143") : QStringLiteral("#2e7d32");
    const QString accentHover = dark ? QStringLiteral("#4aa44f") : QStringLiteral("#256c28");
    const QString hover = dark ? QStringLiteral("#33343a") : QStringLiteral("#eceef0");
    const QString pressed = dark ? QStringLiteral("#3a3b42") : QStringLiteral("#dfe3e6");
    const QString danger = dark ? QStringLiteral("#ef8177") : QStringLiteral("#c0392b");
    const QString selection = dark ? QStringLiteral("#2e7d32") : QStringLiteral("#2e7d32");

    QString qss = QStringLiteral(
        "QWidget { font-size: 13px; color: %text; }"
        "QMainWindow, QDialog, QMessageBox { background: %bg; }"
        "QFrame, QScrollArea, QGroupBox { background: transparent; color: %text; }"
        "QFrame#Card { background: %card; border: 1px solid %border; border-radius: 8px; }"
        "#CardTitle { font-weight: 600; color: %text; }"
        "#Subtle, QLabel#Subtle { color: %subtle; }"
        "#TimerLabel { font-size: 40px; font-weight: 300; color: %text; }"
        "#TimerLabelSmall { font-size: 18px; font-weight: 500; color: %text; }"
        "QLabel#WarningLabel { color: %danger; font-weight: 600; }"
        "QListWidget#Nav { background: transparent; border: none; color: %text; }"
        "QListWidget#Nav::item { padding: 9px 12px; border-radius: 6px; margin: 2px 6px; color: %text; }"
        "QListWidget#Nav::item:selected { background: %accent; color: #ffffff; font-weight: 600; }"
        "QListWidget#Nav::item:hover:!selected { background: %hover; }"
        "QPushButton { padding: 6px 14px; border-radius: 6px; background: %hover; border: 1px solid %border; color: %text; }"
        "QPushButton:hover { background: %pressed; }"
        "QPushButton:pressed { background: %pressed; }"
        "QPushButton:disabled { color: %subtle; background: transparent; border-color: %border; }"
        "QPushButton:focus { border: 1px solid %accent; }"
        "QPushButton#PrimaryButton { background: %accent; border: 1px solid %accent; color: #ffffff; font-weight: 600; padding: 8px 20px; }"
        "QPushButton#PrimaryButton:hover { background: %accentHover; }"
        "QPushButton#PrimaryButton:disabled { background: %hover; border-color: %border; color: %subtle; }"
        "QPushButton#DangerButton { border-color: %danger; color: %danger; }"
        "QCheckBox, QRadioButton { color: %text; spacing: 6px; background: transparent; }"
        "QCheckBox:disabled, QRadioButton:disabled { color: %subtle; }"
        "QComboBox, QLineEdit, QSpinBox, QDateEdit, QTimeEdit, QTextEdit, QPlainTextEdit {"
        "  background: %card; border: 1px solid %border; border-radius: 6px; padding: 4px 8px; color: %text; }"
        "QComboBox:focus, QLineEdit:focus, QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus { border: 1px solid %accent; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background: %card; color: %text; border: 1px solid %border;"
        "  selection-background-color: %selection; selection-color: #ffffff; outline: none; }"
        "QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled { color: %subtle; background: %alt; }"
        "QTableView, QTreeView, QListView, QTableWidget, QTreeWidget, QListWidget {"
        "  background: %card; alternate-background-color: %alt; border: 1px solid %border; color: %text;"
        "  selection-background-color: %selection; selection-color: #ffffff; gridline-color: %border; }"
        "QTableView::item, QTreeView::item, QListView::item { padding: 3px 6px; }"
        "QHeaderView { background: %card; color: %subtle; }"
        "QHeaderView::section { background: %hover; border: none; border-right: 1px solid %border;"
        "  padding: 5px 8px; color: %subtle; font-weight: 600; }"
        "QTableCornerButton::section { background: %hover; border: none; }"
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: %bg; width: 12px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %border; min-height: 24px; border-radius: 5px; margin: 2px 3px; }"
        "QScrollBar::handle:vertical:hover { background: %subtle; }"
        "QScrollBar:horizontal { background: %bg; height: 12px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: %border; min-width: 24px; border-radius: 5px; margin: 3px 2px; }"
        "QScrollBar::handle:horizontal:hover { background: %subtle; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
        "QMenu { background: %card; color: %text; border: 1px solid %border; border-radius: 6px; padding: 4px; }"
        "QMenu::item { padding: 7px 26px 7px 24px; border-radius: 4px; margin: 1px 2px; color: %text; }"
        "QMenu::item:selected { background: %accent; color: #ffffff; }"
        "QMenu::item:disabled { color: %subtle; }"
        "QMenu::separator { height: 1px; background: %border; margin: 4px 10px; }"
        "QToolTip { background: %card; color: %text; border: 1px solid %border; padding: 4px 6px; }"
        "QStatusBar { background: %card; color: %subtle; border-top: 1px solid %border; }"
        "QDialogButtonBox, QMessageBox { background: %bg; }"
        "QGroupBox { border: 1px solid %border; border-radius: 6px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: %subtle; }"
        "QSplitter::handle { background: %border; }"
        "QProgressBar { background: %card; border: 1px solid %border; border-radius: 5px; color: %text; text-align: center; }"
        "QProgressBar::chunk { background: %accent; border-radius: 4px; }");

    qss.replace(QStringLiteral("%bg"), bg)
        .replace(QStringLiteral("%card"), card)
        .replace(QStringLiteral("%alt"), alt)
        .replace(QStringLiteral("%border"), border)
        .replace(QStringLiteral("%text"), text)
        .replace(QStringLiteral("%subtle"), subtle)
        .replace(QStringLiteral("%accentHover"), accentHover)
        .replace(QStringLiteral("%accent"), accent)
        .replace(QStringLiteral("%hover"), hover)
        .replace(QStringLiteral("%pressed"), pressed)
        .replace(QStringLiteral("%danger"), danger)
        .replace(QStringLiteral("%selection"), selection);
    return qss;
}

} // namespace kaeru
