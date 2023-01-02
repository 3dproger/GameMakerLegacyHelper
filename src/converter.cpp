#include "converter.h"
#include <QDirIterator>
#include <QFile>
#include <QTranslator>
#include <QDir>
#include <QTextStream>

namespace
{

static std::function<void(const QString&)> logCallback = nullptr;

}

void Converter::setLogCallback(std::function<void (const QString &)> callback)
{
    logCallback = callback;
}

QList<Converter::Note> Converter::breakToExit(const QString& gms2folder_)
{
    const QString gms2folder = gms2folder_.trimmed();
    if (gms2folder.isEmpty())
    {
        return QList<Converter::Note>({ Note(Note::Error, QTranslator::tr("GMS2 Folder project is empty")) });
    }

    static const QString FileProjectSuffix = "YYP";

    bool foundProjectFile = false;

    QDir root(gms2folder);
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
        return QList<Converter::Note>({ Note(Note::Error, QTranslator::tr("GMS2 Folder project does not contain a project file %1").arg(FileProjectSuffix)) });
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

    return QList<Converter::Note>();
}

bool Converter::isContainsWord(const QByteArray &text, const QByteArray &word)
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

void Converter::log(const QString &text)
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

QByteArray Converter::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        log(QString("Failed to open file \"%1\"").arg(file.fileName()));
        return QByteArray();
    }

    return file.readAll();
}
