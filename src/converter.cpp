#include "converter.h"
#include <QDirIterator>
#include <QFile>
#include <QDebug>
#include <QTranslator>
#include <QDir>

namespace
{

QByteArray readFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file" << file.fileName();
        return QByteArray();
    }

    return file.readAll();
}

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

        if (!data.contains("break"))
        {
            qInfo() << "Ignore file" << fileName << ", not contains 'break'";
            continue;
        }

        if (isContainsWord(data, "for") || isContainsWord(data, "while") || isContainsWord(data, "switch"))
        {
            qInfo() << "Ignore file" << fileName << ", contains stop-word";
            continue;
        }

        //data.replace("break", "exit");

        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Truncate))
        {
            qWarning() << "Failed to open file" << fileName;
            continue;
        }

        QTextStream out(&file);
        out << data;

        qInfo() << "Converted file" << fileName;
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
