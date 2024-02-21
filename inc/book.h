#ifndef BOOK_H
#define BOOK_H

#include<QFile>
#include<QList>
#include<QPair>
#include<QLabel>
#include<QFrame>
#include<QVBoxLayout>
#include<QPixmap>
#include<QString>
#include<QTextBlock>
#include<QTreeWidgetItem>
#include<QComboBox>
#include<QTextBrowser>
#include<QPushButton>
#include<QTextCodec>
#include<QLineEdit>
#include<QFormLayout>
#include<QMessageBox>
#include<QFileDialog>
#include<QDataStream>
#include<QDataStream>

//软件数据的存放顶层路径
#define TOP_DIR (QString("./data/books/"))
//通过txt文件导入书籍时的默认打开路径
#define TEXT_DIR (QString("C:/Users/lei/Desktop/读书/经典/"))
//获取图片文件的默认打开路径
#define PICTURE_DIR (QString("C:/Users/lei/Desktop/pictures/"))

//保存章节内对key的搜索结果的数据结构，每一条QPair为一项，其中包含搜索结果的上下文QString，和该结果在本章内出现的次序
typedef QVector<QPair<QString,int>> Search_Result;

//保存章节信息的类
class Chapter{
public:
    //初始化函数
    Chapter(int Vol_index=0,int Chapter_index=0,QString Name="0",QString TXT="",Search_Result Result=Search_Result());
    //用输入流对象来初始化，该In对象必须为只读类型的QDataStream对象
    Chapter(QDataStream &In);
    //将章节信息通过数据流对象写入文件
    void write(QDataStream &Out);

    bool operator==(const Chapter c);
    int vol_index;      //卷标
    int chapter_index;  //章序
    QString name;       //章节名
    QString txt;        //正文
    Search_Result context_of_results;//存储每一个搜索结果的上下文以及出现的次序
};

//为QVariant登记自定义类型
Q_DECLARE_METATYPE(Chapter*)

//保存书籍信息的类
class Book_Info
{
public:
    Book_Info();
    //通过数据流对象读入书籍信息
    void read(QDataStream &In);
    //将书籍信息通过数据流对象写入文件
    void write(QDataStream &Out);
    //清空书籍信息，使其回到初始状态
    void clear();
    QString name;           //书名
    QPixmap picture;        //封面
    QString description;    //简介
};
class Book
{
public:
    Book();
    void open(QString book_dir);//打开书籍文件
    void load();                //从TXT文件导入书籍
    void create();              //创建新书籍文件
    void save();                //保存更改
    void save_as();             //保存更改
    void remove();              //从目录删除

    void clear();               //格式化书籍
    void clear_search_result(); //清除每一章内储存的搜索结果

    //插入新章节
    void insert_chapter(int volume_index,int chapter_index,QString Name,QString TXT="");
    //获取某个章节的引用
    Chapter& chapter_at(int volume_index,int chapter_index);
    //在最后一卷追加章节
    void add_chapter(QString Name,QString TXT="");
    //删除章节
    void rm_chapter(const Chapter &c);
    //在末尾添加卷
    void add_volume(const QString &name);
    //删除特定的一卷
    void rm_volume(int volume_index);
    //插入新的卷
    void insert_volume(int pos,const QString &name);
    //改书名，并将存储文件一起改名
    bool rename(QString new_name);
    //对src自动分卷、分章
    void auto_subchapter(const QString &src);

    Book_Info info;                                     //书籍信息
    QVector<QTreeWidgetItem*>  volume_top_level_item;   //每一卷的顶项
    QStringList volume_names;                           //保存每一卷的名字
    QList<QList<Chapter>> data;                         //章节内容，书->卷->章
};


#endif // BOOK_H
