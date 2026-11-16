// Kaeru unit tests — official frog icon loading (PNG-only runtime rendering)
#include "ui/Icons.h"

#include <QFile>
#include <QImage>
#include <QSet>
#include <QtTest>

using namespace kaeru;

class TestIcons final : public QObject {
    Q_OBJECT

private slots:
    void embeddedPngVariantsExist()
    {
        for (const QString& state : { QStringLiteral("awake"), QStringLiteral("focused"),
                 QStringLiteral("sleep") }) {
            for (int px : { 16, 24, 32, 48, 64, 128, 256 }) {
                const QString path = QStringLiteral(":/icons/frog/png/%1-%2.png").arg(state).arg(px);
                QVERIFY2(QFile::exists(path), qPrintable(path));
                const QImage image(path);
                QVERIFY2(!image.isNull(), qPrintable(path));
                QCOMPARE(image.size(), QSize(px, px));
            }
        }
    }

    void stateIconsProvideRasterSizes()
    {
        for (IconState state : { IconState::Inactive, IconState::Awake, IconState::Focus,
                 IconState::Recording, IconState::Paused, IconState::Error }) {
            const QIcon icon = Icons::stateIcon(state);
            QVERIFY(!icon.isNull());
            const QList<QSize> sizes = icon.availableSizes();
            QVERIFY2(!sizes.isEmpty(), "state icon has no raster sizes");
            QSet<int> widths;
            for (const QSize& s : sizes)
                widths.insert(s.width());
            QVERIFY(widths.contains(16));
            QVERIFY(widths.contains(64));
            QVERIFY(widths.contains(256));
        }
    }

    void stateSelectionUsesDistinctArtwork()
    {
        const QIcon awake = Icons::stateIcon(IconState::Awake);
        const QIcon focused = Icons::stateIcon(IconState::Focus);
        const QIcon sleep = Icons::stateIcon(IconState::Inactive);
        const QImage awakeImage = awake.pixmap(64, 64).toImage();
        const QImage focusedImage = focused.pixmap(64, 64).toImage();
        const QImage sleepImage = sleep.pixmap(64, 64).toImage();
        QVERIFY(!awakeImage.isNull());
        QVERIFY(!focusedImage.isNull());
        QVERIFY(!sleepImage.isNull());
        QVERIFY(awakeImage != focusedImage);
        QVERIFY(awakeImage != sleepImage);
        QVERIFY(focusedImage != sleepImage);
    }

    void focusedArtworkIsNotBlack()
    {
        // Regression for the Qt 6.2 SVG mis-render: the focused frog must keep
        // its original colors, not collapse to a black silhouette.
        const QImage image = Icons::stateIcon(IconState::Focus).pixmap(64, 64).toImage();
        QVERIFY(!image.isNull());
        bool hasGreen = false;
        bool hasLight = false;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x, y);
                if (c.alpha() == 0)
                    continue;
                if (c.green() > 90 && c.green() > c.red() + 20 && c.green() > c.blue() + 20)
                    hasGreen = true;
                if (c.red() > 180 && c.green() > 180 && c.blue() > 180)
                    hasLight = true;
            }
        }
        QVERIFY2(hasGreen, "focused frog lost its green body color");
        QVERIFY2(hasLight, "focused frog lost its light eyes/highlights");
    }
};

QTEST_MAIN(TestIcons)
#include "test_icons.moc"
