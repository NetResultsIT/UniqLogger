#include <QCoreApplication>
#include <QDebug>
#include <QtTest/QtTest>

#define NRDBG qDebug()
#define NRDBG2 qDebug

#include <testTimeUtils.h>
#include <testFileWriter.h>

QTEST_MAIN(testFileWriter)
//QTEST_MAIN(testTimeUtils)

/*
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    testTimeTicker ttt;

    //return a.exec();
}
*/

//#include "moc_testTimeTicker.cpp"
