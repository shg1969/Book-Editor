#include "inc/mainwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QFile f("C:/Users/lei/Desktop/study_pro/GIthub/Book-Editor/conf/style_sheet_dark.txt");
    f.open(QIODevice::ReadOnly);
    QString s_dark=f.readAll();
    QTextStream In_1(&f);
    while(!f.atEnd())
    {
        In_1>>s_dark;
    }
    f.close();

    QFile f_light("C:/Users/lei/Desktop/study_pro/GIthub/Book-Editor/conf/style_sheet_light.txt");
    f_light.open(QIODevice::ReadOnly);
    QString s_light=f_light.readAll();
    QTextStream In_2(&f_light);
    while(!f_light.atEnd())
    {
        In_2>>s_light;
    }
    f_light.close();
    a.setStyleSheet(s_light);

    w.connect(&w,&MainWindow::theme_changed,[&a,s_dark,s_light](bool mode){
        qDebug()<<"主题改变";
        if(mode)
        {
            a.setStyleSheet(s_dark);
        }
        else
        {
            a.setStyleSheet(s_light);
        }
    });
    w.show();
    return a.exec();
}

/*需求
*按关键词对key、笔记进行模糊搜索
* 写作辅助，写到关键词时，启动线程提供待选词语、好句子
* 书签功能
* 删除封面的功能
*软件锁、书籍锁
* 摘录分类，增加笔记、随记功能
* 设置界面，设置各类默认路径
*/
