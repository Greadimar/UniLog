#ifndef LOGUSEFULL_H
#define LOGUSEFULL_H
#include <QTextStream>
#include <QTime>
#include <QDebug>
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 0)
    //typedef Qt::TS_ENDL = TS_ENDL;
    #define TS_ENDL  Qt::endl
    #define TS_HEX   Qt::hex
    #define TS_DEC   Qt::dec
#else
    #define TS_ENDL  endl
    #define TS_HEX   hex
    #define TS_DEC   dec
#endif
namespace LogTools {
template<typename TQEnum>
QString enumToString (const TQEnum value)
{
    return QVariant::fromValue(value).toString();
}

inline QString hexString(uint hex, QString suffix = "0x")
{
    QString tempStr = QString::number(hex, 16);
    while (tempStr.length() < 8){
        tempStr.prepend("0");
    }
    tempStr.prepend(suffix);
    return tempStr;
}
inline QString curTime(const QString& format = "hh:mm:ss.zzz"){
    return QTime::currentTime().toString(format);
}
inline QString curDateTime(const QString& format = "yy.MM.dd hh:mm:ss"){
    return QDateTime::currentDateTime().toString(format);
}
}
#endif // LOGUSEFULL_H
