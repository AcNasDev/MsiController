#include <QElapsedTimer>
#include <QFile>
#include <QTest>

#include "systemaccess.h"

class SystemAccessTest : public QObject {
    Q_OBJECT

private slots:
    void writeTextReportsFlushFailure() {
        if (!QFile::exists(QStringLiteral("/dev/full"))) {
            QSKIP("/dev/full is unavailable");
        }
        QtFileSystemAccess files;
        QString error;
        QVERIFY(!files.writeText(QStringLiteral("/dev/full"), QStringLiteral("hello"), &error));
        QVERIFY(!error.isEmpty());
    }

    void timedOutProcessIsStopped() {
        QtProcessRunner runner;
        QElapsedTimer elapsed;
        elapsed.start();

        const ProcessResult result = runner.run(QStringLiteral("/bin/sleep"), {QStringLiteral("2")}, 20);
        QVERIFY(result.started);
        QVERIFY(!result.finished);
        QCOMPARE(result.exitCode, -1);
        QVERIFY(result.standardError.contains("timed out"));
        QVERIFY(elapsed.elapsed() < 1000);
    }
};

QTEST_MAIN(SystemAccessTest)
#include "systemaccesstest.moc"
