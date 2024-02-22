#include "inc/book.h"
#include<QDir>
#include<QDebug>

//默认构造函数，以“undefine”为命名前缀
Book::Book()
{

}

//清空数据
void Book::clear()
{
    //清空
    info.clear();
    volume_names.clear();
    data.clear();
    volume_top_level_item.clear();

    //重新给定默认命名
    auto list=QDir(TOP_DIR).entryInfoList();
    int count=0;
    //统计有多少以默认命名法命名的书籍
    for(auto&i:list)
    {
        if(i.fileName().contains("undefine"))
            count++;
    }
    QString str =QString::number(count);
    //默认命名
    info.name="undefine"+str;//命名临时文档
}

//打开书籍文件，获取书籍信息和章节信息
void Book::open(QString book_dir)
{
    //参数检查
    if(!book_dir.size())return;
    //打开文件
    QFile f(book_dir);
    if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开文件失败！！！");
        return;
    }
    QDataStream In(&f);
    //清空原来的数据
    clear();

    //开始读入
    //先将书籍信息读入
    info.read(In);
    //读入章节数
    int count=0;
    QString temp_str;
    //读入分卷信息
    volume_names.clear();
    In>>count;
    for(int i=0;i<count;i++)
    {
        In>>temp_str;
        volume_names.push_back(temp_str);
    }
    //逐章读入
    data.clear();
    while(!In.atEnd())
    {
        Chapter c(In);
        if(c.vol_index>data.size()-1)
        {
            data.push_back(QList<Chapter>());
        }
        add_chapter(c.name,c.txt);
    }
    f.close();
}
QString load_preview(QString &book_name,const QByteArray &src)
{
    QByteArray preview_str=src.left(3200);
    QString res(src);

    QDialog w;
    w.resize(800,600);
    w.setWindowTitle("预览");
    QVBoxLayout main_layout;
    main_layout.setMargin(0);
    w.setLayout(&main_layout);

    QFormLayout layout;
    main_layout.addLayout(&layout);
    QLabel L_book_name("书名：");
    QLineEdit edit_name;
    layout.addRow(&L_book_name,&edit_name);
    edit_name.setText(book_name);

    QLabel L_select_type("编码方式");
    QComboBox select_encode;
    select_encode.addItems({"UTF-8","GB18030","UTF-16","UTF-32"});
    layout.addRow(&L_select_type,&select_encode);

    QTextBrowser browse;
    browse.setText(preview_str);
    main_layout.addWidget(&browse);

    QHBoxLayout H_layout_btn;
    main_layout.addLayout(&H_layout_btn);
    QPushButton ok("确认"),cancel("取消");
    H_layout_btn.addWidget(&ok);
    H_layout_btn.addWidget(&cancel);
    ok.setFocusPolicy(Qt::NoFocus);
    cancel.setFocusPolicy(Qt::NoFocus);

    w.connect(&select_encode,&QComboBox::currentTextChanged,[&](const QString &text){
        QTextCodec *codec = QTextCodec::codecForName(text.toUtf8());
        browse.setText(codec->toUnicode(preview_str));
        res=codec->toUnicode(src);
    });
    w.connect(&ok,&QPushButton::clicked,[&]{
        book_name=edit_name.text();
        w.close();
    });
    w.connect(&cancel,&QPushButton::clicked,[&]{
        res.clear();
        w.close();
    });
    w.exec();
    return res;
}


//分章函数
void Book::auto_subchapter(const QString &src)
{
    QString found_new_volume_name;
    volume_names.clear();
    data.clear();
    volume_top_level_item.clear();

    add_volume("第0卷");
    //序章
    QString para="",doc="";//para段落，doc全章正文
    QString last_chapter_name="正文前";//分章录入
    const int max_length=QString("第九千九百九十九章").length()*3;
    for(int i=0;i<src.length();i++)
    {
        //读取一段
        if(src[i]!='\n')
        {
            para+=src[i];
        }
        else//段落末尾，unicode_txt[i]=='\n'
        {
            //首尾去空
            para=para.trimmed();
            //去除空段落
            if(para.isEmpty())
            {
                para.clear();
                continue;
            }
            //如果这一段是卷名
            if(para.size()<max_length&&para[0]=="第"
                    &&(para.contains("卷")||para.contains("部")||para.contains("集"))
                    &&(para.indexOf("卷")<8||para.indexOf("部")<8||para.indexOf("集")<8))
            {
                found_new_volume_name=para.trimmed();
                int p=found_new_volume_name.indexOf(' ');
                if(p>=0)
                    found_new_volume_name=found_new_volume_name.left(p);
            }
            else
                found_new_volume_name.clear();
            //如果这一段是章节名称
            if(para.size()<max_length&&para[0]=="第"&&(para.contains("章")||para.contains("节"))&&(para.indexOf("章")<8||para.indexOf("节")<8))
            {
                if(last_chapter_name==para)
                {
                    para.clear();
                    continue;
                }
                add_chapter(last_chapter_name,doc);
                //将前一章写入contents后才增加新卷
                if(found_new_volume_name.size()&&found_new_volume_name!=volume_names.back())
                    add_volume(found_new_volume_name);
                doc.clear();
                last_chapter_name=para;
                para.clear();
            }
            else//不是章节名，只是普通段
            {
                doc+=para+"\n\n";
                para.clear();
            }
        }
    }
    add_chapter(last_chapter_name,doc);
}

void Book::load()
{
    //取得txt文件的路径
    auto new_file_name=QFileDialog::getOpenFileName(Q_NULLPTR,"从txt文件导入书籍",TEXT_DIR);
    if(!new_file_name.size())
    {
        return;
    }

    //清空原来的数据
    this->clear();

    //打开文件并读取
    QFile f(new_file_name);
    if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"打开文件失败","未能导入书籍！！！");
        return;
    }
    QByteArray array=f.readAll();
    f.close();

    //获取文件名
    info.name=QDir(new_file_name).dirName().split('.')[0];

    //将其转换为utf8代码
    QString src=load_preview(info.name,array);
    if(src.size()==0)return;

    //分章
    auto_subchapter(src);
}

void Book::create()
{
    QDialog w;
    w.setWindowTitle("新建书籍");
    QHBoxLayout mainlayout;
    mainlayout.setMargin(2);
    w.setLayout(&mainlayout);

    QVBoxLayout L_group,R_group;
    L_group.setMargin(0);
    R_group.setMargin(0);
    mainlayout.addLayout(&L_group);
    mainlayout.addLayout(&R_group);

    QLabel label_book_name("书名"),label_picture("封面"),showing_picture("暂无封面"),label_description("简介");
    QLineEdit edit_name,edit_dir;
    QPushButton btn_select_picture("选择图片");
    edit_dir.setEnabled(0);
    QTextEdit edit_description;

    L_group.addWidget(&label_book_name);
    L_group.addWidget(&edit_name);
    L_group.addWidget(&label_picture);
    L_group.addWidget(&edit_dir);
    L_group.addWidget(&showing_picture);
    showing_picture.setPixmap(info.picture);
    L_group.addWidget(&btn_select_picture);
    L_group.addSpacing(150);

    R_group.addWidget(&label_description);
    R_group.addWidget(&edit_description);

    QHBoxLayout H_layout_btn;
    R_group.addLayout(&H_layout_btn);
    QPushButton ok("确认"),cancel("取消");
    H_layout_btn.addWidget(&ok);
    H_layout_btn.addWidget(&cancel);
    ok.setFocusPolicy(Qt::NoFocus);
    cancel.setFocusPolicy(Qt::NoFocus);

    w.connect(&btn_select_picture,&QPushButton::clicked,[&]{
        auto dir=QFileDialog::getOpenFileName(Q_NULLPTR,"选择封面图片",PICTURE_DIR,"Images (*.png *.xpm *.jpg)");
        edit_dir.setText(dir);
        QImage pic(dir);
        showing_picture.setPixmap(QPixmap::fromImage(pic));
        showing_picture.resize(30,40);
    });
    w.connect(&ok,&QPushButton::clicked,[&]{
        if(edit_name.text().size()==0)
        {
            QMessageBox::warning(Q_NULLPTR,"错误","新建书籍失败！\n书名为空！！！");
            return;
        }
        //清空
        clear();
        info.name=edit_name.text();
        info.picture=*showing_picture.pixmap();
        info.description=edit_description.toPlainText();
        //保存新书
        save();
        w.close();
    });
    w.connect(&cancel,&QPushButton::clicked,[&]{
        w.close();
    });
    w.exec();
}

void Book::save()
{
    QFile f;
    QString file_name;
    //规范书名
    if(info.name.left(8)=="undefine"||!info.name.size())
    {
        file_name=QFileDialog::getSaveFileName(Q_NULLPTR,"保存文件",TOP_DIR);
        if(file_name.size()==0)return;
        while(!file_name.size())
        {
            QMessageBox::warning(Q_NULLPTR,"错误","文件名为空！！！");
            file_name=QFileDialog::getSaveFileName(Q_NULLPTR,"保存文件",TOP_DIR);
        }

        f.setFileName(file_name);
        info.name=file_name.split("/").last();
    }
    else
        f.setFileName(TOP_DIR+info.name);
    //打开书籍
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","创建文件失败！！！");
        return;
    }
    //写入文件
    QDataStream out(&f);

    //先将书籍信息写入文件
    info.write(out);
    //写入分卷信息
    out<<volume_names.size();
    for(auto &i:volume_names)
    {
        out<<i;
    }
    //逐章写入文件
    for(auto &i:data)
    {
        for(auto j:i)
        {
            j.write(out);
        }
    }
    f.close();
}

void Book::save_as()
{
    auto res=QFileDialog::getSaveFileName(Q_NULLPTR,"另存为","./","*.txt");
    if(res.size()==0)
        return;
    QFile f(res);
    if(f.open(QIODevice::WriteOnly)==0)
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开文件失败！！！");
        return;
    }
    QTextStream out(&f);
    out<<this->info.name;
    out<<info.description;
    for(int v=0;v<data.size();v++)
    {
        out<<volume_names[v]<<"\n\n\n";
        for(auto c:data[v])
        {
            out<<c.name;
            out<<"\n\n";
            out<<c.txt<<"\n\n";
        }
    }
    f.close();
}

void Book::remove()
{
    auto file=TOP_DIR+info.name;
    if(!QDir().remove(file))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","删除操作失败！");
    }
}

void Book::clear_search_result()
{
    for(auto&j:data)
    {
        for(auto &i:j)
        {
            i.context_of_results.clear();
        }
    }
}

void Book::insert_chapter(int volume_index, int chapter_index, QString Name, QString TXT)
{
    Chapter c(volume_index,chapter_index,Name,TXT);
    if(volume_index<0||volume_index>=data.size())
        return;
    if(chapter_index<0||chapter_index>data[volume_index].size())
        return;
    data[volume_index].insert(chapter_index,c);

    //更新相关卷序号和章序号
    int i=0;
    for(auto &c:data[volume_index])
    {
        c.chapter_index=i;
        i++;
    }
}

Chapter &Book::chapter_at(int volume_index, int chapter_index)
{
    return data[volume_index][chapter_index];
}

void Book::add_chapter(QString Name, QString TXT)
{
    if(Name.size()==0)return;
    insert_chapter(data.size()-1,data[data.size()-1].size(),Name,TXT);
}

void Book::rm_chapter(const Chapter &c)
{
    data[c.vol_index].removeAt(c.chapter_index);
    //更新相关卷序号和章序号
    int i=0;
    for(auto &chapter:data[c.vol_index])
    {
        chapter.chapter_index=i;
        i++;
    }
}

void Book::add_volume(const QString &name)
{
    volume_names.push_back(name);
    data.push_back(QList<Chapter>());
}

void Book::rm_volume(int volume_index)
{
    volume_names.removeAt(volume_index);
    data.removeAt(volume_index);
    for(int v=0;v<data.size();v++)
    {
        for(int c=0;c<data[v].size();c++)
        {
            data[v][c].vol_index=v;
        }
    }
}

void Book::insert_volume(int pos, const QString &name)
{
    if(pos<0)pos=0;
    if(pos>data.size())
    {
        pos=data.size();
    }
    volume_names.insert(pos,name);
    data.insert(pos,QList<Chapter>());
    //更新相关卷序号和章序号
    for(int v=pos+1;v<data.size();v++)
    {
        for(auto &c:data[v])
        {
            c.vol_index=v;
        }
    }
}

bool Book::rename(QString new_name)
{
    auto old_name=info.name;
    info.name=new_name; //改书名
    if(old_name==new_name)return 1;

    if(!QFile::exists(TOP_DIR+old_name))
        this->save();
    else
    {
        save();
        return QDir().remove(TOP_DIR+old_name);
    }

    return 1;
}

Chapter::Chapter(int Vol_index,int Chapter_index,QString Name, QString TXT, Search_Result Result)
{
    vol_index=Vol_index;
    chapter_index=Chapter_index;
    name=Name;
    txt=TXT;
    context_of_results=Result;
}


Chapter::Chapter(QDataStream &In)
{
    In>>vol_index;
    In>>chapter_index;
    In>>name;
    In>>txt;
}

void Chapter::write(QDataStream &Out)
{
    Out<<vol_index;
    Out<<chapter_index;
    Out<<name;
    Out<<txt;
}

bool Chapter::operator==(const Chapter c)
{
    if(     chapter_index==c.chapter_index&&
            vol_index==c.vol_index&&
            name==c.name&&
            txt==c.txt)
        return 1;
    else
        return 0;
}

Book_Info::Book_Info()
{
    //获取已存在书籍信息
    auto list=QDir(TOP_DIR).entryInfoList();
    int count=0;
    //统计有多少以默认命名法命名的书籍
    for(auto&i:list)
    {
        if(i.fileName().contains("undefine"))
            count++;
    }
    //默认命名
    name="undefine"+QString::number(count);//命名临时文档
    //默认封面
    picture=QPixmap::fromImage(QImage(":/bmp/book.bmp"));
}

void Book_Info::read(QDataStream &In)
{
    In>>name;
    In>>description;
    In>>picture;
    if(picture.isNull())
    {
        //默认封面
        picture=QPixmap::fromImage(QImage(":/bmp/book.bmp"));
    }
}

void Book_Info::write(QDataStream &Out)
{
    Out<<name;
    Out<<description;
    Out<<picture;
}

void Book_Info::clear()
{
    //获取已存在书籍信息
    auto list=QDir(TOP_DIR).entryInfoList();
    int count=0;
    //统计有多少以默认命名法命名的书籍
    for(auto&i:list)
    {
        if(i.fileName().contains("undefine"))
            count++;
    }
    //默认命名
    name="undefine"+QString::number(count);//命名临时文档
    description.clear();
    //默认封面
    picture=QPixmap::fromImage(QImage(":/bmp/book.bmp"));
}