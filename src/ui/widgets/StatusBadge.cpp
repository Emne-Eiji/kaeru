#include "StatusBadge.h"

#include <QHBoxLayout>
#include <QLabel>

namespace kaeru {

StatusBadge::StatusBadge(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    m_dot = new QLabel(this);
    m_text = new QLabel(this);
    m_dot->setFixedWidth(14);
    m_dot->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_dot);
    layout->addWidget(m_text);
    setAccessibleName(m_text->text());
}

void StatusBadge::setStatus(const QString& word, const QString& colorRole, bool filled)
{
    m_word = word;
    m_role = colorRole;
    m_filled = filled;
    updateUi();
}

QString StatusBadge::accessibleText() const
{
    return m_word;
}

void StatusBadge::updateUi()
{
    // Glyphs are shape-distinct: filled dot, hollow ring, dash, exclamation.
    QString glyph;
    QColor color;
    if (m_role == QStringLiteral("ok")) {
        color = palette().color(QPalette::WindowText).lightness() < 128 ? QColor(0x2e, 0x7d, 0x32) : QColor(0x66, 0xbb, 0x6a);
        glyph = m_filled ? QStringLiteral("●") : QStringLiteral("○");
    } else if (m_role == QStringLiteral("warn")) {
        color = palette().color(QPalette::WindowText).lightness() < 128 ? QColor(0xb7, 0x79, 0x1f) : QColor(0xf0, 0xb4, 0x29);
        glyph = m_filled ? QStringLiteral("◐") : QStringLiteral("◑");
    } else if (m_role == QStringLiteral("bad")) {
        color = palette().color(QPalette::WindowText).lightness() < 128 ? QColor(0xc0, 0x39, 0x2b) : QColor(0xef, 0x81, 0x77);
        glyph = QStringLiteral("✕");
    } else if (m_role == QStringLiteral("info")) {
        color = palette().color(QPalette::WindowText).lightness() < 128 ? QColor(0x1a, 0x73, 0xc8) : QColor(0x7c, 0xb1, 0xf0);
        glyph = QStringLiteral("◆");
    } else {
        color = palette().color(QPalette::WindowText);
        glyph = QStringLiteral("–");
    }
    m_dot->setText(glyph);
    QPalette p = m_dot->palette();
    p.setColor(QPalette::WindowText, color);
    m_dot->setPalette(p);
    m_text->setText(m_word);
    setAccessibleName(m_word);
}

} // namespace kaeru
