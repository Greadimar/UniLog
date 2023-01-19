#ifndef MSGLOGNOTE_H
#define MSGLOGNOTE_H
#include "msgtypes.h"
namespace LogTools {
struct MsgNote{
    MsgNote(){}
    MsgNote(const QString& timestamp, const QString& cat, const QString& msg, const LogTools::MsgType& type):
        timestamp(timestamp + " "), category(cat), msg(msg), type(type){}
    QString timestamp;
    QString category;
    QString msg;
    LogTools::MsgType type{LogTools::MsgType::info};
};
}

#endif // MSGLOGNOTE_H
