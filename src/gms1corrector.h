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

    static void copyScripts(const QString& gmkSplitOutput, const QString& gms1folder);

    static void correctObjectsCodes(const QString& gmkSplitOutput, const QString& gms1folder);
    static void correctObjectCodes(const QString& objectName, const QString& gms1folder, const QList<SourceEvent>& sourceEvents);

    static void correctRoomsCreationCode(const QString& gmkSplitOutput, const QString& gms1folder);
};
