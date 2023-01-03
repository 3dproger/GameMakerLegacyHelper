#include "gms1corrector.h"
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDirIterator>
#include <QDebug>
#include <QXmlStreamReader>
#include <QDomDocument>
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
            QFile sourceFile(eventFile.absoluteFilePath());
            if (!sourceFile.open(QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text))
            {
                log(QString("Failed to open file \"%1\"").arg(eventFile.absoluteFilePath()));
                continue;
            }

            QString sourceEventType;
            QString sourceEventId;
            QString sourceEventWith;

            QStringList sourceCodes;

            bool isCode = false;

            QXmlStreamReader xml(&sourceFile);
            while (!xml.atEnd())
            {
                const QXmlStreamAttributes& attributes = xml.attributes();

                if (sourceEventType.isEmpty() && xml.name() == "event")
                {
                    sourceEventType = attributes.value("category").toString();
                    sourceEventId = attributes.value("id").toString();
                    sourceEventWith = attributes.value("with").toString();
                }

                if (xml.name() == "kind")
                {
                    isCode = xml.readElementText(QXmlStreamReader::ReadElementTextBehaviour::IncludeChildElements) == "CODE";
                }

                if (isCode && xml.name() == "argument" && attributes.value("kind") == "STRING")
                {
                    sourceCodes.append(xml.readElementText(QXmlStreamReader::ReadElementTextBehaviour::IncludeChildElements));
                }

                xml.readNext();
            }

            if (sourceCodes.isEmpty())
            {
                continue;
            }

            bool needSaveFile = false;

            const QString destFileName = gms1folder + "/objects/" + objectName + ".object.gmx";
            QFile destFileRead(destFileName);
            if (!destFileRead.exists())
            {
                log(QString("File \"%1\" not found").arg(destFileName));
                continue;
            }

            if (!destFileRead.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                log(QString("Failed to open file \"%1\"").arg(destFileName));
                continue;
            }

            QDomDocument dom;
            if (!dom.setContent(&destFileRead))
            {
                log(QString("Failed to load DOM content from \"%1\"").arg(destFileName));
                continue;
            }

            const QDomNodeList objectNodes = dom.elementsByTagName("object");
            if (objectNodes.isEmpty())
            {
                log(QString("No 'object' tag in destination file \"%1\"").arg(destFileName));
                continue;
            }

            const QDomNodeList eventsNodes = objectNodes.at(0).toElement().elementsByTagName("events");
            if (eventsNodes.isEmpty())
            {
                log(QString("No 'events' tag in destination file \"%1\"").arg(destFileName));
                continue;
            }

            const QDomNodeList eventNodes = eventsNodes.at(0).toElement().elementsByTagName("event");
            for (int i = 0; i < eventNodes.count(); ++i)
            {
                const QDomElement event = eventNodes.at(i).toElement();
                const QString destEventType = event.attribute("eventtype");
                const QString destEventId = event.attribute("enumb");
                const QString destEventWith = event.attribute("ename");

                if (gms1EventTypeToGmk(destEventType).isEmpty())
                {
                    log(QString("Unknown event type \"%1\" in object \"%2\"").arg(destEventType).arg(objectName));
                    continue;
                }

                if (gms1EventTypeToGmk(destEventType) != sourceEventType || destEventId != sourceEventId || destEventWith != sourceEventWith)
                {
                    continue;
                }

                int sourceCodeIndex = 0;
                int destCodes = 0;
                const QDomNodeList actionNodes = event.elementsByTagName("action");
                for (int i = 0; i < actionNodes.count(); ++i)
                {
                    const QDomElement action = actionNodes.at(i).toElement();
                    const QDomNodeList kindNodes = action.elementsByTagName("kind");
                    if (kindNodes.isEmpty())
                    {
                        continue;
                    }

                    if (kindNodes.at(0).toElement().text() == "7")
                    {
                        destCodes++;
                    }
                    else
                    {
                        continue;
                    }

                    const QDomNodeList argumentsNodes = action.elementsByTagName("arguments");
                    if (argumentsNodes.isEmpty())
                    {
                        log(QString("'arguments' not found in destination object \"%1\"").arg(objectName));
                        continue;
                    }

                    const QDomNodeList argumentNodes = argumentsNodes.at(0).toElement().elementsByTagName("argument");
                    if (argumentNodes.isEmpty())
                    {
                        log(QString("'argument' not found in destination object \"%1\"").arg(objectName));
                        continue;
                    }

                    const QDomNodeList stringNodes = argumentNodes.at(0).toElement().elementsByTagName("string");
                    if (argumentNodes.isEmpty())
                    {
                        log(QString("'string' not found in destination object \"%1\"").arg(objectName));
                        continue;
                    }

                    if (sourceCodeIndex >= sourceCodes.count())
                    {
                        log(QString("Fewer source codes than destination codes in object \"%1\"").arg(objectName));
                        continue;
                    }

                    stringNodes.at(0).toElement().setNodeValue(sourceCodes.at(sourceCodeIndex));

                    needSaveFile = true;

                    sourceCodeIndex++;
                }

                if (destCodes != sourceCodes.count())
                {
                    log(QString("The number of source codes (%1) does not match the number of destination codes (%2) in object \"%3\", event: %4 (%5)")
                        .arg(sourceCodes.count()).arg(destCodes).arg(objectName, sourceEventType, destEventType));
                }
            }

            if (needSaveFile)
            {
                destFileRead.close();

                QFile destFileWrite(destFileName);
                if (!destFileWrite.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
                {
                    log(QString("Failed to open file \"%1\"").arg(destFileName));
                    continue;
                }

                destFileWrite.write(dom.toString().toUtf8()
                                    .replace("&#xd;", "") // TODO: A crutch that fixes the appearance of a large number of such substrings out of nowhere
                                    );

                log(QString("Updated event code %1 in object \"%2\"").arg(sourceEventType, objectName));
            }
        }
    }
}

void GMS1Corrector::copyRoomCodes(const QString &gmkSplitOutput, const QString &gms1folder)
{

}

QString GMS1Corrector::gms1EventTypeToGmk(const QString& type)
{
    if (type == "0")
    {
        return "CREATE";
    }

    if (type == "1")
    {
        return "DESTROY";
    }

    if (type == "2")
    {
        return "ALARM";
    }

    if (type == "3")
    {
        return "STEP";
    }

    if (type == "4")
    {
        return "COLLISION";
    }

    if (type == "5")
    {
        return "KEYBOARD";
    }

    if (type == "6")
    {
        //TODO:
    }

    if (type == "7")
    {
        return "OTHER";
    }

    if (type == "8")
    {
        return "DRAW";
    }

    if (type == "9")
    {
        return "KEYPRESS";
    }

    if (type == "10")
    {
        return "KEYRELEASE";
    }

    return QString();
}
