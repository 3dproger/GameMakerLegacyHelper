#pragma once

#include <QString>

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

    static QList<Note> breakToExit(const QString& gms2folder);

private:
    static bool isContainsWord(const QByteArray& text, const QByteArray& word);
};
