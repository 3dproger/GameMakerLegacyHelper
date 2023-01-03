#include "gms1corrector.h"
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDirIterator>
#include <QDebug>
#include <QXmlStreamReader>
#include <thread>

namespace
{

static std::function<void(const QString&)> logCallback = nullptr;

bool removeDir(QString dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            if (info.isDir())
            {
                result = removeDir(info.absoluteFilePath());
            }
            else
            {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
            {
                return result;
            }
        }

        result = dir.rmdir(dirName);
    }

    return result;
}

}

void GMS1Corrector::setLogCallback(std::function<void (const QString &)> callback)
{
    logCallback = callback;
}

void GMS1Corrector::convertAnsiToUtf8(const QString &gmkFileName, const QString &gms1folder)
{
    QFileInfo gmkSplit(QCoreApplication::applicationDirPath() + "/GmkSplitter.v0.18/gmksplit.exe");
    if (!gmkSplit.exists())
    {
        log(QString("File \"%1\" not found").arg(gmkSplit.absoluteFilePath()));
        return;
    }

    QFileInfo gmk(gmkFileName);
    if (!gmk.exists())
    {
        log(QString("GMK file \"%1\" not found").arg(gmkFileName));
        return;
    }

    QDir root(gms1folder);
    if (!root.exists())
    {
        log(QString("Folder \"%1\" not exists!").arg(gms1folder));
        return;
    }

    static const QString FileProjectSuffix = "GMX";

    bool foundProjectFile = false;

    const QFileInfoList rootFiles = root.entryInfoList(QDir::Filter::Files);
    for (const QFileInfo& fileInfo : rootFiles)
    {
        if (fileInfo.suffix().toUpper() == FileProjectSuffix)
        {
            foundProjectFile = true;
            break;
        }
    }

    if (!foundProjectFile)
    {
        log(QString("GMS1 Folder project does not contain a project file %1").arg(FileProjectSuffix));
        return;
    }

    const QString temp = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::TempLocation);
    if (temp.isEmpty())
    {
        log("Writable temp directory not found");
        return;
    }

    const QString gmkSplitOutput = temp + "/gmksplit_output";

    log(QString("GmkSplit output: \"%1\"").arg(gmkSplitOutput));

    if (!removeDir(gmkSplitOutput))
    {
        log(QString("Problem deleting folder \"%1\"").arg(gmkSplitOutput));
    }

    QProcess process;

    QObject::connect(&process, &QProcess::readyRead, [&process]()
    {
        log(process.readAll());
    });

    process.start(gmkSplit.absoluteFilePath(), { gmk.absoluteFilePath(), gmkSplitOutput });
    process.waitForFinished(-1);
    if (process.exitStatus() == QProcess::ExitStatus::CrashExit)
    {
        log(QString("Failed to execute GmkSplit, exit code: %1").arg(process.exitCode()));
        return;
    }

    copyScripts(gmkSplitOutput, gms1folder);
    copyObjectCodes(gmkSplitOutput, gms1folder);
    copyRoomCodes(gmkSplitOutput, gms1folder);

    log("Done!");
}

void GMS1Corrector::log(const QString &text)
{
    qDebug(text.toUtf8());

    if (logCallback)
    {
        logCallback(text);
    }
}

void GMS1Corrector::copyScripts(const QString& gmkSplitOutput, const QString& gms1folder)
{
    QDirIterator it(gmkSplitOutput + "/Scripts", QStringList() << "*.gml", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QFileInfo sourceFile(it.next());
        QFileInfo destFile(gms1folder + "/scripts/" + sourceFile.fileName());
        if (!destFile.exists())
        {
            log(QString("Destination file \"%1\" not found").arg(destFile.absoluteFilePath()));
            continue;
        }

        if (destFile.exists() && !QFile::remove(destFile.absoluteFilePath()))
        {
            log(QString("Failed to remove file \"%1\"").arg(destFile.absoluteFilePath()));
        }

        if (QFile::copy(sourceFile.absoluteFilePath(), destFile.absoluteFilePath()))
        {
            log(QString("Copied \"%1\" to \"%2\"").arg(sourceFile.absoluteFilePath(), destFile.absoluteFilePath()));
        }
        else
        {
            log(QString("Failed to copy \"%1\" to \"%2\"").arg(sourceFile.absoluteFilePath(), destFile.absoluteFilePath()));
        }
    }
}

void GMS1Corrector::copyObjectCodes(const QString &gmkSplitOutput, const QString &gms1folder)
{
    QDirIterator objectsDirIt(gmkSplitOutput + "/Objects", QStringList() << "*.events", QDir::Filter::Dirs, QDirIterator::Subdirectories);
    while (objectsDirIt.hasNext())
    {
        const QDir objectDir(objectsDirIt.next());
        const QString objectName = objectDir.dirName().left(objectDir.dirName().length() - 7);

        const QFileInfoList eventsFiles = objectDir.entryInfoList(QDir::Filter::Files);
        for (const QFileInfo& eventFile : eventsFiles)
        {
            QFile file(eventFile.absoluteFilePath());
            if (!file.open(QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text))
            {
                log(QString("Failed to open file \"%1\"").arg(eventFile.absoluteFilePath()));
                continue;
            }

            QString eventType;
            QString eventId;
            QString eventWith;

            QStringList codes;

            QXmlStreamReader xml(&file);
            while (!xml.atEnd())
            {
                const QXmlStreamAttributes& attributes = xml.attributes();

                if (eventType.isEmpty() && xml.name() == "event")
                {
                    eventType = attributes.value("category").toString();
                    eventId = attributes.value("id").toString();
                    eventWith = attributes.value("with").toString();
                }

                if (xml.name() == "argument" && attributes.value("kind") == "STRING")
                {
                    codes.append(xml.readElementText());
                }

                xml.readNext();
            }

            qDebug() << eventType << eventId << eventWith;
        }
    }
}

void GMS1Corrector::copyRoomCodes(const QString &gmkSplitOutput, const QString &gms1folder)
{

}
