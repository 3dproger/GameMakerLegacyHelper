#pragma once

#include <QStringList>
#include <QDomDocument>
#include <functional>

class GMS1Corrector
{
public:
    static void setLogCallback(std::function<void(const QString&)> callback);
    static void convertAnsiToUtf8(const QString& gmkFileName, const QString& gms1folder);

private:
    struct SourceEvent
    {
        SourceEvent(const QString& type_, const QString& id_, const QString& with_)
            : type(type_), id(id_), with(with_) {}

        QString type;
        QString id;
        QString with;

        QStringList codes;
    };

    static void log(const QString& text);

    static void copyScripts(const QString& gmkSplitOutput, const QString& gms1folder);
    static void copyObjectCodes(const QString& gmkSplitOutput, const QString& gms1folder);
    static void copyObjectCode(const QString& objectName, const QString& gms1folder, const QList<SourceEvent>& sourceEvents);

    static void copyRoomCodes(const QString& gmkSplitOutput, const QString& gms1folder);

    static QString gms1EventTypeToGmk(const QString& type);

    static void domToStringCorrected(QString& result, const QDomNode& node, int intend = 0);
};
