#include "gms1corrector.h"
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDirIterator>
#include <QDebug>
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

        QList<SourceEvent> events;

        const QFileInfoList eventsFiles = objectDir.entryInfoList(QDir::Filter::Files);
        for (const QFileInfo& eventFile : eventsFiles)
        {
            QFile sourceFile(eventFile.absoluteFilePath());
            if (!sourceFile.open(QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text))
            {
                log(QString("Failed to open file \"%1\"").arg(eventFile.absoluteFilePath()));
                continue;
            }

            QDomDocument sourceDom;
            if (!sourceDom.setContent(&sourceFile))
            {
                log(QString("Failed to load DOM content from \"%1\"").arg(sourceFile.fileName()));
                continue;
            }

            const QDomNode event = sourceDom.namedItem("event");

            SourceEvent sourceEvent(event.attributes().namedItem("category").nodeValue(),
                                    event.attributes().namedItem("id").nodeValue(),
                                    event.attributes().namedItem("with").nodeValue());

            const QDomNodeList actions = event.namedItem("actions").childNodes();
            for (int i = 0; i < actions.count(); ++i)
            {
                const QDomNode action = actions.at(i);
                if (action.namedItem("kind").firstChild().nodeValue() != "CODE")
                {
                    continue;
                }

                sourceEvent.codes.append(action.namedItem("arguments").childNodes().at(0).firstChild().nodeValue());
            }

            events.append(sourceEvent);
        }

        copyObjectCode(objectName, gms1folder, events);
    }
}

void GMS1Corrector::copyObjectCode(const QString &objectName, const QString& gms1folder, const QList<SourceEvent> &sourceEvents)
{
    if (sourceEvents.isEmpty())
    {
        return;
    }

    bool needSaveFile = false;

    const QString destFileName = gms1folder + "/objects/" + objectName + ".object.gmx";
    QFile destFileRead(destFileName);
    if (!destFileRead.exists())
    {
        log(QString("File \"%1\" not found").arg(destFileName));
        return;
    }

    if (!destFileRead.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        log(QString("Failed to open file \"%1\" for read").arg(destFileName));
        return;
    }

    QDomDocument dom;
    if (!dom.setContent(&destFileRead))
    {
        log(QString("Failed to load DOM content from \"%1\"").arg(destFileName));
        return;
    }

    const QDomNodeList events = dom.namedItem("object").namedItem("events").childNodes();

    for (int i = 0; i < events.count(); ++i)
    {
        const QDomNode event = events.at(i);

        const QString destEventType = event.attributes().namedItem("eventtype").nodeValue();
        const QString destEventId = event.attributes().namedItem("enumb").nodeValue();
        const QString destEventWith = event.attributes().namedItem("ename").nodeValue();

        if (gms1EventTypeToGmk(destEventType).isEmpty())
        {
            log(QString("Unknown event type \"%1\" in object \"%2\"").arg(destEventType, objectName));
            continue;
        }

        const SourceEvent* sourceEvent = nullptr;

        for (const SourceEvent& sourceEvent_ : sourceEvents)
        {
            if (gms1EventTypeToGmk(destEventType) == sourceEvent_.type && destEventId == sourceEvent_.id && destEventWith == sourceEvent_.with)
            {
                sourceEvent = &sourceEvent_;
            }
        }

        if (!sourceEvent)
        {
            log(QString("Not found GM7/8 event for GMS1 event \"%1\" (%2) in object \"%3\"").arg(destEventType, destEventType, objectName));
            continue;
        }

        int sourceCodeIndex = 0;
        int destCodes = 0;
        const QDomNodeList actionNodes = event.childNodes();
        for (int i = 0; i < actionNodes.count(); ++i)
        {
            const QDomNode action = actionNodes.at(i);
            if (action.nodeName() != "action")
            {
                continue;
            }

            if (action.namedItem("kind").firstChild().nodeValue() == "7")
            {
                destCodes++;
            }
            else
            {
                continue;
            }

            QDomNode codeNode = action.namedItem("arguments").namedItem("argument").namedItem("string").firstChild();

            if (sourceCodeIndex >= sourceEvent->codes.count())
            {
                log(QString("Fewer GM7/8 codes than GMS1 codes in event \"%1\" (%2) in object \"%3\"").arg(destEventType, destEventType, objectName));
                continue;
            }

            codeNode.setNodeValue(sourceEvent->codes.at(sourceCodeIndex));

            needSaveFile = true;

            sourceCodeIndex++;
        }

        if (destCodes != sourceEvent->codes.count())
        {
            log(QString("The number of GM7/8 codes (%1) does not match the number of GMS1 codes (%2) in object \"%3\", event: %4 (%5)")
                .arg(sourceEvent->codes.count()).arg(destCodes).arg(objectName, sourceEvent->type, destEventType));
        }
    }

    if (needSaveFile)
    {
        destFileRead.close();

        QFile destFileWrite(destFileName);
        if (!destFileWrite.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            log(QString("Failed to open file \"%1\" for write").arg(destFileName));
            return;
        }

        destFileWrite.write(dom.toString().toUtf8());

        log(QString("Corrected file \"%1\"").arg(destFileName));
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

void GMS1Corrector::domToStringCorrected(QString& result, const QDomNode& node, int intend)
{
    if (node.isComment())
    {
        return;
    }

    if (!node.nodeName().startsWith("#"))
    {
        if (result.endsWith(">"))
        {
            result += "\n";

            for (int i = 0; i < intend; ++i)
            {
                result += "  ";
            }
        }

        result += "<" + node.nodeName();

        const QDomNamedNodeMap attributes = node.attributes();
        for (int i = 0; i < attributes.count(); ++i)
        {
            const QDomNode attribute = attributes.item(i);

            result += " " + attribute.nodeName() + "=\"" + attribute.nodeValue() + "\"";
        }

        result += ">";
    }

    if (!node.nodeValue().isEmpty())
    {
        result += node.nodeValue();
    }

    const QDomNodeList children = node.childNodes();
    for (int i = 0; i < children.count(); ++i)
    {
        const QDomNode child = children.at(i);
        if (child.isText() )//&& !node.nodeValue().isEmpty())
        {
            continue;
        }

        domToStringCorrected(result, child, intend + 1);
    }

    if (!node.nodeName().startsWith("#"))
    {
        result += "</" + node.nodeName() + ">\n";

        for (int i = 0; i < intend; ++i)
        {
            result += "  ";
        }
    }
}
