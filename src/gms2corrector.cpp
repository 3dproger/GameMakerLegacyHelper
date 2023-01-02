#include "gms2corrector.h"
#include <QDirIterator>
#include <QFile>
#include <QDir>
#include <QTextStream>

namespace
{

static std::function<void(const QString&)> logCallback = nullptr;

}

void GMS2Corrector::setLogCallback(std::function<void (const QString &)> callback)
{
    logCallback = callback;
}

void GMS2Corrector::breakToExit(const QString& gms2folder)
{
    static const QString FileProjectSuffix = "YYP";

    bool foundProjectFile = false;

    QDir root(gms2folder);
    if (!root.exists())
    {
        log(QString("Folder \"%1\" not exists!").arg(gms2folder));
        return;
    }

    const QFileInfoList rootFiles = root.entryInfoList(QDir::Filter::Files);
    for (const QFileInfo& fileInfo : rootFiles)
    {
        if (fileInfo.completeSuffix().toUpper() == FileProjectSuffix)
        {
            foundProjectFile = true;
            break;
        }
    }

    if (!foundProjectFile)
    {
        log(QString("GMS2 Folder project does not contain a project file %1").arg(FileProjectSuffix));
        return;
    }

    QDirIterator it(gms2folder, QStringList() << "*.gml", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString fileName = it.next();

        QByteArray data = readFile(fileName);

        if (!isContainsWord(data, "break"))
        {
            log(QString("Ignore file \"%1\", not contains 'break'").arg(fileName));
            continue;
        }

        if (isContainsWord(data, "for") || isContainsWord(data, "while") || isContainsWord(data, "switch"))
        {
            log(QString("Ignore file \"%1\", contains stop-word").arg(fileName));
            continue;
        }

        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Truncate))
        {
            log(QString("Failed to open file \"%1\"").arg(fileName));
            continue;
        }

        QTextStream out(&file);
        out << data;

        log(QString("Converted file \"%1\"").arg(fileName));
    }

    log("Done!");
}

bool GMS2Corrector::isContainsWord(const QByteArray &text, const QByteArray &word)
{
    if (word.isEmpty() || text.isEmpty())
    {
        return false;
    }

    for (int i = 0; i < text.length(); i += word.length())
    {
        i = text.indexOf(word, i);
        if (i == -1)
        {
            return false;
        }

        const bool noAtLeft = i == 0 || (i > 0 && !QChar(text[i - 1]).isLetterOrNumber());
        const bool noAtRight = i == text.length() - 1 || (i < text.length() - 1 && !QChar(text[text.length() - 1]).isLetterOrNumber());

        if (noAtLeft && noAtRight)
        {
            return true;
        }
    }

    return false;
}

void GMS2Corrector::log(const QString &text)
{
    qDebug(text.toUtf8());

    if (logCallback)
    {
        logCallback(text);
    }
}

QByteArray GMS2Corrector::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        log(QString("Failed to open file \"%1\"").arg(file.fileName()));
        return QByteArray();
    }

    return file.readAll();
}
