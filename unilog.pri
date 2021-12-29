!contains(DEFINES, __UNILOG__){
DEFINES += __UNILOG__
HEADERS += \
    $$PWD/logusefull.h \
    $$PWD/msgnote.h \
    $$PWD/msgtypes.h
    contains(QT, gui){
        HEADERS += \
            $$PWD/unilog.h
        SOURCES += \
            $$PWD/unilog.cpp
        linux{
            packagesExist(sqlite3){
                DEFINES += SQLITE3_EXISTS
                QT += sql
                HEADERS += \
                $$PWD/unlogsqlite.h
                SOURCES += \
                $$PWD/unlogsqlite.cpp
            }
            else{
                message(UniLog: sqlite3 was not found. Some logging function will be disabled. Please install libsqlite3-dev for linux)
            }
        }
        win32 || win64{
            DEFINES += SQLITE3_EXISTS
            QT += sql
            HEADERS += \
            $$PWD/unlogsqlite.h
            SOURCES += \
            $$PWD/unlogsqlite.cpp
        }
    }

}

