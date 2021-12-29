#ifndef UNILOGDETAILS_H
#define UNILOGDETAILS_H
#include <QtGlobal>
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QObject>
#include <QVector>
#include <QTimer>
#include "logusefull.h"
#include "msgtypes.h"
#include "msgnote.h"
namespace UniLogSqlite{
constexpr bool benchQueries{true};


//LogBlockFilter t{{"1","2"}, {LogTools::MsgType::critical}};


class SqliteLogger : public QObject{
    Q_OBJECT
signals:
    void tableIsTooLarge();
public slots:
    bool checkTableSize();
    void gotLog(QString msg, QString category = "common", LogTools::MsgType type = LogTools::MsgType::info, QString timestamp = LogTools::curDateTime()){
        bufferToWrite << LogTools::MsgNote{timestamp, category, msg, type};
    }
    void gotLog(const LogTools::MsgNote& note){
        bufferToWrite << note;
    }
public:
    ~SqliteLogger(){
        db.close();}
    const int maxRowsCount{1000000};
    const QString dbName{"unilog.db"};
    const QString logTableName{"logTable"};
    const QString indexLaunchTableName{"indexLaunchTable"};
    const QString msgTypeTableName{"msgTypeTable"};
    const std::chrono::minutes checkSizeFrequency{1};
    const std::chrono::milliseconds bufferToWriteFrequency{10};
    QSqlDatabase db;
    int currentLaunchId{0};

    QVector<LogTools::MsgNote> bufferToWrite;
    QTimer* writeTimer;
    const int estimatedMaxBufferSize{1500};
    int popBufferAtOnce{100};

    void startDb();


    bool incrementLaunchId(int &launchId);
    bool createLogTable() const;
    bool createMsgTypeTable() const;
    bool createIndexTableToId(const QString& columnName) const;
    bool fillMsgTypeTable();
    bool cleanLogTable() const;

    bool writeMsg(const LogTools::MsgNote& note);

    bool execQuery(const QString& cmd) const;
    bool execQuery(QSqlQuery &query, const QString& cmd) const;
    bool execQuery(QSqlQuery &query) const;
    bool execQueryBatch(QSqlQuery& query) ;
private slots:
    void onBufferToWrite();
};


}
#endif // UNILOGDETAILS_H
