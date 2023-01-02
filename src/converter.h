#pragma once

#include <QString>
#include <functional>

class Converter
{
public:
    struct Note
    {
        enum Type { Error };

        Note(const Type type_, const QString& text_)
            : type(type_)
            , text(text_)
        {}

        Type type;
        QString text;
    };

    static void setLogCallback(std::function<void(const QString&)> callback);
    static QList<Note> breakToExit(const QString& gms2folder);

private:
    static bool isContainsWord(const QByteArray& text, const QByteArray& word);
    static void log(const QString& text);
    static QByteArray readFile(const QString& fileName);
};
