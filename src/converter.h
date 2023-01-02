#pragma once

#include <QString>

class Converter
{
public:
    static void convert(const QString& gms2folder);

private:
    static bool isContainsWord(const QByteArray& text, const QByteArray& word);
};
