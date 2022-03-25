#include "unilog.h"
#include "unlogsqlite.h"

#include <QMessageBox>
#include <QReadLocker>

#include <QString>
#include <QWriteLocker>
#include "../functiontools/posttothread.h"

using namespace UniLogSqlite;
QMap<LogTools::MsgType, QColor> UniLogPalette::fontLightColor{
    {LogTools::MsgType::info, QColor(Qt::blue)},
    {LogTools::MsgType::debug, QColor(Qt::black)},
    {LogTools::MsgType::warning, QColor("#c65102")},
    {LogTools::MsgType::valid, QColor(Qt::darkGreen)},
    {LogTools::MsgType::critical, QColor(Qt::red)}
    };
QMap<LogTools::MsgType, QColor> UniLogPalette::fontDarkColor{
            {LogTools::MsgType::info, QColor(Qt::cyan)},
            {LogTools::MsgType::debug, QColor(Qt::white)},
            {LogTools::MsgType::warning, QColor(Qt::yellow)},
            {LogTools::MsgType::valid, QColor(Qt::green)},
            {LogTools::MsgType::critical, QColor(Qt::red)}
    };
QMap<LogTools::MsgType, QColor>* UniLogPalette::fontColor = &fontLightColor;
void UniLogPalette::changeToDarkMode(){
    fontColor = &fontDarkColor;
}
void UniLogPalette::changeToLightMode(){
    fontColor = &fontLightColor;
}
UniLogInterception::~UniLogInterception(){
    if (hasHtml) logger.logMsg(buffer, optHtml, currentCategory.catName, msgType);
    else logger.logMsg(buffer, optHtml, currentCategory.catName, msgType);
}
#ifdef SQLITE3_EXISTS
void UniLog::setupSqlOutput(QObject* context, BlockFilter ignoreFilter){
    std::call_once(sqlSetuped, [=](QObject *context, const UniLog::BlockFilter& ignoreFilter){setupSqlOutputOnce(context, ignoreFilter);}, context, ignoreFilter);
}
void UniLog::setupSqlOutputOnce(QObject *context, UniLog::BlockFilter ignoreFilter)
{
    sqlLoggerThread = new QThread();
    sqlLogger = new UniLogSqlite::SqliteLogger();
    sqlLoggerThread->start();
    sqlLogger->moveToThread(sqlLoggerThread);
    context->connect(sqlLoggerThread, &QThread::finished, sqlLogger, &QObject::deleteLater);
    context->connect(sqlLoggerThread, &QThread::finished, sqlLoggerThread, &QObject::deleteLater);
    context->connect(context, &QObject::destroyed, sqlLoggerThread, &QThread::quit);
    QMetaObject::invokeMethod(sqlLogger, &SqliteLogger::startDb, Qt::QueuedConnection);
    auto msgReciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(sqlLoggerThread, [=](){sqlLogger->gotLog(note);});
    };
    QWriteLocker locker(&rwMutex);
    logOutputs << LogOutput{sqlLogger, msgReciever, ignoreFilter, false, sqlLoggerThread};
}
#endif

void UniLog::setupQIODeviceOutput(QIODevice *qioDevice, bool htmlSupport, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    setupQIODeviceOutput(qioDevice, htmlSupport, false, ignoreFilter, threadForOutput, bodyStyleString);
}

void UniLog::setupQIODeviceOutput(QIODevice *qioDevice, bool htmlSupport, bool addCategerySuffix, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    LogReciever reciever;
    QTextStream(qioDevice) << bodyStyleString;
    reciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(threadForOutput, [=](){
                //TODO: check for proper pattern
                QString optCat = addCategerySuffix? note.category + "; ": "";
                QTextStream(qioDevice) << note.timestamp << ": " << optCat << note.msg;
            });
    };
    QWriteLocker locker(&rwMutex);
    logOutputs << LogOutput{qioDevice, reciever, ignoreFilter, htmlSupport, threadForOutput};
}

void UniLog::setupPteOutput(QPlainTextEdit *pte, bool htmlSupport, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString& bodyStyleString)
{
    setupPteOutput(pte, htmlSupport, false, ignoreFilter, threadForOutput, bodyStyleString);
}

void UniLog::setupPteOutput(QPlainTextEdit *pte, bool htmlSupport, bool addTimeStamp, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString& bodyStyleString)
{
    setupPteOutput(pte, htmlSupport, addTimeStamp, false, ignoreFilter, threadForOutput, bodyStyleString);
}

void UniLog::setupPteOutput(QPlainTextEdit *pte, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    if (!bodyStyleString.isEmpty()) FunctionTools::postToThread(threadForOutput, [=](){pte->appendHtml(bodyStyleString);});
    LogReciever reciever;
    reciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(threadForOutput, [=](){
                QString out = note.msg;
                if (addCategorySuffix) out.prepend(note.category+ "; ");
                if (addTimeStamp) out.prepend(note.timestamp + ": ");
                pte->appendHtml(out);
            });
    };
    QWriteLocker locker(&rwMutex);
    logOutputs << LogOutput{pte, reciever, ignoreFilter, htmlSupport, threadForOutput};

}

void UniLog::setupTeOutput(QTextEdit *te, bool htmlSupport, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString& bodyStyleString)
{
    setupTeOutput(te, htmlSupport, false, false, ignoreFilter, threadForOutput, bodyStyleString);
}
void UniLog::setupTeOutput(QTextEdit *te, bool htmlSupport, bool addTimeStamp, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString& bodyStyleString)
{
    setupTeOutput(te, htmlSupport, addTimeStamp, false, ignoreFilter, threadForOutput, bodyStyleString);
}

void UniLog::setupTeOutput(QTextEdit *te, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    if (!bodyStyleString.isEmpty()) FunctionTools::postToThread(threadForOutput, [=](){te->append(bodyStyleString);});
    LogReciever reciever;
    reciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(threadForOutput, [=](){
                QString out = note.msg;
                if (addCategorySuffix) out.prepend(note.category+ "; ");
                if (addTimeStamp) out.prepend(note.timestamp + ": ");
//                if (htmlSupport) out = Qt::convertFromPlainText(out);
                te->append(out);});
    };
    QWriteLocker locker(&rwMutex);
    logOutputs << LogOutput{te, reciever, ignoreFilter, htmlSupport, threadForOutput};
}

void UniLog::setupTsOutput(QTextStream *ts, bool htmlSupport, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    setupTsOutput(ts, htmlSupport, false, false, ignoreFilter, threadForOutput, bodyStyleString);
}

void UniLog::setupTsOutput(QTextStream *ts, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix, const BlockFilter &ignoreFilter, QThread *threadForOutput, const QString &bodyStyleString)
{
    LogReciever reciever;
    *ts << bodyStyleString;
    reciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(threadForOutput, [=](){
                //TODO check for proper pattern
                QString optTime = addTimeStamp? note.timestamp + ": " : "";
                QString optCat = addCategorySuffix? note.category + "; ": "";
                *ts << optTime << optCat << note.msg  << TS_ENDL;
            });
    };
    QWriteLocker locker(&rwMutex);
    logOutputs << LogOutput{ts->device(), reciever, ignoreFilter, htmlSupport, threadForOutput};
}

void UniLog::setupCustomOutput(std::function<void(const LogTools::MsgNote &)> rcvr, bool htmlSupport, QThread *threadForOutput, UniLog::BlockFilter ignoreFilter)
{
    QWriteLocker locker(&rwMutex);
    LogReciever reciever;
    reciever = [=](const LogTools::MsgNote& note){
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type))
            FunctionTools::postToThread(threadForOutput, [=](){rcvr(note);});
    };
    logOutputs << LogOutput{nullptr, reciever, ignoreFilter, htmlSupport, threadForOutput};
}


void UniLog::setupQDebugOutput(UniLog::BlockFilter ignoreFilter)
{
    QWriteLocker locker(&rwMutex);
    LogReciever reciever;
    reciever = [=](const LogTools::MsgNote& note) mutable{
        if (!ignoreFilter.isIgnored(note.category) && !ignoreFilter.isIgnored(note.type)){
            QMessageLogContext qtlogcontext(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC,
                                            note.category.isEmpty() ? "default" :
                                                                      note.category.toStdString().c_str());
            // QMessageLogger //same
            //                switch (note.type) {
            //                    case ()
            //                }
            // qtmsglogger.debug() << note.msg;
            QtMsgType qtmsgtype = note.type == LogTools::MsgType::valid? QtMsgType::QtInfoMsg :
                                                                         static_cast<QtMsgType>(note.type);
            qt_message_output(qtmsgtype, qtlogcontext, note.msg);
        }
    };
    logOutputs << LogOutput{nullptr, reciever, ignoreFilter, false, nullptr};
}

bool UniLog::removeOutput(QObject* outputDevice){
    QWriteLocker locker(&rwMutex);
    QMutableVectorIterator<LogOutput> it(logOutputs);
    while (it.hasNext()){
        auto& o = it.next();
        if (o.outputObject){
            if (o.outputObject == outputDevice){
                it.remove();
                return true;
            }
        }
    }
    return false;
}

void UniLog::logMsg(QString msg, const QString &category, const LogTools::MsgType &type)
{
    QReadLocker locker(&rwMutex);
    for (const auto& output: qAsConst(logOutputs)){
        LogTools::MsgNote note{LogTools::curTime(), category, msg, type};
        output.reciever(note);
    }
}

void UniLog::logMsg(const QString& msg, const UniHtml& html, const QString& category, const LogTools::MsgType& type){
    //NOTE: need bench for this lockers... maybe removeOutput doesn't cost a loss in efficiency
    QReadLocker locker(&rwMutex);
    for (const auto& output: qAsConst(logOutputs)){
        QString outputMsg = msg;
        if (output.htmlSupport){
            outputMsg.prepend(html.htmlBegin);
            outputMsg.append(html.htmlEnd);
        }
        LogTools::MsgNote note{LogTools::curTime(), category, outputMsg, type};
        output.reciever(note);
    }

}


#ifdef SQLITE3_EXISTS
void UniLog::showSqlWarningMsg()
{
#ifdef QT_WIDGETS_LIB
    QMessageBox box(QMessageBox::Warning, "Внимание, логи переполнены", "Таблица логов занимает много места на жёстком диске."
                                                                        "Очистить логи автоматически? ",
                    QMessageBox::Yes | QMessageBox::No);
    box.setModal(false);
    int ret = box.exec();
    if (ret == QMessageBox::Yes) QMetaObject::invokeMethod(sqlLogger, &SqliteLogger::cleanLogTable, Qt::QueuedConnection);
#else
    qDebug() << "UniLog:: Log table is full, please clean logs as soon as possible"
 #endif
}
#endif

