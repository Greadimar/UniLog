#include "unilogexample.h"
using namespace LogTools;
UniLogExample::UniLogExample(QTextEdit* te)
{
    ul().setupQDebugOutput();
  //  ul().setupSqlOutput(); // text edit is only context for connects and delete later
    UniLog::BlockFilter blockFilter;
    blockFilter.msgTypeFilters << MsgType::valid;
    blockFilter.categoryFilters.insert("lol1");
    ul().setupTeOutput(te, true, QThread::currentThread(), blockFilter);

}

void UniLogExample::writeInfo()
{
   // ul() << "some info";
            ulInfo() << "simple info";
        ulCInfo("lol1") << "info with category";
    ulCValid("lol2") << "valid info with category";
}
