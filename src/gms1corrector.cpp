#include "gms1corrector.h"

namespace
{

static std::function<void(const QString&)> logCallback = nullptr;

}

void GMS1Corrector::setLogCallback(std::function<void (const QString &)> callback)
{
    logCallback = callback;
}

void GMS1Corrector::convertAnsiToUtf8(const QString &gmkFileName, const QString &gms1folder)
{
    log("Done!");
}

void GMS1Corrector::log(const QString &text)
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
