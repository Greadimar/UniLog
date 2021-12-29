#ifndef MSGLOGTYPES_H
#define MSGLOGTYPES_H
#include "QMetaObject"
#include <QDebug>
#include <QHash>
namespace LogTools {
Q_NAMESPACE
enum class MsgType: short{
    debug = static_cast<uint>(QtMsgType::QtDebugMsg),
    warning = static_cast<uint>(QtMsgType::QtWarningMsg),
    critical = static_cast<uint>(QtMsgType::QtCriticalMsg),
    info = static_cast<uint>(QtMsgType::QtInfoMsg),
    valid = 6


};
Q_ENUM_NS(MsgType)
}
// doesn't work, something with namespaces
inline uint qHash(const LogTools::MsgType &key, uint seed)
{
    return qHash(seed) ^ static_cast<int>(key);
}



#endif // UNILOGTYPES_H
