#include "unlogsqlite.h"
#include "logusefull.h"
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>
#include <QTime>
#include <QMetaEnum>
#include "../debugtools.h"
using namespace UniLogSqlite;
using namespace LogTools;
using namespace DebugTools;
void SqliteLogger::startDb()
{
    db = QSqlDatabase::addDatabase("QSQLITE", "UniLogDb");
    db.setDatabaseName(dbName);
    if (!db.open()){
        qWarning() << "UniLog: can't open database '" << db.lastError() << "'";
    }
    createLogTable();
    checkTableSize();
    createMsgTypeTable();
    fillMsgTypeTable();
    createIndexTableToId("LaunchId");
    incrementLaunchId(currentLaunchId);
    QTimer* checkSizeTimer = new QTimer(this);
    connect(checkSizeTimer, &QTimer::timeout, this, &SqliteLogger::checkTableSize);
    checkSizeTimer->start(checkSizeFrequency);

    writeTimer = new QTimer(this);
    connect(writeTimer, &QTimer::timeout, this, &SqliteLogger::onBufferToWrite);
    writeTimer->setSingleShot(true);
    writeTimer->start(bufferToWriteFrequency);
}



bool SqliteLogger::incrementLaunchId(int &launchId)
{
    QString note = "select max(LaunchId) from " + logTableName;
    QSqlQuery qu(db);
    if (!execQuery(qu, note)) return false;
    qu.next();
    int lastId = qu.value(0).toInt();
    launchId = ++lastId;
    return true;
}

bool SqliteLogger::checkTableSize()
{
    QString note = "select max(ID) from " + logTableName;
    QSqlQuery qu(db);
    if (!execQuery(qu, note)) return false;
    qu.next();
    long rowCount = qu.value(0).toLongLong();
    if (rowCount > maxRowsCount) emit tableIsTooLarge();
    return true;
}

bool SqliteLogger::createLogTable() const{
    QString note = "CREATE TABLE IF NOT EXISTS " + logTableName +  " (ID LONG PRIMARY KEY  AUTOINCREMENT, "
                                                                   "LaunchId INTEGER,"
                                                                   "Time TEXT DEFAULT CURRENT_TIMESTAMP, "
                                                                   "Category varchar(63),"
                                                                   "Msg varchar(255),"
                                                                   "MsgType INTEGER)";
    return execQuery(note);
}

bool SqliteLogger::createMsgTypeTable() const
{
    QString note = "CREATE TABLE IF NOT EXISTS " + msgTypeTableName +  " (Seq INTEGER PRIMARY KEY, "
                                                                        "Type CHAR(8))";
    return execQuery(note);
}

bool SqliteLogger::createIndexTableToId(const QString& columnName) const
{
    QString note = "CREATE INDEX IF NOT EXISTS " + indexLaunchTableName + " ON " + logTableName + " (" + columnName + ")";
#if __cplusplus >= 201703L
    bool result{false};
    if constexpr (benchQueries) bench([=, &result](){result = execQuery(note);});
    return result;
#else
    return execQuery(note);
#endif
}

bool SqliteLogger::fillMsgTypeTable()
{
    QSqlQuery query(db);
    query.prepare("INSERT or IGNORE INTO " + msgTypeTableName + " VALUES (:Seq, :Type)");
    QVariantList seqs,types;

    QMetaEnum me = QMetaEnum::fromType<LogTools::MsgType>();
     for (int i = 0; i < me.keyCount(); i++){
        seqs << static_cast<uint>(me.keyToValue(me.key(i)));
        types << enumToString(static_cast<MsgType>(me.keyToValue(me.key(i))));
     }

    query.bindValue(":Seq", seqs);
    query.bindValue(":Type", types);
    return execQueryBatch(query);
}

bool SqliteLogger::cleanLogTable() const
{
    QString note = "DROP TABLE IF EXISTS " + logTableName;
    if (execQuery(note))
        return createLogTable();
    else return false;
}

bool SqliteLogger::writeMsg(const MsgNote &note){
    QString cmd = "INSERT INTO " + logTableName + " (LaunchId, Time, Category, Msg, MsgType) " +
                                                  "VALUES (?, ?, ?, ?, ?)";
    QSqlQuery qu(db);
    qu.prepare(cmd);
    qu.addBindValue(currentLaunchId);
    qu.addBindValue(note.timestamp);
    qu.addBindValue(note.category);
    qu.addBindValue(note.msg);
    qu.addBindValue(enumToString(note.type));
    return execQuery(qu);
}

bool SqliteLogger::execQuery(const QString &cmd) const{
    QSqlQuery query(db);
    if (!query.exec(cmd)){
        qWarning() << Q_FUNC_INFO << query.lastError();
        qDebug() << Q_FUNC_INFO << "UniLog: failed to execute '" << query.lastQuery() << "'" << QTime::currentTime();
        return false;
    }
    return true;
}

bool SqliteLogger::execQuery(QSqlQuery &query, const QString &cmd) const
{
    if (!query.exec(cmd)){
        qWarning() << Q_FUNC_INFO << query.lastError();
        qDebug() << Q_FUNC_INFO << "UniLog: failed to execute '" << query.lastQuery() << "'" << QTime::currentTime();
        return false;
    }
    return true;
}

bool SqliteLogger::execQuery(QSqlQuery &query) const
{
    if (!query.exec()){
        qWarning() << Q_FUNC_INFO << query.lastError();
        qDebug() << Q_FUNC_INFO << "UniLog: failed to execute '" << db.lastError() << "'" << QTime::currentTime();
        return false;
    }
    return true;
}

bool SqliteLogger::execQueryBatch(QSqlQuery &query) {
    db.transaction();
    if (!query.execBatch()){
        db.commit();
        qWarning() << Q_FUNC_INFO << query.lastError();
        qDebug() << Q_FUNC_INFO << "UniLog: failed to execute '" << query.lastQuery() << "'" << QTime::currentTime();
        return false;
    }
    db.commit();
    return true;
}

void SqliteLogger::onBufferToWrite()
{
     if (bufferToWrite.size() > estimatedMaxBufferSize) popBufferAtOnce++;
     int bufSize = bufferToWrite.size();
     int toPop = bufSize > popBufferAtOnce? popBufferAtOnce : bufSize;
     auto logToSendVec = bufferToWrite.mid(0, toPop);
     if (logToSendVec.isEmpty()){
         writeTimer->start(bufferToWriteFrequency * 5); //more time before next buffer squeeze. It was empty before, so we can wait more time, to free more resources
         return;
     }
     db.transaction();
     QSqlQuery insertLogQuery(db);
     insertLogQuery.prepare("INSERT INTO " + logTableName +
                            " (LaunchId, Time, Category, Msg, MsgType)"
                            " VALUES (:LaunchId, :Time, :Category, :Msg, :MsgType)");
     for (const auto& l: qAsConst(logToSendVec)){
         insertLogQuery.bindValue(":LaunchId", currentLaunchId);
         insertLogQuery.bindValue(":Time",  l.timestamp);
         insertLogQuery.bindValue(":Category", l.category);
         insertLogQuery.bindValue(":Msg", l.msg);
         insertLogQuery.bindValue(":MsgType", enumToString(l.type));
         insertLogQuery.exec();
     }
     db.commit();
     bufferToWrite.remove(0, toPop);
     writeTimer->start(bufferToWriteFrequency);
}
