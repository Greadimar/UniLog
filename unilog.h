#ifndef UNILOG_H
#define UNILOG_H
#include <QMutex>
#include <QIODevice>
#include <QDebug>
#include <QTime>
#include <QLoggingCategory>
#include <QtSql/QSqlDatabase>
#include "logusefull.h"
#include "msgtypes.h"
#include "msgnote.h"
#ifdef SQLITE3_EXISTS
#include "unlogsqlite.h"
#endif
#include <QThread>
#include <QSet>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QHash>
#include <tuple>
#include <QReadWriteLock>
#include "../hashtools/hashtools.h"

struct UniLogPalette{
    static QMap<LogTools::MsgType, QColor> fontLightColor;
    static QMap<LogTools::MsgType, QColor> fontDarkColor;
    static QMap<LogTools::MsgType, QColor>* fontColor;
    static QColor getColor(const LogTools::MsgType& mt){
        auto col = fontColor->value(mt);
        return col;
    }
    static void changeToDarkMode();
    static void changeToLightMode();
};

struct UniHtml{

    enum class HtmlTag{
        regular, bold, italic, custom
    };
    UniHtml(){}
    UniHtml(QColor color){
        htmlBegin = "<font color=\"" + color.name() + "\">";
        htmlEnd = "</font> ";
    }
    UniHtml(QFont ){
        //TODO: support fonts
    }
    UniHtml(QFont::Weight ){
        //TODO: support font's weight
    }
    HtmlTag tag{HtmlTag::regular};
    QString htmlBegin;
    QString htmlEnd;
};

struct LogCategory{
    LogCategory(){}
    LogCategory(const QString cName): catName(cName){}
    QString catName;
};

class UniLog;

class UniLogInterception;
using UniLogInterceptionLambda = std::function<UniLogInterception &(UniLogInterception&)>;
class UniLogInterception: public QDebug{
public:
    ~UniLogInterception();
    // why redeclare string operators? utf8 support?
    inline UniLogInterception &operator<<(const QString& t) {
        (*static_cast<QDebug*>(this)) << reinterpret_cast<const char*>(t.toUtf8().data());
        return *this;
    }
    inline UniLogInterception &operator<<(const QStringRef& t) {
        (*static_cast<QDebug*>(this)) << reinterpret_cast<const char*>(t.toUtf8().data());
        return *this;
    }
    inline UniLogInterception &operator<<(const QStringView& s) {
        (*static_cast<QDebug*>(this)) << reinterpret_cast<const char*>(s.toUtf8().data());
        return *this;
    }
    inline UniLogInterception &operator<<(const QLatin1String& t) {
        (*static_cast<QDebug*>(this)) << reinterpret_cast<const char*>(t.data());
        return *this;
    }
    inline UniLogInterception &operator<<(const LogCategory& c) {
        this->currentCategory = c;
        return *this;
    }
    inline UniLogInterception &operator<<(LogTools::MsgType mt) {
        this->msgType = mt;
        hasHtml = true;
        optHtml = UniHtml(UniLogPalette::getColor(mt));
        return *this;
    }
    inline UniLogInterception &operator<<(UniHtml mt) {
        if(hasHtml)
            *this << optHtml.htmlEnd;
        *this << mt.htmlBegin;
        return *this;
    }
    template <typename T>
    inline UniLogInterception &operator<<(const T& t) {
        (*static_cast<QDebug*>(this)) << t;
        return *this;
    }
private:
    //constructors for MsgTypes, log category, and UniHtml;
    UniLogInterception(UniLog& logger, const LogTools::MsgType& msgType = LogTools::MsgType::info, const LogCategory& logCategory = {""}):
        QDebug(&buffer), logger(logger), msgType(msgType), currentCategory(logCategory){
    }
    UniLogInterception(UniLog& logger, const UniHtml& customStyle,
                       const LogTools::MsgType& msgType = LogTools::MsgType::info, const LogCategory& logCategory = {""}):
        QDebug(&buffer), logger(logger), optHtml(customStyle), msgType(msgType), currentCategory(logCategory){
        hasHtml = true;
    }
    //other constructors
    template <typename T>
    UniLogInterception(UniLog& logger, const T& t): QDebug(&buffer), logger(logger){
        *this << t;
    }

    friend class UniLog;
    QString buffer;
    UniLog& logger;
    bool hasHtml{false};    //in c++ 17 just use const std::optional<UniHtml> optHtml;
    UniHtml optHtml;
    LogTools::MsgType msgType{LogTools::MsgType::info};
    LogCategory currentCategory;
};
#define ul() UniLog::instance()
#define ulC(Category) UniLog::instance() << LogCategory(Category)

#define ulInfo() UniLog::instance() << LogTools::MsgType::info
#define ulDebug() UniLog::instance() << LogTools::MsgType::debug
#define ulValid() UniLog::instance() << LogTools::MsgType::valid
#define ulWarning() UniLog::instance() << LogTools::MsgType::warning
#define ulCritical() UniLog::instance() << LogTools::MsgType::critical

#define ulCInfo(Category) UniLog::instance() << LogCategory(Category) << LogTools::MsgType::info
#define ulCDebug(Category) UniLog::instance() << LogCategory(Category) << LogTools::MsgType::debug
#define ulCValid(Category) UniLog::instance() << LogCategory(Category) << LogTools::MsgType::valid
#define ulCWarning(Category) UniLog::instance() << LogCategory(Category) << LogTools::MsgType::warning
#define ulCCritical(Category) UniLog::instance() << LogCategory(Category) << LogTools::MsgType::critical

#define ul_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>()
#define ulC_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category)

#define ulInfo_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogTools::MsgType::info
#define ulDebug_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogTools::MsgType::debug
#define ulValid_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogTools::MsgType::valid
#define ulWarning_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogTools::MsgType::warning
#define ulCritical_spc(loggerName) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogTools::MsgType::critical

#define ulCInfo_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category) << LogTools::MsgType::info
#define ulCDebug_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category) << LogTools::MsgType::debug
#define ulCValid_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category) << LogTools::MsgType::valid
#define ulCWarning_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category) << LogTools::MsgType::warning
#define ulCCritical_spc(loggerName, Category) UniLog::instance<HashTools::hash_crc32(loggerName)>() << LogCategory(Category) << LogTools::MsgType::critical




class UniLog
{
public:
    struct BlockFilter{
        QSet<QString> categoryFilters;
        QVector<LogTools::MsgType> msgTypeFilters;  //TODO: QSet
        bool isIgnored(const QString& category) const{
            bool&& res = categoryFilters.contains(category);
            return res;
        };
        bool isIgnored(const LogTools::MsgType& msgType) const{
            bool&& res = msgTypeFilters.contains(msgType);
            return res;
        };
    };
    template <uint uniqueId = 0>
    static UniLog& instance(){ static UniLog uniLog; return uniLog;}
public:
    //TODO return uint with id of output, so we can remove output by id
#ifdef SQLITE3_EXISTS
    void setupSqlOutput(QObject* context, BlockFilter ignoreFilter = {});
#endif
    void setupQIODeviceOutput(QIODevice* qioDevice, bool htmlSupport, const BlockFilter& ignoreFilter = {},
                              QThread* threadForOutput = QThread::currentThread(),
                              const QString& bodyStyleString = {});
    void setupQIODeviceOutput(QIODevice* qioDevice, bool htmlSupport, bool addCategerySuffix, const BlockFilter& ignoreFilter = {},
                              QThread* threadForOutput = QThread::currentThread(),
                              const QString& bodyStyleString = {});
#ifdef QT_GUI_LIB
    void setupPteOutput(QPlainTextEdit* pte, bool htmlSupport,
                        const BlockFilter& ignoreFilter = {},
                        QThread* threadForOutput = QThread::currentThread(),
                        const QString& bodyStyleString = {});
    void setupPteOutput(QPlainTextEdit* pte, bool htmlSupport, bool addTimeStamp,
                        const BlockFilter &ignoreFilter = {},
                        QThread* threadForOutput = QThread::currentThread(),
                        const QString& bodyStyleString = {});
    void setupPteOutput(QPlainTextEdit* pte, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix,
                        const BlockFilter &ignoreFilter = {},
                        QThread* threadForOutput = QThread::currentThread(),
                        const QString& bodyStyleString = {});
    void setupTeOutput(QTextEdit* te, bool htmlSupport,
                       const BlockFilter &ignoreFilter = {},
                       QThread* threadForOutput = QThread::currentThread(),
                       const QString &bodyStyleString = {});
    void setupTeOutput(QTextEdit* te, bool htmlSupport, bool addTimeStamp,
                       const BlockFilter& ignoreFilter = {},
                       QThread* threadForOutput = QThread::currentThread(),
                       const QString& bodyStyleString = {});
    void setupTeOutput(QTextEdit* te, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix,
                       const BlockFilter& ignoreFilter = {},
                       QThread* threadForOutput = QThread::currentThread(),
                       const QString& bodyStyleString = {});
#endif
    void setupTsOutput(QTextStream* ts, bool htmlSupport, const BlockFilter& ignoreFilter = {},
                       QThread* threadForOutput = QThread::currentThread(),
                       const QString& bodyStyleString = {});
    void setupTsOutput(QTextStream* ts, bool htmlSupport, bool addTimeStamp, bool addCategorySuffix, const BlockFilter& ignoreFilter = {},
                       QThread* threadForOutput = QThread::currentThread(),
                       const QString& bodyStyleString = {});
    void setupCustomOutput(std::function<void(const LogTools::MsgNote&)> rcvr, bool htmlSupport,
                           QThread* threadForOutput = QThread::currentThread(),
                           BlockFilter ignoreFilter = {});
    bool removeOutput(QObject* outputDevice); //textedits, qiodevices... TODO remove customOutputs and debugOutput...
    void setupQDebugOutput(BlockFilter ignoreFilter = {});

    UniLogInterception operator<< (const LogCategory& c){
        return UniLogInterception(*this, LogTools::MsgType::info, c);
    }
    UniLogInterception operator<< (const UniHtml& h){
        return UniLogInterception(*this, h);
    }
    UniLogInterception operator<< (const LogTools::MsgType& mt){
        return UniLogInterception(*this, UniHtml(UniLogPalette::getColor(mt)), mt);
    }
    template <typename T>
    UniLogInterception operator<< (T arg){
        return UniLogInterception(*this, arg);
    }

    void logMsg(QString msg, const QString &category = {""}, const LogTools::MsgType &type = LogTools::MsgType::info);
    void logMsg(const QString &msg, const UniHtml &html, const QString &category = {""}, const LogTools::MsgType &type = LogTools::MsgType::info);


private:
    explicit UniLog(){
        qDebug() << Q_FUNC_INFO;}
    QMutex mutex;
    void openAndStartDb(QSqlDatabase database);
private:
    using LogReciever = std::function<void(const LogTools::MsgNote&)>;
    QReadWriteLock rwMutex{QReadWriteLock::RecursionMode::Recursive};
    struct LogOutput{
        explicit LogOutput(){}
        explicit LogOutput(QObject* output, const LogReciever& rcvr, const BlockFilter& filter, bool htmlSupport, QThread* threadToExecute):
            outputObject(output), reciever(rcvr), ignoreFilter(filter), htmlSupport(htmlSupport), threadToExecute(threadToExecute)
        {
        }
        QObject* outputObject{nullptr};
        LogReciever reciever;
        const BlockFilter ignoreFilter;
        const bool htmlSupport{false};
        const QThread* threadToExecute;
        //TODO: add Pattern for output. For example: "{time}: {cat} - {msg}". Inside LogReciver pattern should be parsed.
        //Watch logging pattern implementation in QDebug
    };

    std::once_flag sqlSetuped;
#ifdef SQLITE3_EXISTS
    UniLogSqlite::SqliteLogger* sqlLogger;
#endif

    QThread* sqlLoggerThread;
    QVector<LogOutput> logOutputs;

    BlockFilter debugFilter;
    QString debugRegisteredCategories;

    void setupSqlOutputOnce(QObject* context, BlockFilter ignoreFilter = {});
    void colorizeByType(QString &text);
private slots:
#ifdef SQLITE3_EXISTS
    void showSqlWarningMsg();
#endif




};
inline UniLog& uniLog(){ return UniLog::instance();}


#endif // UNILOG_H
