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
};
