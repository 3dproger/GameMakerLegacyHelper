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
void log(const QString &text)
{
    qDebug(text.toUtf8());

    if (logCallback)
    {
        logCallback(text);
    }
}

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

QString gms1EventTypeToGmk(const QString& type)
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

    log(QString("GmkSplit started (%1)").arg(gmkSplit.absoluteFilePath()));

    process.waitForFinished(-1);
    if (process.exitStatus() == QProcess::ExitStatus::CrashExit)
    {
        log(QString("Failed to execute GmkSplit, exit code: %1").arg(process.exitCode()));
        return;
    }

    log("GmkSplit finished");

    copyScripts(gmkSplitOutput, gms1folder);
    correctObjectsCodes(gmkSplitOutput, gms1folder);
    correctRoomsCreationCode(gmkSplitOutput, gms1folder);

    log("Done!");
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
            log(QString("Corrected script code \"%1\"").arg(sourceFile.baseName()));
        }
        else
        {
            log(QString("Failed to copy \"%1\" to \"%2\"").arg(sourceFile.absoluteFilePath(), destFile.absoluteFilePath()));
        }
    }
}

void GMS1Corrector::correctObjectsCodes(const QString &gmkSplitOutput, const QString &gms1folder)
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
            if (!sourceFile.open(QFile::OpenModeFlag::ReadOnly | QFile::OpenModeFlag::Text))
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

        correctObjectCodes(objectName, gms1folder, events);
    }
}

void GMS1Corrector::correctObjectCodes(const QString &objectName, const QString& gms1folder, const QList<SourceEvent> &sourceEvents)
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

    if (!destFileRead.open(QFile::ReadOnly | QFile::Text))
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
        if (!destFileWrite.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        {
            log(QString("Failed to open file \"%1\" for write").arg(destFileName));
            return;
        }

        destFileWrite.write(dom.toString().toUtf8());

        log(QString("Corrected object code \"%1\"").arg(objectName));
    }
}

void GMS1Corrector::correctRoomsCreationCode(const QString &gmkSplitOutput, const QString &gms1folder)
{
    QDirIterator roomsDirIt(gmkSplitOutput + "/Rooms", QStringList() << "*.xml", QDir::Filter::Files, QDirIterator::Subdirectories);
    while (roomsDirIt.hasNext())
    {
        const QString sourceFileName = roomsDirIt.next();
        const QFileInfo sourceRoomFileInfo(sourceFileName);
        if (sourceRoomFileInfo.fileName() == "_resources.list.xml")
        {
            continue;
        }

        const QString roomName = sourceRoomFileInfo.fileName().left(sourceRoomFileInfo.fileName().length() - 4);
        QFile sourceFile(sourceFileName);
        if (!sourceFile.open(QFile::ReadOnly | QFile::Text))
        {
            log(QString("Failed to open file \"%1\" for read").arg(sourceFileName));
            return;
        }

        QDomDocument sourceDom;
        if (!sourceDom.setContent(&sourceFile))
        {
            log(QString("Failed to load DOM content from \"%1\"").arg(sourceFile.fileName()));
            continue;
        }

        const QString code = sourceDom.namedItem("room").namedItem("creationCode").firstChild().nodeValue();

        QList<Instance> instances;

        const QDomNodeList domInstances = sourceDom.namedItem("room").namedItem("instances").childNodes();
        for (int i = 0; i < domInstances.count(); ++i)
        {
            const QDomNode domInstance = domInstances.at(i);

            Instance instance;

            instance.objectName = domInstance.namedItem("object").firstChild().nodeValue();
            instance.x = domInstance.namedItem("position").attributes().namedItem("x").nodeValue().toLongLong();
            instance.y = domInstance.namedItem("position").attributes().namedItem("y").nodeValue().toLongLong();
            instance.creationCode = domInstance.namedItem("creationCode").firstChild().nodeValue();

            instances.append(instance);
        }

        const QString destFileName = gms1folder + "/rooms/" + roomName + ".room.gmx";

        QFile destFileRead(destFileName);
        if (!destFileRead.open(QFile::ReadOnly | QFile::Text))
        {
            log(QString("Failed to open file \"%1\" for read").arg(destFileName));
            return;
        }

        QDomDocument destDom;
        if (!destDom.setContent(&destFileRead))
        {
            log(QString("Failed to load DOM content from \"%1\"").arg(destFileRead.fileName()));
            continue;
        }

        QStringList msgs;

        QDomNode roomNode = destDom.namedItem("room");
        roomNode.namedItem("code").firstChild().setNodeValue(code);

        QDomNodeList destInstancesNodes = roomNode.namedItem("instances").childNodes();

        if (instances.count() != destInstancesNodes.count())
        {
            log(QString("The number of instances in projects GM7/8 (count: %1) and GMS1 (count: %2) does not match in room \"%3\"")
                .arg(instances.count()).arg(destInstancesNodes.count()).arg(roomName));
        }

        for (int i = 0; i < std::min(destInstancesNodes.count(), instances.count()); ++i)
        {
            QDomNamedNodeMap attributes = destInstancesNodes.at(i).attributes();

            QDomNode destCodeNode = attributes.namedItem("code");
            if (destCodeNode.nodeValue().isEmpty())
            {
                continue;
            }

            Instance destInstance;

            destInstance.objectName = attributes.namedItem("objName").nodeValue();
            destInstance.x = attributes.namedItem("x").nodeValue().toLongLong();
            destInstance.y = attributes.namedItem("y").nodeValue().toLongLong();

            const Instance sourceInstance = instances.at(i);

            if (destInstance.isSameInstance(sourceInstance))
            {
                destCodeNode.setNodeValue(sourceInstance.creationCode);

                msgs.append(QString("Corrected instance creation code %1 in room \"%2\"").arg(destInstance.getInfoString(), roomName));
            }
            else
            {
                msgs.append(QString("At index %1 found %2 but need %3 in room \"%4\"").arg(i).arg(sourceInstance.getInfoString(), destInstance.getInfoString(), roomName));
            }
        }

        destFileRead.close();

        QFile destFileWrite(destFileName);
        if (!destFileWrite.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        {
            log(QString("Failed to open file \"%1\" for write").arg(destFileName));
            return;
        }

        destFileWrite.write(destDom.toString().toUtf8());

        msgs.append(QString("Corrected room creation code \"%1\"").arg(roomName));

        for (const QString& msg : msgs)
        {
            log(msg);
        }
    }
}
