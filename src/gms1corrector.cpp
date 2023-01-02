#include "gms1corrector.h"
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>

namespace
{

static std::function<void(const QString&)> logCallback = nullptr;

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

    log("Done!");
}

void GMS1Corrector::log(const QString &text)
{
    if (logCallback)
    {
        logCallback(text);
    }
    else
    {
        wprintf(text.toStdWString().c_str());
        wprintf(L"\n");
    }
}
