#pragma once

#include <QString>
#include <functional>

class GMS2Corrector
{
public:
    static void setLogCallback(std::function<void(const QString&)> callback);
    static void breakToExit(const QString& gms2folder);
    static void replace(const QString& gms2folder, const QString& from, const QString& to);

private:
    static bool checkInput(const QString& gms2folder);

    static bool isContainsWord(const QByteArray& text, const QByteArray& word);
    static void log(const QString& text);
    static QByteArray readFile(const QString& fileName);
};
