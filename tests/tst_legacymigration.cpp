// SPDX-License-Identifier: GPL-3.0-or-later
// Moving the files of V3 Keyboard (up to 0.6.1) to Tastra's folders.
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "core/legacymigration.h"

namespace
{
void writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray("<missing>");
    return file.readAll();
}
}

class TestLegacyMigration : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_home;
    Tastra::MigrationRoots roots() const
    {
        return {m_home.filePath(QStringLiteral("config")), m_home.filePath(QStringLiteral("data")),
                m_home.filePath(QStringLiteral("state"))};
    }
    QString config(const QString &rel) const { return m_home.filePath(QStringLiteral("config/") + rel); }
    QString data(const QString &rel) const { return m_home.filePath(QStringLiteral("data/") + rel); }
    QString state(const QString &rel) const { return m_home.filePath(QStringLiteral("state/") + rel); }

private slots:
    void init()
    {
        QDir(m_home.path()).removeRecursively();
        QDir().mkpath(m_home.path());
    }

    void movesEveryOldFolderAndLeavesLinksForRollback()
    {
        writeFile(config(QStringLiteral("v3-keyboard/dictionary.txt")), "Ольденбург\n");
        writeFile(config(QStringLiteral("v3-keyboard/shortcuts.txt")), "omw = on my way\n");
        writeFile(data(QStringLiteral("v3-keyboard/dictionaries/uk_UA.dic")), "dic");
        writeFile(data(QStringLiteral("v3-keyboard/backups/v3-keyboard.before-0.6.1.1")), "old binary");
        writeFile(state(QStringLiteral("v3-keyboard/trace.enable")), "");

        const QStringList moved = Tastra::migrateLegacyPaths(roots());
        QVERIFY2(!moved.isEmpty(), "nothing reported");

        QCOMPARE(readFile(config(QStringLiteral("tastra/dictionary.txt"))), QByteArray("Ольденбург\n"));
        QCOMPARE(readFile(config(QStringLiteral("tastra/shortcuts.txt"))), QByteArray("omw = on my way\n"));
        QCOMPARE(readFile(data(QStringLiteral("tastra/dictionaries/uk_UA.dic"))), QByteArray("dic"));
        QCOMPARE(readFile(data(QStringLiteral("tastra/backups/v3-keyboard.before-0.6.1.1"))), QByteArray("old binary"));
        QVERIFY(QFile::exists(state(QStringLiteral("tastra/trace.enable"))));

        // A rolled-back 0.6.1 binary still reads and writes the same files.
        for (const QString &old : {config(QStringLiteral("v3-keyboard")), data(QStringLiteral("v3-keyboard")),
                                   state(QStringLiteral("v3-keyboard"))}) {
            QVERIFY2(QFileInfo(old).isSymLink(), qPrintable(old));
        }
        QCOMPARE(readFile(config(QStringLiteral("v3-keyboard/dictionary.txt"))), QByteArray("Ольденбург\n"));
    }

    void carriesSettingsOverToTheNewFile()
    {
        {
            QSettings old(config(QStringLiteral("V3Keyboard/V3 Keyboard.conf")), QSettings::IniFormat);
            old.setValue(QStringLiteral("theme"), QStringLiteral("dark"));
            old.setValue(QStringLiteral("clipboardPinned"), QStringList{QStringLiteral("pinned text")});
        }
        Tastra::migrateLegacyPaths(roots());

        QSettings now(config(QStringLiteral("tastra/tastra.conf")), QSettings::IniFormat);
        QCOMPARE(now.value(QStringLiteral("theme")).toString(), QStringLiteral("dark"));
        QCOMPARE(now.value(QStringLiteral("clipboardPinned")).toStringList(), QStringList{QStringLiteral("pinned text")});
        // The old file stays for a rolled-back binary.
        QVERIFY(QFile::exists(config(QStringLiteral("V3Keyboard/V3 Keyboard.conf"))));
    }

    void neverOverwritesWhatTastraAlreadyHas()
    {
        writeFile(config(QStringLiteral("tastra/dictionary.txt")), "new\n");
        writeFile(config(QStringLiteral("tastra/tastra.conf")), "[General]\ntheme=light\n");
        writeFile(config(QStringLiteral("v3-keyboard/dictionary.txt")), "old\n");
        writeFile(config(QStringLiteral("v3-keyboard/shortcuts.txt")), "brb = be right back\n");
        writeFile(config(QStringLiteral("V3Keyboard/V3 Keyboard.conf")), "[General]\ntheme=dark\n");

        Tastra::migrateLegacyPaths(roots());

        QCOMPARE(readFile(config(QStringLiteral("tastra/dictionary.txt"))), QByteArray("new\n"));
        QCOMPARE(readFile(config(QStringLiteral("tastra/tastra.conf"))), QByteArray("[General]\ntheme=light\n"));
        // What did not clash is merged in.
        QCOMPARE(readFile(config(QStringLiteral("tastra/shortcuts.txt"))), QByteArray("brb = be right back\n"));
        // The clashing old file is kept where it was, so nothing is lost.
        QVERIFY(!QFileInfo(config(QStringLiteral("v3-keyboard"))).isSymLink());
        QCOMPARE(readFile(config(QStringLiteral("v3-keyboard/dictionary.txt"))), QByteArray("old\n"));
    }

    void mergesNestedFoldersCreatedBeforeTheFirstStart()
    {
        // The installer may provision dictionaries into the new folder first.
        writeFile(data(QStringLiteral("tastra/dictionaries/de_DE.dic")), "de");
        writeFile(data(QStringLiteral("v3-keyboard/dictionaries/uk_UA.dic")), "uk");
        writeFile(data(QStringLiteral("v3-keyboard/voice/ggml-base-q5_1.bin")), "model");

        Tastra::migrateLegacyPaths(roots());

        QCOMPARE(readFile(data(QStringLiteral("tastra/dictionaries/de_DE.dic"))), QByteArray("de"));
        QCOMPARE(readFile(data(QStringLiteral("tastra/dictionaries/uk_UA.dic"))), QByteArray("uk"));
        QCOMPARE(readFile(data(QStringLiteral("tastra/voice/ggml-base-q5_1.bin"))), QByteArray("model"));
        // Everything moved, so the old folder became a link.
        QVERIFY(QFileInfo(data(QStringLiteral("v3-keyboard"))).isSymLink());
    }

    void secondRunAndFreshInstallsDoNothing()
    {
        QVERIFY(Tastra::migrateLegacyPaths(roots()).isEmpty());
        QVERIFY(!QFileInfo::exists(config(QStringLiteral("tastra"))));

        writeFile(config(QStringLiteral("v3-keyboard/dictionary.txt")), "x\n");
        QVERIFY(!Tastra::migrateLegacyPaths(roots()).isEmpty());
        QVERIFY(Tastra::migrateLegacyPaths(roots()).isEmpty());
        QCOMPARE(readFile(config(QStringLiteral("tastra/dictionary.txt"))), QByteArray("x\n"));
    }
};

QTEST_GUILESS_MAIN(TestLegacyMigration)
#include "tst_legacymigration.moc"
