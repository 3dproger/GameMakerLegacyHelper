#pragma once

#include <QString>
#include <functional>

class GMS1Corrector
{
public:
    static void setLogCallback(std::function<void(const QString&)> callback);
    static void convertAnsiToUtf8(const QString& gmkFileName, const QString& gms1folder);

private:
    static void log(const QString& text);

    static void copyScripts(const QString& gmkSplitOutput, const QString& gms1folder);
    static void copyObjectCodes(const QString& gmkSplitOutput, const QString& gms1folder);
    static void copyRoomCodes(const QString& gmkSplitOutput, const QString& gms1folder);

    static QString gms1EventTypeToGmk(const QString& type);
};
