// Kaeru unit tests — complete light/dark/system appearance (WP-09)
#include "ui/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QtTest>

using namespace kaeru;

class TestTheme final : public QObject {
    Q_OBJECT

private slots:
    void darkAppliesPaletteAndStylesheet()
    {
        Theme::instance().apply(QStringLiteral("dark"), ColorScheme::Light);
        QVERIFY(Theme::instance().isDark());
        QVERIFY(qApp->palette().color(QPalette::Window).lightness() < 80);
        QVERIFY(qApp->palette().color(QPalette::WindowText).lightness() > 180);
        const QString qss = qApp->styleSheet();
        QVERIFY(qss.contains(QStringLiteral("QScrollBar")));
        QVERIFY(qss.contains(QStringLiteral("QMenu::item")));
        QVERIFY(qss.contains(QStringLiteral("QComboBox QAbstractItemView")));
        QVERIFY(qss.contains(QStringLiteral("QHeaderView::section")));
    }

    void lightAppliesPalette()
    {
        Theme::instance().apply(QStringLiteral("light"), ColorScheme::Dark);
        QVERIFY(!Theme::instance().isDark());
        QVERIFY(qApp->palette().color(QPalette::Window).lightness() > 180);
        QVERIFY(qApp->palette().color(QPalette::WindowText).lightness() < 100);
    }

    void systemFollowsDesktopScheme()
    {
        Theme::instance().apply(QStringLiteral("system"), ColorScheme::Dark);
        QVERIFY(Theme::instance().isDark());
        Theme::instance().apply(QStringLiteral("system"), ColorScheme::Light);
        QVERIFY(!Theme::instance().isDark());
    }

    void changedSignalFiresOnlyOnRealChange()
    {
        Theme::instance().apply(QStringLiteral("light"), ColorScheme::Light);
        QSignalSpy spy(&Theme::instance(), &Theme::changed);
        Theme::instance().apply(QStringLiteral("light"), ColorScheme::Light);
        QCOMPARE(spy.count(), 0);
        Theme::instance().apply(QStringLiteral("dark"), ColorScheme::Light);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestTheme)
#include "test_theme.moc"
