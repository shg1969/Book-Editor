#include "inc/mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileSystemModel>
#include <QSizePolicy>
#include <QPoint>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug()<<QDir::currentPath();
    ui->setupUi(this);
    //创建必须的文件夹
    if(!QDir(TOP_DIR).exists())
        QDir().mkpath(TOP_DIR);

    //检查文件夹与文件存在与否
    QDir().mkpath(NOTE_DIR);
    if(!QFile::exists(SETTING_FILE))
    {
//        QDir().mkpath(SETTING_FILE);
        QFile f(SETTING_FILE);
        f.open(QIODevice::WriteOnly);
        f.close();
    }
    if(!QFile::exists(NOTE_CONTENT))
    {
        QFile f(NOTE_CONTENT);
        f.open(QIODevice::WriteOnly);
        f.close();
    }
    else
    {
        //加载笔记
        QFile f(NOTE_CONTENT);
        f.open(QIODevice::ReadOnly);
        QDataStream In(&f);
        QStringList i;
        while(!f.atEnd())
        {
            In>>i;
            if(i.count())
                key_notes[i.value(0)]=i;
            i.clear();

        }
        f.close();
    }

    //载入配置信息
    readSettings();

    //最大化显示
    //    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint& ~Qt::WindowMinimizeButtonHint);
    //    setWindowFlags(windowFlags()&~(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint));
    showMaximized();

    auto_set_focus=0;
    setWindowTitle(APP_NAME);

    //关闭搜索结果窗口
    ui->search_result_dock->close();

    ui->show_nodes_dockWidget->close();

    // 调整水平分布比例
    ui->general_splitter->setStretchFactor(1,QSizePolicy::Minimum);
    // 调整目录页垂直分布比例
    ui->book_dir_splitter->setSizes({25,ui->book_dir_splitter->height()-25});
    //导入文件目录以浏览
    renew_file_browse_tree();
    //字体工具栏
    QToolBar*font_toolbar=new QToolBar(this);
    addToolBar(font_toolbar);
    QFontComboBox *select_font_tpye=new QFontComboBox(this);
    //设置字体类型
    font_toolbar->addWidget(select_font_tpye);
//    renew_viewport_format(select_font_tpye->currentFont().family());
    select_font_tpye->setCurrentFont(fmt.font());
    connect(select_font_tpye,&QFontComboBox::currentFontChanged,[this](const QFont &font){renew_viewport_format(font.family());});
    //设置字体大小
    auto spinBox_Size = new QSpinBox(font_toolbar);
    font_toolbar->addWidget(new QLabel("字体大小：",this));
    font_toolbar->addWidget(spinBox_Size);
    spinBox_Size->setRange(4,100);
    spinBox_Size->setValue(fmt.fontPointSize());
//    renew_viewport_format(18);
    connect(spinBox_Size, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, static_cast<void(MainWindow::*)(int)>(&MainWindow::renew_viewport_format));
    font_toolbar->addSeparator();

    //字体颜色和背景色
    auto btn_font_color=new QPushButton("字体颜色",this);
    font_toolbar->addWidget(btn_font_color);

    connect(btn_font_color,&QPushButton::clicked,[&]()
    {
        renew_viewport_format(QColorDialog::getColor());
    });
    auto btn_back_color=new QPushButton("背景颜色",this);
    font_toolbar->addWidget(btn_back_color);
    connect(btn_back_color,&QPushButton::clicked,[&](){
        if(ui->tabWidget->count()<0)return;
        auto w=dynamic_cast<QTextEdit*>(ui->tabWidget->currentWidget());
        int r=0,g=0,b=0;
        auto color=QColorDialog::getColor();
        color.getRgb(&r,&g,&b);
        this->background_color_sheet="background-color:rgb("
                +QString::number(r)+","
                +QString::number(g)+","
                +QString::number(b)+")";
        w->setStyleSheet(background_color_sheet);
    });

    //窗口右下角显示字数
    label_count_word=new QLabel(ui->statusBar);
    this->ui->statusBar->addPermanentWidget(label_count_word);

    //在窗口左上角显示当前书籍名称
    setWindowTitle(APP_NAME+"@"+book.info.name);

    is_modefied=0;
    search_option=0;

//    on_actionDark_Mode_triggered(0);
}

MainWindow::~MainWindow()
{
    writeSettings();
    delete ui;
}

void MainWindow::renew_tab_name()
{
    auto index=ui->tabWidget->currentIndex();
    if(index<0)return;

    auto tab_name=ui->tabWidget->tabText(index);
    if(dynamic_cast<QTextEdit*>(ui->tabWidget->currentWidget()))
        if(tab_name.size()<3||(tab_name[0]!='*'&&tab_name[0]!=' '))
            ui->tabWidget->setTabText(index,"* "+tab_name);
}

void MainWindow::on_actionOpen_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续打开新文件？",QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return;
    }
    QDialog w;
    w.setWindowTitle("书架");

    QVBoxLayout layout;
    layout.setMargin(0);
    w.setLayout(&layout);
    QListWidget listwidget;
    layout.addWidget(&listwidget);

    QStringList book_list=QDir(TOP_DIR).entryList();
    //去除两个隐藏的相对路径
    book_list.removeOne(".");
    book_list.removeOne("..");

    listwidget.addItems(book_list);

    connect(&listwidget,&QListWidget::itemClicked,[&](QListWidgetItem *item){
        book.open(TOP_DIR+item->text());
        set_saved_flag();
        refresh();
        w.close();
    });

    w.exec();
}

void MainWindow::renew_dir()
{
    //清空原目录
    ui->dir_treeWidget->clear();
    //新建目录
    book.volume_top_level_item.clear();

    if(ui->show_volume_checkBox->isChecked())
    {
        for(int vol_index=0;vol_index<book.data.size();vol_index++)
        {
            QTreeWidgetItem *new_item=new QTreeWidgetItem({book.volume_names[vol_index]});
//            new_item->setBackgroundColor(0,QColor::fromRgb(168,172,158));
            new_item->setData(0,Qt::WhatsThisRole,QString("volume"));
            for(auto &c:book.data[vol_index])
            {
                auto child=new QTreeWidgetItem({c.name});
                child->setData(0,Qt::UserRole,QVariant::fromValue(&c));
                new_item->addChild(child);
            }
            book.volume_top_level_item.push_back(new_item);
            ui->dir_treeWidget->addTopLevelItem(new_item);
        }
    }
    else
    {
        for(auto &v:book.data)
        {
            for(auto &c:v)
            {
                QTreeWidgetItem *new_item=new QTreeWidgetItem({c.name});
                //将章节指针保存到列表项中
                new_item->setData(0,Qt::UserRole,QVariant::fromValue(&c));
                ui->dir_treeWidget->addTopLevelItem(new_item);
            }
        }
    }
    ui->statusBar->showMessage("更新目录成功！但未保存！",1000);
}

void MainWindow::on_actionLoad_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续打开新文件？",QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return;
    }
    book.load();
    is_modefied=1;
    refresh();
    renew_window_title();
}

QTextEdit *MainWindow::current_page()
{
    if(ui->tabWidget->currentWidget()==Q_NULLPTR)return Q_NULLPTR;
    else
    {
        return dynamic_cast<QTextEdit*>(ui->tabWidget->currentWidget());
    }
}

//void MainWindow::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
//{
//    //获取光标
//    if(current_page()==Q_NULLPTR)return;
//    QTextCursor cursor = current_page()->textCursor();
//    //如果光标没选中文本
//    if (!cursor.hasSelection())
//        cursor.select(QTextCursor::WordUnderCursor);//选中“”单词块”（连续的字母）
//    //光标选中了文本,改变光标所选字符的格式，设置后续输入字符的格式
//    cursor.mergeCharFormat(format);

//    auto current_edit=current_page();
//    if(current_edit==Q_NULLPTR)
//        return;
//    current_edit->mergeCurrentCharFormat(format);
//    auto a=QString(current_page()->toPlainText());
//    current_page()->setText(a);
//}

void MainWindow::on_actionNew_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续打开新文件？",QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return;
    }
    book.create();
    set_saved_flag();
    refresh();
}

void MainWindow::renew_window_title()
{
    if(is_modefied)
        setWindowTitle(APP_NAME+"@ *"+book.info.name);
    else
        setWindowTitle(APP_NAME+"@"+book.info.name);
}

void MainWindow::refresh()
{
    //在窗口左上角显示当前书籍名称
    renew_window_title();
    //清除已打开的所有标签页
    int index=ui->tabWidget->currentIndex();
    while(index>=0)
    {
        ui->tabWidget->removeTab(index);
        index=ui->tabWidget->currentIndex();
    }
    //清空打开记录
    opened_chapters.clear();
    //更新书架
    renew_file_browse_tree();
    //更新目录
    renew_dir();
}
//章节经过保存后，处理*标志
void MainWindow::set_saved_flag()
{
    is_modefied=0;
    renew_window_title();
    for(auto i=0;i<ui->tabWidget->count();i++)
    {
        auto s=ui->tabWidget->tabText(i);
        if(s.left(2)=="* ")
            ui->tabWidget->setTabText(i,s.right(s.size()-2));
    }
}

void MainWindow::renew_viewport_format()
{
    auto w=current_page();
    if(!w)return;

    if(w->currentCharFormat()==fmt)return;
    //背景颜色
    w->setStyleSheet(this->background_color_sheet);
    //字体格式
//    w->mergeCurrentCharFormat(fmt);
    w->setCurrentCharFormat(fmt);

    auto corsor=w->textCursor();
    corsor.select(QTextCursor::Document);
    corsor.setCharFormat(fmt);
    corsor.clearSelection();
}
void MainWindow::renew_viewport_format(int size)
{
    fmt.setFontPointSize(size);

    auto w=current_page();
    if(!w)return;
    //字体格式
//    w->mergeCurrentCharFormat(fmt);
    w->setCurrentCharFormat(fmt);
    writeSettings();

    auto corsor=w->textCursor();
    corsor.select(QTextCursor::Document);
    corsor.setCharFormat(fmt);
    corsor.clearSelection();
}
void MainWindow::renew_viewport_format(QColor font_color)
{
    auto w=current_page();
    if(!w)return;

    fmt.setForeground(font_color);

    //字体格式
//    w->mergeCurrentCharFormat(fmt);
    w->setCurrentCharFormat(fmt);
    writeSettings();

    auto corsor=w->textCursor();
    corsor.select(QTextCursor::Document);
    corsor.setCharFormat(fmt);
    corsor.clearSelection();
}
void MainWindow::renew_viewport_format(const QString &font_family)
{
    auto w=current_page();
    if(!w)return;

    fmt.setFontFamily(font_family);

    //字体格式
//    w->mergeCurrentCharFormat(fmt);
    w->setCurrentCharFormat(fmt);
    writeSettings();

    auto corsor=w->textCursor();
    corsor.select(QTextCursor::Document);
    corsor.setCharFormat(fmt);
    corsor.clearSelection();
}

void MainWindow::renew_word_num_showing()
{
    //统计本章字数
    auto w=this->current_page();
    if(w==Q_NULLPTR)
    {
        label_count_word->setText(COUNT_WORD_MSG(0));
        return;
    }
    auto str=w->toPlainText();
    int count=0;
    for(auto i:str)
    {
        if(!i.isSpace())
            count++;
    }
    label_count_word->setText(COUNT_WORD_MSG(count));
}

void MainWindow::renew_file_browse_tree()
{
    //清空
    ui->file_browse_listWidget->clear();

    auto file_list=QDir(TOP_DIR).entryList();

    QFile f;
    QDataStream In;
    Book_Info book_info;
    for(auto &i:file_list)
    {
        if(i=="."||i=="..")continue;
        QListWidgetItem *new_item=new QListWidgetItem({i});
//        new_item->setBackgroundColor(0,QColor::fromRgb(188,192,158));
        ui->file_browse_listWidget->addItem(new_item);
        //显示封面
        //读入图标
        f.setFileName((TOP_DIR+i));
        f.open(QIODevice::ReadOnly);
        In.setDevice(&f);
        book_info.read(In);
        f.close();
        if(!book_info.picture.isNull())
        {
            new_item->setIcon(QIcon(book_info.picture));
        }
    }
    ui->file_browse_listWidget->setIconSize(QSize(50,100));
}

void MainWindow::on_actionSave_triggered()
{
    //更新正文
    for(auto &i:opened_chapters.keys())
    {
        i->txt=opened_chapters[i]->toPlainText();
    }
    //更新分卷
    book.volume_names.clear();
    for(int i=0;i<ui->dir_treeWidget->topLevelItemCount();i++)
    {
        auto item=ui->dir_treeWidget->topLevelItem(i);
        if(item)
        {
            book.volume_names.push_back(item->text(0));
        }
    }
    book.save();                //保存书籍
    set_saved_flag();           //更新标识
    renew_file_browse_tree();   //更新书架
    on_note_save_btn_clicked(); //保存笔记
}

void MainWindow::on_actionSave_as_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::question(Q_NULLPTR,"另存为","所作出的更改是否保存到文件？");
        if(ans==QMessageBox::Yes)
        {
            //将编辑框内的内容写入book对应的内存
            for(auto &i:opened_chapters.keys())
                i->txt=opened_chapters[i]->toPlainText();
        }
    }
    book.save_as();
    set_saved_flag();
    renew_file_browse_tree();
}

void MainWindow::on_actionRemove_triggered()
{
    auto ans=QMessageBox::warning(this,"警告","永久删除该书籍，不可恢复，是否继续？",QMessageBox::Yes|QMessageBox::No);
    if(ans!=QMessageBox::Yes)return;

    book.remove();
    book.clear();
    refresh();
    set_saved_flag();
}

void MainWindow::on_actionClose_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续关闭该书？",QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return;
    }
    book=Book();
    refresh();
    set_saved_flag();
}

void MainWindow::on_actionAuthor_triggered()
{
    QMessageBox::about(Q_NULLPTR,"关于作者","雷");
}

void MainWindow::on_actionQt_triggered()
{
    QMessageBox::aboutQt(Q_NULLPTR);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    if(ui->tabWidget->count()<=0)return;
    //将编辑过的内容加载到内存
    if(ui->tabWidget->tabText(index).left(2)=="* ")
    {
        auto ans=QMessageBox::question(Q_NULLPTR,"关闭章节","该章节内容经过了编辑，是否保存到内存（更未保存到文件）");
        if(ans==QMessageBox::Yes)
        {
            auto w=dynamic_cast<QTextEdit*>(ui->tabWidget->widget(index));
            auto key=opened_chapters.key(w);
            key->txt=w->toPlainText();
            opened_chapters.remove(key);
        }
    }
    ui->tabWidget->removeTab(index);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    index=0;//无用功，避免警告
    if(ui->tabWidget->count()<=0)return;
    //统计本章字数
    auto str=dynamic_cast<QTextEdit*>(ui->tabWidget->currentWidget())->toPlainText();
    int count=0;
    for(auto i:str)
    {
        if(!i.isSpace())
            count++;
    }
    renew_word_num_showing();
    //同步文字显示格式
    renew_viewport_format();
}

void MainWindow::on_actionAdd_Chapter_triggered()
{
    if(book.data.size()==0)
    {
        auto res=QMessageBox::warning(Q_NULLPTR,"错误","尚未有分卷，是否创建第一个分卷？",
                             QMessageBox::Yes|QMessageBox::No);
        if(res==QMessageBox::Yes)
        {
            on_actionAdd_volume_triggered();
        }
        else
            return;
    }
    //添加章节最关键的是要找到目标位置的卷标和章序号
    int volume_index=0,chapter_index=0;
    //确定卷标
    auto current_item=ui->dir_treeWidget->currentItem();//当前项
    qDebug()<<current_item;
    if(current_item!=Q_NULLPTR) //当前项不为空
    {
        auto tell=current_item->data(0,Qt::WhatsThisRole).toString();
        if(tell==QString("volume"))//所选项是卷名
        {
            volume_index=ui->dir_treeWidget->currentIndex().row();
            qDebug()<<"目标卷标"<<volume_index;
            if(current_item->childCount())  //有子项，该卷有章节
            {
                chapter_index=current_item->childCount();//在尽头处，若用其来索引，必定超出范围
            }
            else
                chapter_index=0;           //没有子项，该卷没有章节
        }
        else                                //所选项是章名
        {
            auto selected_chapter=current_item->data(0,Qt::UserRole).value<Chapter*>();
            if(selected_chapter!=Q_NULLPTR)
            {
                volume_index=selected_chapter->vol_index;
                chapter_index=selected_chapter->chapter_index;
            }
        }
    }

    QDialog w;
    w.setWindowTitle("添加新章节");
    QVBoxLayout main_layout;
    main_layout.setMargin(2);
    w.setLayout(&main_layout);

    QLabel L_volume("卷名："+book.volume_names[volume_index]);
    main_layout.addWidget(&L_volume);

    QLabel pos_tips;
    if(book.data.size()&&book.data[volume_index].size()>chapter_index)//有章节作为位置参考
        pos_tips.setText("在章节 “"+book.data[volume_index][chapter_index].name+"” 前插入");
    else                                //没有章节作为位置参考
        pos_tips.setText("在卷 “"+book.volume_names[volume_index]+"” 内追加");
    main_layout.addWidget(&pos_tips);
    QHBoxLayout H_layout_name;
    main_layout.addLayout(&H_layout_name);

    QLabel L_name("章名");
    QLineEdit edit_name;
    QSpinBox spinbox_index;
    spinbox_index.setRange(0,chapter_index+1>book.data[volume_index].size()?chapter_index+1:book.data[volume_index].size());
    spinbox_index.setValue(chapter_index);
    H_layout_name.addWidget(&L_name);
    H_layout_name.addWidget(&edit_name);
    H_layout_name.addWidget(&spinbox_index);

    QTextEdit edit_txt;
    main_layout.addWidget(&edit_txt);

    QHBoxLayout H_layout_btn;
    main_layout.addLayout(&H_layout_btn);
    QPushButton ok("确认"),cancel("取消");
    H_layout_btn.addWidget(&ok);
    H_layout_btn.addWidget(&cancel);
    ok.setFocusPolicy(Qt::NoFocus);
    cancel.setFocusPolicy(Qt::NoFocus);

    connect(&ok,&QPushButton::clicked,[&]{
        if(!edit_name.text().size())return;
        //添加到书籍
        book.insert_chapter(volume_index,chapter_index,edit_name.text(),edit_txt.toPlainText());
        w.close();
        //更新目录
        if(ui->show_volume_checkBox->isChecked())
        {
            auto child=new QTreeWidgetItem({edit_name.text()});
            child->setData(0,Qt::UserRole,QVariant::fromValue(&book.chapter_at(volume_index,chapter_index)));
            ui->dir_treeWidget->topLevelItem(volume_index)->insertChild(chapter_index,child);
        }
        else
        {
            renew_dir();
        }
        is_modefied=1;
    });
    connect(&spinbox_index,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),[&](int i){
        //确定插入位置chapter_index
        chapter_index=i;
        if(i<book.data[volume_index].size())
            pos_tips.setText("在章节 “"+book.data[volume_index][i].name+"” 前插入");
        else if(book.data[volume_index].size()==0)
            pos_tips.setText("在“"+book.volume_names[volume_index]+"” 内添加");
        else
            pos_tips.setText("在章节 “"+book.data[volume_index].back().name+"” 后插入");
    });
    connect(&cancel,&QPushButton::clicked,[&]{
        w.close();
    });

    w.exec();
}

void MainWindow::on_actionRemove_Chapter_triggered()
{
    //所选为卷名
    if(!ui->dir_treeWidget->currentItem())
    {
        QMessageBox::warning(Q_NULLPTR,"删除章节","选择无效！");
        return;
    }
    auto chapter_p=ui->dir_treeWidget->currentItem()->data(0,Qt::UserRole).value<Chapter*>();
    if(chapter_p==Q_NULLPTR)return;

    auto ans=QMessageBox::warning(Q_NULLPTR,"删除章节","确认要删除以下章节吗？\n“"
                                          +chapter_p->name+"”",QMessageBox::Yes|QMessageBox::No);
    if(ans!=QMessageBox::Yes)return;
    //关闭对应的Tab
    ui->tabWidget->setCurrentWidget(opened_chapters[chapter_p]);
    ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
    //从打开章节的记录中删除
    opened_chapters.remove(chapter_p);
    //从书籍数据结构中删除
    book.rm_chapter(*chapter_p);
    //更新目录
//    renew_dir();
    delete ui->dir_treeWidget->currentItem();
    is_modefied=1;
}

//删除空白章节
void MainWindow::on_actionRemove_Blank_Chapter_triggered()
{
    int count=0;
    for(auto&v:book.data)
    {
        for(auto &c:v)
        {
            count=0;
            for(auto j:c.txt)
            {
                if(!j.isSpace())
                {
                    count++;
                    break;
                }
            }
            if(count==0)
            {
                //关闭相对应的页面
                ui->tabWidget->removeTab(ui->tabWidget->indexOf(opened_chapters[&c]));
                opened_chapters.remove(&c);
                //删除章节
                book.rm_chapter(c);
            }
        }
    }
    //更新目录
    renew_dir();
    is_modefied=1;
}

void MainWindow::on_actionAdd_Blank_Lines_triggered()
{
    auto edit=current_page();
    if(edit!=Q_NULLPTR)
    {
        auto str=edit->toPlainText();
        auto list_str=str.split('\n');
        str.clear();
        for(auto i:list_str)
        {
            str+=i+"\n\n";
        }
        edit->setText(str);
    }
    is_modefied=1;
}

void MainWindow::on_actionRemove_Space_triggered()
{
    auto edit=current_page();
    if(edit!=Q_NULLPTR)
    {
        auto str=edit->toPlainText();
        QString s;
        for(auto i:str)
        {
            if(i.isSpace()&&i!='\n')continue;
            s+=i;
        }
        edit->setText(s);
    }
    is_modefied=1;
}

void MainWindow::on_actionDark_Mode_triggered(bool checked)
{
    if(checked)
    {
        this->background_color_sheet="background-color:#474536";
        auto w=current_page();
        if(w)w->setStyleSheet(background_color_sheet);
        renew_viewport_format(QColor(Qt::white));
    }
    else
    {
        renew_viewport_format(QColor(Qt::black));
        this->background_color_sheet="background-color:#a1a554";
        auto w=current_page();
        if(w)w->setStyleSheet(background_color_sheet);
        renew_viewport_format(QColor(Qt::black));
    }
    emit theme_changed(checked);
}

void MainWindow::on_search_option_comboBox_currentTextChanged(const QString &arg1)
{
    qDebug()<<arg1;
    if(arg1==QString("对当前章节正文"))
        search_option=0;
    if(arg1==QString("对全书章节名"))
        search_option=1;
    if(arg1==QString("对全书正文"))
        search_option=2;
}

void MainWindow::search_in_textedit(Chapter *c,QString key_word)
{
    int count=0;//记录出现次数
    //获取搜索关键字长度
    int key_length=key_word.length();
    if(key_length<1)return;
//    //设置标注颜色
//    QTextCharFormat fmt;
//    auto back_col=QColor(245,255,152);
//    auto font_col=QColor(0,85,255);
//    fmt.setForeground(font_col);
//    fmt.setBackground(back_col);

    if(!opened_chapters.contains(c))//对于未打开浏览的章节
    {
        //获取源文本
        auto src=QTextEdit(c->txt,Q_NULLPTR).toPlainText();
        //开始搜索
        int pos=src.indexOf(key_word);
        while(pos>=0)
        {
            count++;
            //获取上下文
            int down=pos-10<0?0:pos-10;
            int top=pos+key_length+30<src.size()?pos+key_length+30:src.size();
            QString temp="";
            for(int i=down;i<top;i++)
            {
                if(src[i]!='\n')
                    temp+=src[i];
                else
                    temp+='|';
            }
            c->context_of_results.push_back({temp,count});
            temp.clear();
            pos=src.indexOf(key_word,pos+key_length);
        }
    }
    else                            //对于已经打开的章节
    {
        auto current_edit_win=opened_chapters[c];
        //获取源文本
        auto src=current_edit_win->toPlainText();
//        //获取光标
//        auto cursor=current_edit_win->textCursor();
        //开始搜索
        int pos=src.indexOf(key_word);
        while(pos>=0)
        {
            count++;
            //获取上下文
            int down=pos-10<0?0:pos-10;
            int top=pos+key_length+30<src.size()?pos+key_length+30:src.size();
            QString temp="";
            for(int i=down;i<top;i++)
            {
                if(src[i]!='\n')
                    temp+=src[i];
                else
                    temp+='|';
            }
            c->context_of_results.push_back({temp,count});
            temp.clear();

//            //选中搜索结果
//            cursor.setPosition(pos);
//            cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,key_length);
//            //标注
//            cursor.mergeCharFormat(fmt);
//            cursor.clearSelection();

            //搜索下一个
            pos=src.indexOf(key_word,pos+key_length);
        }

    }
}
//显示每一章的搜索结果
void MainWindow::show_search_result()
{
    //头部
    QTreeWidgetItem *header=new QTreeWidgetItem({"章名","上下文","章内出现次序"});
    ui->search_treeWidget->setHeaderItem(header);

    ui->search_treeWidget->setColumnWidth(0,300);
    ui->search_treeWidget->setColumnWidth(1,400);
    ui->search_treeWidget->setColumnWidth(2,100);
    ui->search_treeWidget->setColumnWidth(2,100);

//    ui->search_treeWidget

    //显示每一章的结果
    for(auto &v:book.data)
    {
        for(auto&c:v)
        {
            if(!c.context_of_results.size())
            {
                continue;//跳过不包含关键字的章节
            }

    //        QTreeWidgetItem *for_every_chapter=new QTreeWidgetItem({i.name,QString::number(i.context_of_results.size())});
            for(auto j:c.context_of_results)//j为同一章内的每个结果的序号
            {
                QTreeWidgetItem *child=new QTreeWidgetItem({c.name,j.first,QString::number(j.second)});
                child->setData(0,Qt::UserRole,QVariant::fromValue(&c));//  这也是一种储存额外信息的一种办法,存放该章的地址
                ui->search_treeWidget->addTopLevelItem(dynamic_cast<QTreeWidgetItem *>(child));
    //            for_every_chapter->addChild(child);
            }
    //        ui->search_treeWidget->addTopLevelItem(for_every_chapter);
        }
    }

    ui->search_result_dock->show();
}

void MainWindow::on_find_all_btn_clicked()
{
    //清除原有记录
    book.clear_search_result();
    //清除以前的记录
    ui->search_treeWidget->clear();

    //获取搜索结果
    auto key_word=ui->search_key_edit->text();
    if(!key_word.size())
    {
        QMessageBox::warning(Q_NULLPTR,"错误","关键字为空！");
        return;
    }
    //0:对当前章节正文，1：对全书章节名，2：对全书正文
    if(search_option==0)//对当前章节正文
    {
        if(ui->tabWidget->count()==0)
        {
            QMessageBox::warning(Q_NULLPTR,"错误","没有任何被打开的章节！");
            return;
        }
        //搜索，结果将存放在每个章节对象中
        if(!ui->dir_treeWidget->currentItem())
        {
            QMessageBox::warning(Q_NULLPTR,"删除章节","选择无效！");
            return;
        }
        auto chapter_p=ui->dir_treeWidget->currentItem()->data(0,Qt::UserRole).value<Chapter*>();
        if(chapter_p==Q_NULLPTR)return;
        search_in_textedit(chapter_p,key_word);
        //在分支显示窗口中显示结果
        show_search_result();
    }
    else if(search_option==1)//对全书章节名
    {
        for(auto&v:book.data)
        {
            for(auto&c:v)
            {
                if(c.name.contains(key_word))
                    c.context_of_results.push_back({c.name,0});
            }
        }
        //在分支显示窗口中显示结果
        show_search_result();
    }
    else if(search_option==2)//对全书正文
    {
        for(auto&v:book.data)
        {
            for(auto&c:v)
            {
                //搜索，结果将存放在每个章节对象中
                search_in_textedit(&c,key_word);
            }
        }
        //在分支显示窗口中显示结果
        show_search_result();
    }
    else
    {
        QMessageBox::warning(Q_NULLPTR,"错误","没有该选项！");
    }
}

void MainWindow::on_auto_focus_checkBox_stateChanged(int arg1)
{
    auto_set_focus=arg1;
}

void MainWindow::on_search_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    if(column!=3)
    {
        qDebug()<<"非目的行";
    }
    bool ok1=1,ok2=1;
    //从树状表中获取数据
    Chapter* c_p=item->data(0,Qt::UserRole).value<Chapter*>();
    int apparence_count=item->data(2,Qt::DisplayRole).toString().toInt(&ok1);
    if(!(ok1&&ok2))
    {
        QMessageBox::warning(Q_NULLPTR,"转换出错","无法将字符转换为数字");
        return;
    }
    qDebug()<<"次序"<<apparence_count<<endl;

    //打开章节
    if(!opened_chapters.contains(c_p))
    {
        QTextEdit *new_tab=new QTextEdit(this);
        opened_chapters.insert(c_p,new_tab);
        ui->tabWidget->addTab(new_tab,c_p->name);
        new_tab->setText(c_p->txt);
        connect(new_tab,&QTextEdit::textChanged,[this]{
            is_modefied=1;
            renew_tab_name();
            renew_word_num_showing();
        });
    }
    auto textedit=opened_chapters[c_p];
    //重置光标位置
    auto cursor=textedit->textCursor();
    cursor.setPosition(0);
    textedit->setTextCursor(cursor);
    ui->tabWidget->setCurrentWidget(textedit);
    //涂色
    search_in_textedit(c_p,ui->search_key_edit->text());
    //定位
    for(int i=0;i<apparence_count;i++)
    {
        QPalette palette = textedit->palette();
        palette.setColor(QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
        textedit->setPalette(palette);

        textedit->find(ui->search_key_edit->text());
    }

}

void MainWindow::on_actionBook_info_triggered()
{
    QDialog w;
    w.setWindowTitle("书籍信息");

    QVBoxLayout main_layout;
    QHBoxLayout layout_name;
    QHBoxLayout layout_description;
    QVBoxLayout layout_picture;
    QHBoxLayout layout_btn;

    w.setLayout(&main_layout);
    main_layout.addLayout(&layout_name);
    main_layout.addLayout(&layout_description);
    main_layout.addLayout(&layout_picture);
    main_layout.addLayout(&layout_btn);


    QLabel l_name("书名："),L_description("简介："),L_picture("封面："),L_showing_pic;
    QTextEdit edit_description(book.info.description);
    QLineEdit edit_name(book.info.name);
    L_showing_pic.setPixmap(book.info.picture);

    layout_name.addWidget(&l_name);
    layout_name.addWidget(&edit_name);

    layout_description.addWidget(&L_description);
    layout_description.addWidget(&edit_description);

    QPushButton select_pic("添加封面"),rm_pic("删除封面");
    layout_picture.addWidget(&L_picture);
    layout_picture.addWidget(&L_showing_pic);
    layout_picture.addWidget(&select_pic);
    layout_picture.addWidget(&rm_pic);
    rm_pic.hide();
    if(!book.info.picture.isNull())
    {
        select_pic.setText("更改封面");
        rm_pic.show();
    }

    QPushButton ok("确认修改"),cancel("结束浏览");
    layout_btn.addWidget(&ok);
    layout_btn.addWidget(&cancel);

    ok.setFocusPolicy(Qt::NoFocus);
    cancel.setFocusPolicy(Qt::NoFocus);
    select_pic.setFocusPolicy(Qt::NoFocus);
    rm_pic.setFocusPolicy(Qt::NoFocus);

    connect(&select_pic,&QPushButton::clicked,[&]{
        QString pic_dir=QFileDialog::getOpenFileName(Q_NULLPTR,"选择封面图片",PICTURE_DIR,"Images (*.png *.xpm *.jpg)");
        if(pic_dir.size()==0)return;
        L_showing_pic.setPixmap(QPixmap::fromImage(QImage(pic_dir)));
        select_pic.setText("更改封面");
        rm_pic.show();
    });
    connect(&rm_pic,&QPushButton::clicked,[&]{
        L_showing_pic.setPixmap(QPixmap());
        select_pic.setText("添加封面");
        rm_pic.hide();
    });

    connect(&edit_name,&QLineEdit::returnPressed,[&]{
        edit_description.setFocus();
    });
    //更新书籍的编辑与保存情况
    connect(&ok,&QPushButton::clicked,[&]{
        book.rename(edit_name.text());
        book.info.description=edit_description.toPlainText();
        if(L_showing_pic.pixmap()!=Q_NULLPTR)
        {
            book.info.picture=*L_showing_pic.pixmap();
        }
        qDebug()<<edit_name.text();
        w.close();
        renew_window_title();
        is_modefied=1;
    });
    connect(&cancel,&QPushButton::clicked,[&]{
        w.close();
    });

    w.exec();
}

void MainWindow::on_find_next_btn_clicked()
{
    auto textedit=current_page();
    QPalette palette = textedit->palette();
    palette.setColor(QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
    textedit->setPalette(palette);
    if(textedit==Q_NULLPTR)
    {
        QMessageBox::warning(Q_NULLPTR,"错误","未打开正文浏览页面");
        return;
    }
    if(!textedit->find(ui->search_key_edit->text()))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","本章后续无搜索结果\n下次将从本章重新开始");
        auto cursor=textedit->textCursor();
        cursor.setPosition(0);
        textedit->setTextCursor(cursor);
        return;
    }
}

void MainWindow::on_replace_current_btn_clicked()
{
    auto w=current_page();
    if(w==Q_NULLPTR)return;

    auto cursor=w->textCursor();
    if(!cursor.hasSelection())return;
    cursor.deleteChar();
    cursor.insertText(ui->obj_kew_edit->text());

    is_modefied=1;
}

void MainWindow::on_replace_all_btn_clicked()
{
    QStringList s;
    for(auto&v:book.data)
    {
        for(auto&c:v)
        {
            s=c.txt.split(ui->search_key_edit->text());
            c.txt=s[0];
            for(int i=1;i<s.size();i++)
            {
                c.txt+=ui->obj_kew_edit->text()+s[i];
            }

            if(opened_chapters.contains(&c))
            {
                opened_chapters[&c]->setText(c.txt);
            }
        }
    }
    is_modefied=1;
    on_find_all_btn_clicked();
}
/*
void MainWindow::on_dir_listWidget_itemClicked(QListWidgetItem *item)
{
    auto chapter_p=item->data(Qt::UserRole).value<Chapter*>();
    if(chapter_p==Q_NULLPTR)return;
    qDebug()<<chapter_p->name;

    if(!opened_chapters.contains(chapter_p))
    {
        QTextEdit *new_tab=new QTextEdit(this);
        new_tab->setAcceptRichText(0);
        opened_chapters.insert(chapter_p,new_tab);
        ui->tabWidget->addTab(new_tab,chapter_p->name);
        new_tab->setText(chapter_p->txt);
        //更新字数显示
        show_word_num();
        connect(new_tab,&QTextEdit::textChanged,[this]{emit this->chapter_modified();});
        //自动聚焦
        connect(new_tab,&QTextEdit::selectionChanged,[this]{
            auto current_edit=current_page();
            if(current_edit==Q_NULLPTR)return;
            ui->note_content_LineEdit->setText(current_edit->textCursor().selectedText());

            if(auto_set_focus==0)return;
            ui->note_key_LineEdit->setFocus();
            ui->note_key_LineEdit->selectAll();
        });
    }
    ui->tabWidget->setCurrentWidget(opened_chapters[chapter_p]);
    ui->tabWidget->currentWidget()->setFocus();
}*/

void MainWindow::on_note_key_LineEdit_returnPressed()
{
    auto key=ui->note_key_LineEdit->text();
    auto content=ui->note_content_LineEdit->text();

    if(key_notes.contains(key))
    {
        key_notes[key].append(content);
    }
    else
    {
        key_notes[key].append(key);
        key_notes[key].append(content);
        ui->note_key_listWidget->addItem(key);
    }
    is_modefied=1;
}

void MainWindow::on_note_save_btn_clicked()
{
    QFile f(NOTE_CONTENT);
    f.open(QIODevice::WriteOnly);
    QDataStream Out(&f);
    for(auto &i:key_notes)
    {
        Out<<i;
    }
    f.close();
}

void MainWindow::on_note_renew_key_list_btn_clicked()
{
    ui->note_key_listWidget->clear();
    QStringList a;

    for(auto i:key_notes)
    {
        a.append(i[0]);
    }
    a.sort(Qt::CaseInsensitive);
    for(auto i:a)
    {
        if(i.size()==0)continue;
        ui->note_key_listWidget->addItem(i);
    }
}

void MainWindow::on_node_rm_row_btn_clicked()
{
    int obj=ui->note_select_row_spinBox->value();
    key_notes[the_key_showing].removeAt(obj);

    ui->note_textEdit->clear();
    auto key=the_key_showing;
    the_key_showing=key;
    QString s;
    int count=0;
    for(auto &i:key_notes[key])
    {
        s+="\n----"+QString::number(count)+"----\n"+i;
        count++;
    }
    ui->note_textEdit->setText(s);
    ui->show_nodes_dockWidget->show();

    is_modefied=1;
}

void MainWindow::on_note_key_listWidget_itemClicked(QListWidgetItem *item)
{
    ui->note_textEdit->clear();
    the_key_showing=item->text();
    QString s;
    int count=0;
    for(auto &i:key_notes[the_key_showing])
    {
        s+="\n----"+QString::number(count)+"----\n"+i;
        count++;
    }
    //将所选key填入到编辑框
    ui->note_key_LineEdit->setText(the_key_showing);
    ui->note_textEdit->setText(s);
    ui->show_nodes_dockWidget->show();
}

void MainWindow::on_node_rm_key_btn_clicked()
{
    auto ans=QMessageBox::question(Q_NULLPTR,"删除","这将会删除整个索引以及所有相关笔记，确认码？\n"+the_key_showing,
                          QMessageBox::Yes|QMessageBox::No);
    if(ans!=QMessageBox::Yes)return ;
    key_notes.remove(the_key_showing);
    on_note_renew_key_list_btn_clicked();

    is_modefied=1;
}

void MainWindow::on_node_edit_note_btn_clicked()
{
    int obj_index=ui->note_select_row_spinBox->value();

    QString msg="编辑“"+the_key_showing+"”中的第"+QString::number(obj_index)+"项";
    bool ok=0;
    QString old_str=key_notes[the_key_showing][obj_index];
    auto ans=QInputDialog::getMultiLineText(Q_NULLPTR,"编辑",msg,old_str,&ok);
    if(ok)
        key_notes[the_key_showing][obj_index]=ans;
    else
        return;
    //更新显示
    ui->note_textEdit->find(old_str);
    auto cursor=ui->note_textEdit->textCursor();
    cursor.deleteChar();
    cursor.insertText(ans);

    is_modefied=1;
}

void MainWindow::on_note_select_row_spinBox_valueChanged(int arg1)
{
    int max=key_notes[the_key_showing].size();
    if(arg1>=max)
        ui->note_select_row_spinBox->setValue(max-1);
}

void MainWindow::on_actionDelete_bank_row_triggered()
{
    auto edit=current_page();
    if(edit!=Q_NULLPTR)
    {
        auto str=edit->toPlainText();
        edit->clear();
        QString s;
        auto s_list=str.split('\n');
        for(auto i:s_list)
        {
            if(i.isEmpty())continue;
            s+=i+'\n';
        }
        edit->setText(s);
    }
    is_modefied=1;
}

void MainWindow::on_actionAuto_subchapter_triggered()
{
    QString src;
    for(auto &v:book.data)
    {
        for(auto &c:v)
        {
            src+=c.name+'\n'+c.txt+'\n';
        }
    }
    //分章
    book.auto_subchapter(src);
    is_modefied=1;
    refresh();
}

void MainWindow::on_note_content_LineEdit_returnPressed()
{
    on_note_key_LineEdit_returnPressed();
}

void MainWindow::on_dir_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    column=0;

    auto chapter_p=item->data(column,Qt::UserRole).value<Chapter*>();
    if(chapter_p==Q_NULLPTR)return;
    qDebug()<<chapter_p->name;

    if(!opened_chapters.contains(chapter_p))
    {
        QTextEdit *new_tab=new QTextEdit(this);
        new_tab->setAcceptRichText(0);
        opened_chapters.insert(chapter_p,new_tab);
        ui->tabWidget->addTab(new_tab,chapter_p->name);
        new_tab->setText(chapter_p->txt);
        connect(new_tab,&QTextEdit::textChanged,[this]{
            is_modefied=1;
            renew_tab_name();
            renew_word_num_showing();
        });
        //自动聚焦
        connect(new_tab,&QTextEdit::selectionChanged,[this]{
            auto current_edit=current_page();
            if(current_edit==Q_NULLPTR)return;
            ui->note_content_LineEdit->setText(current_edit->textCursor().selectedText());

            if(auto_set_focus==0)return;
            ui->note_key_LineEdit->setFocus();
            ui->note_key_LineEdit->selectAll();
        });
    }
    ui->tabWidget->setCurrentWidget(opened_chapters[chapter_p]);
    ui->tabWidget->currentWidget()->setFocus();
    renew_word_num_showing();
}

void MainWindow::on_dir_treeWidget_customContextMenuRequested(const QPoint &pos)
{
    pos.x();
    QMenu menu;
    auto act_modify=menu.addAction("重命名");
    menu.addSeparator();
    menu.addAction(ui->actionAdd_Chapter);
    menu.addAction(ui->actionRemove_Chapter);
    menu.addSeparator();
    menu.addAction(ui->actionAdd_volume);
    menu.addAction(ui->actionRm_volume);
    menu.addSeparator();
    menu.addAction(ui->actionRemove_Blank_Chapter);
    menu.addAction(ui->actionAuto_subchapter);
    menu.addAction(ui->action_edit_chapter);

    connect(act_modify,&QAction::triggered,[&]{
        bool ok=0;

        auto res=QInputDialog::getText(Q_NULLPTR,"重命名","改为：",QLineEdit::Normal,ui->dir_treeWidget->currentItem()->text(0),&ok);
        if(ok)
        {
            if(!ui->dir_treeWidget->currentItem())
            {
                QMessageBox::warning(Q_NULLPTR,"删除章节","选择无效！");
                return;
            }
            auto chapter_p=ui->dir_treeWidget->currentItem()->data(0,Qt::UserRole).value<Chapter*>();
            if(chapter_p==Q_NULLPTR)
            {
                ui->dir_treeWidget->currentItem()->setText(0,res);
                book.volume_names.clear();
                for(auto i=0;i<ui->dir_treeWidget->topLevelItemCount();i++)
                {
                    book.volume_names.push_back(ui->dir_treeWidget->topLevelItem(i)->text(0));
                }
            }
            else
            {
                ui->dir_treeWidget->currentItem()->setText(0,res);
                if(opened_chapters.contains(chapter_p))
                {
                    auto old_page= ui->tabWidget->currentIndex();
                    ui->tabWidget->setCurrentWidget(opened_chapters[chapter_p]);
                    ui->tabWidget->setTabText(ui->tabWidget->currentIndex(),"* "+res);
                    ui->tabWidget->setCurrentIndex(old_page);
                }
                chapter_p->name=res;
            }
        }
        menu.close();
    });

    menu.exec(QCursor::pos());
}

void MainWindow::on_actionAdd_volume_triggered()
{
    //添加章节最关键的是要找到目标位置的卷标
    int volume_index=0;
    //确定卷标
    auto current_item=ui->dir_treeWidget->currentItem();//当前项
    qDebug()<<current_item;
    if(current_item!=Q_NULLPTR)
    {
        auto tell=current_item->data(0,Qt::WhatsThisRole).toString();
        if(tell==QString("volume"))//所选项是卷名
        {
            volume_index=ui->dir_treeWidget->currentIndex().row();
            qDebug()<<"目标卷标"<<volume_index;
        }
        else                                //所选项是章名
        {
            auto selected_chapter=current_item->data(0,Qt::UserRole).value<Chapter*>();
            if(selected_chapter!=Q_NULLPTR)
            {
                volume_index=selected_chapter->vol_index;
            }
        }
    }

    QDialog w;
    w.setWindowTitle("添加新卷");
    QVBoxLayout main_layout;
    main_layout.setMargin(2);
    w.setLayout(&main_layout);

    QLabel pos_tips;
    if(book.data.size())//有卷名作为位置参考
        pos_tips.setText("在 “"+book.volume_names[volume_index]+"” 前插入");
    else                //没有章节作为位置参考
        pos_tips.setText("新建卷");
    main_layout.addWidget(&pos_tips);
    QHBoxLayout H_layout_name;
    main_layout.addLayout(&H_layout_name);

    QLabel L_name("新卷卷名");
    QLineEdit edit_name;
    QSpinBox spinbox_index;
    spinbox_index.setRange(0,volume_index+1);
    spinbox_index.setValue(volume_index);
    H_layout_name.addWidget(&L_name);
    H_layout_name.addWidget(&edit_name);
    H_layout_name.addWidget(&spinbox_index);

    QHBoxLayout H_layout_btn;
    main_layout.addLayout(&H_layout_btn);
    QPushButton ok("确认"),cancel("取消");
    H_layout_btn.addWidget(&ok);
    H_layout_btn.addWidget(&cancel);
    ok.setFocusPolicy(Qt::NoFocus);
    cancel.setFocusPolicy(Qt::NoFocus);

    connect(&ok,&QPushButton::clicked,[&]{
        if(!edit_name.text().size())return;
        //添加到书籍
        book.insert_volume(volume_index,edit_name.text());
        w.close();
       //更新目录
        if(ui->show_volume_checkBox->isChecked())
        {
            QTreeWidgetItem *new_item=new QTreeWidgetItem({edit_name.text()});
//            new_item->setBackgroundColor(0,QColor::fromRgb(168,172,158));
            new_item->setData(0,Qt::WhatsThisRole,QString("volume"));
            book.volume_top_level_item.push_back(new_item);
            ui->dir_treeWidget->insertTopLevelItem(volume_index,new_item);
        }
        else
        {
            renew_dir();
        }
        is_modefied=1;
    });
    connect(&spinbox_index,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),[&](int i){
        //确定插入位置chapter_index
        volume_index=i;
        if(i<book.data.size())
            pos_tips.setText("在 “"+book.volume_names[volume_index]+"” 前插入");
        else if(book.data.size()==0)
            pos_tips.setText("添加新卷");
        else
            pos_tips.setText("在 “"+book.volume_names.back()+"” 后插入");
    });
    connect(&cancel,&QPushButton::clicked,[&]{
        w.close();
    });

    w.exec();
}


void MainWindow::on_actionRm_volume_triggered()
{
    //添加章节最关键的是要找到目标位置的卷标
    int volume_index=0;
    //确定卷标
    auto current_item=ui->dir_treeWidget->currentItem();//当前项
    qDebug()<<current_item;
    if(current_item!=Q_NULLPTR)
    {
        auto tell=current_item->data(0,Qt::WhatsThisRole).toString();
        if(tell==QString("volume"))//所选项是卷名
        {
            volume_index=ui->dir_treeWidget->currentIndex().row();
            qDebug()<<"目标卷标"<<volume_index;
        }
        else                                //所选项是章名
        {
            auto selected_chapter=current_item->data(0,Qt::UserRole).value<Chapter*>();
            if(selected_chapter!=Q_NULLPTR)
            {
                volume_index=selected_chapter->vol_index;
            }
        }
    }

    auto ans=QMessageBox::warning(Q_NULLPTR,"删除卷","确认要删除以下卷吗？\n“"
                                  +book.volume_names[volume_index]+
                                  "”\n其中包含"+QString::number(book.data[volume_index].size())+"个章节！"
                                   ,QMessageBox::Yes|QMessageBox::No);
    if(ans!=QMessageBox::Yes)return;

    book.rm_volume(volume_index);
//    renew_dir();
    auto v=ui->dir_treeWidget->takeTopLevelItem(volume_index);
    delete v;
    is_modefied=1;
}

void MainWindow::on_show_volume_checkBox_clicked(bool checked)
{
    if(checked)
    {
        ui->actionAdd_volume->setEnabled(1);
        ui->actionRm_volume->setEnabled(1);
    }
    else
    {
        ui->actionAdd_volume->setEnabled(0);
        ui->actionRm_volume->setEnabled(0);
    }
    renew_dir();
}

void MainWindow::on_actionExport_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::question(Q_NULLPTR,"导出为pdf","所作出的更改是否保存到文件？");
        if(ans==QMessageBox::Yes)
        {
            //将编辑框内的内容写入book对应的内存
            for(auto &i:opened_chapters.keys())
                i->txt=opened_chapters[i]->toPlainText();
        }
    }
    QTextEdit edit;
    auto res=QFileDialog::getSaveFileName(Q_NULLPTR,"另存为","./","*.pdf");
    if(res.size()==0)
        return;
    QFile f(res);
    if(f.open(QIODevice::WriteOnly)==0)
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开文件失败！！！");
        return;
    }
    QPdfWriter pdf(&f);

    for(int v=0;v<book.data.size();v++)
    {
        for(auto c:book.data[v])
        {
            edit.append(c.name+"\n\n"+c.txt+"\n\n");
        }
    }
    edit.print(&pdf);
    f.close();
}

void MainWindow::writeSettings()
{
    QFile f(SETTING_FILE);
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开配置文件失败！");
        return;
    }
    QDataStream out(&f);
    out<<fmt.font();
    out<<fmt.foreground();
    out<<background_color_sheet;

    out<<book.info.name;
    qDebug()<<book.info.name;
    int vol=0,ch=0;
    if(current_page()!=Q_NULLPTR)
    {
        auto c=opened_chapters.key(current_page());
        vol=c->vol_index;
        ch=c->chapter_index;
    }
    out<<vol;
    out<<ch;
    f.close();
}

void MainWindow::readSettings()
{
    QFile f(SETTING_FILE);
    if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开配置文件失败！");
        return;
    }
    QDataStream In(&f);

    QFont font;
    QBrush brush;
    In>>font;
    In>>brush;
    fmt.setFont(font);
    fmt.setForeground(brush);
    In>>background_color_sheet;

    QString last_book;
    In>>last_book;
    qDebug()<<last_book;

    int vol=0,ch=0;
    In>>vol;
    In>>ch;

    if(last_book.size()&&!last_book.contains("undifine")&&book.info.name!=last_book)
    {
        book.open(TOP_DIR+last_book);
        if(book.info.name!=last_book)return;
        set_saved_flag();
        refresh();
        if(book.data.size()>vol&&book.data[vol].size()>ch)
        {
            auto vol_item=ui->dir_treeWidget->topLevelItem(vol);
            vol_item->setExpanded(1);
            ui->dir_treeWidget->scrollToItem(vol_item->child(ch));
        }
        f.close();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改？",QMessageBox::Yes|QMessageBox::No);
        if(ans==QMessageBox::Yes)
            event->accept();
        else
        {
            event->ignore();
        }
    }
}


void MainWindow::on_action_read_mode_triggered()
{
    auto w=current_page();
    if(w!=Q_NULLPTR)
    {
        QDialog dialog(Q_NULLPTR,Qt::FramelessWindowHint);
        dialog.setModal(Qt::ApplicationModal);

        QVBoxLayout layout;
        dialog.setLayout(&layout);
        layout.setMargin(0);

        QTextEdit reader(&dialog);
        reader.setContextMenuPolicy(Qt::CustomContextMenu);
        reader.setText(w->toPlainText());
        reader.setCurrentCharFormat(fmt);
        reader.setStyleSheet(background_color_sheet);

        auto cursor=reader.textCursor();
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(fmt);
        cursor.clearSelection();

        layout.addWidget(&reader);

        dialog.showMaximized();

        auto chapter_base=opened_chapters.key(current_page());
        auto chapter_current=chapter_base;
        if(chapter_current==Q_NULLPTR)return;

        connect(&reader,&QDialog::customContextMenuRequested,[&]
        {
//            qDebug()<<"customContextMenuRequested";
            QMenu menu;
            auto pre_chapter=menu.addAction("上一章");
            auto next_chapter=menu.addAction("下一章");
            menu.addSeparator();
            auto add_note=menu.addAction("添加笔记");
            menu.addSeparator();
            auto exit=menu.addAction("退出全屏显示");


            int v=chapter_current->vol_index;
            int c=chapter_current->chapter_index;
            if(v==0&&c==0)
                menu.removeAction(pre_chapter);
            if(v==book.data.size()-1&&c==book.data[v].size()-1)
                menu.removeAction(next_chapter);
            if(!reader.textCursor().hasSelection())
                menu.removeAction(add_note);

            auto fore_color=fmt.foreground().color();
            auto sheet="QMenu {"+background_color_sheet+";"
                               "color:rgb("
                               +QString::number(fore_color.red())+","
                               +QString::number(fore_color.green())+","
                               +QString::number(fore_color.blue())+")"+";border: 2px solid black;}"
                               "QMenu::item {"+background_color_sheet+";}"
//                               "QMenu::item:disabled{color:rgb("
//                                    +QString::number(255-fore_color.red())+","
//                                    +QString::number(255-fore_color.green())+","
//                                    +QString::number(255-fore_color.blue())+")"+";"
//                               "border: 0px solid black;"
//                               "background-color:rgb("
//                                    +QString::number((255-fore_color.red())/2)+","
//                                    +QString::number((255-fore_color.green())/2)+","
//                                    +QString::number((255-fore_color.blue())/2)+")"+";}"
                                "QMenu::item:selected {background-color: #654321;}";
//            qDebug()<<"sheet:"<<sheet;
            menu.setStyleSheet(sheet);

            connect(pre_chapter,&QAction::triggered,[&]{
                if(c==0)
                {
                    v--;
                    if(!book.data[v].size())dialog.close();//遇到空卷
                    chapter_current=&book.data[v].back();
                }
                else
                {
                    chapter_current=&book.data[v][c-1];
                }
                reader.setText(chapter_current->txt);
            });

            connect(next_chapter,&QAction::triggered,[&]{
                if(c==book.data[v].size()-1)
                {
                    v++;
                    if(!book.data[v].size())dialog.close();//遇到空卷
                    chapter_current=&book.data[v][0];
                }
                else
                {
                    chapter_current=&book.data[v][c+1];
                }
                reader.setText(chapter_current->txt);
            });
            connect(exit,&QAction::triggered,[&]{
                dialog.close();
            });
            connect(add_note,&QAction::triggered,[&]{
                auto ans=QInputDialog::getText(Q_NULLPTR,"添加笔记","索引：");
                if(ans.size()==0)return;
                ans=ans.trimmed();
                if(!key_notes.contains(ans))key_notes[ans].push_back(ans);
                key_notes[ans].append(reader.textCursor().selectedText().trimmed());
            });
            menu.exec(QCursor::pos());
        });

        dialog.exec();
    }
    else
    {
        QMessageBox::warning(Q_NULLPTR,"错误","请先打开某一章节开始阅读。");
    }
}

void MainWindow::on_file_browse_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
//    item->setHidden(1);
    if(item->text()==book.info.name)
        return;

    //从树状表中获取数据
    QString book_name=item->data(Qt::DisplayRole).toString();
    //打开书籍
    if(is_modefied)
    {
        auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续打开新文件？",QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)
        {
            ui->file_browse_listWidget->clearSelection();
            return;
        }
    }
    if(!book_name.size())
    {
        return;
    }
    book.open(TOP_DIR+book_name);
    refresh();
    is_modefied=0;
}

void MainWindow::on_file_browse_listWidget_customContextMenuRequested(const QPoint &pos)
{
    if(!ui->dir_treeWidget->currentItem())
    {
        QMessageBox::warning(Q_NULLPTR,"删除章节","选择无效！");
        return;
    }

    QMenu menu;
    auto open=menu.addAction("打开");
    auto rename=menu.addAction("重命名");
    auto remove=menu.addAction("删除该书");
    auto new_pic=menu.addAction("更新封面");
    auto new_des=menu.addAction("更新简介");
    menu.addAction(ui->actionNew);

    auto item=ui->file_browse_listWidget->currentItem();
    //从树状表中获取数据
    QString book_name=item->data(Qt::DisplayRole).toString();
    if(book.info.name==book_name)
    {
        on_actionBook_info_triggered();
        renew_file_browse_tree();
        return;
    }
    connect(open,&QAction::triggered,[&]{
        //打开书籍
        if(is_modefied)
        {
            auto ans=QMessageBox::warning(this,"警告","有未保存的更改，是否舍弃这些更改继续打开新文件？",QMessageBox::Yes|QMessageBox::No);
            if(ans!=QMessageBox::Yes)
            {
                ui->file_browse_listWidget->clearSelection();
                return;
            }
        }
        if(!book_name.size())
        {
            return;
        }
        book.open(TOP_DIR+book_name);
        refresh();
        is_modefied=0;
    });
    connect(rename,&QAction::triggered,[&]{
        auto ans=QInputDialog::getText(Q_NULLPTR,"对"+book_name+"重命名","命名为：",QLineEdit::Normal,book_name);
        ans=ans.trimmed();
        if(ans.size()==0||ans==book_name)return;
        Book b;
        b.open(TOP_DIR+book_name);
        if(!b.rename(ans))
        {
            QMessageBox::warning(Q_NULLPTR,"错误","重命名失败！");
            return;
        }
        b.save();
        renew_file_browse_tree();
    });
    connect(new_pic,&QAction::triggered,[&]{
        auto pic_dir=QFileDialog::getOpenFileName(Q_NULLPTR,"选择封面图片",PICTURE_DIR,"Images (*.png *.xpm *.jpg)");
        if(!pic_dir.size())return;
        Book b;
        b.open(TOP_DIR+book_name);
        b.info.picture=QPixmap::fromImage(QImage(pic_dir));
        b.save();
        renew_file_browse_tree();
    });
    connect(new_des,&QAction::triggered,[&]{
        Book b;
        b.open(TOP_DIR+book_name);
        auto ans=QInputDialog::getMultiLineText(Q_NULLPTR,"编辑"+book_name+"的简介","简介：",b.info.description);
        if(ans.size()==0)return;
        b.info.description=ans;
        b.save();
        renew_file_browse_tree();
    });
    connect(remove,&QAction::triggered,[&]{
        auto ans=QMessageBox::question(Q_NULLPTR,"删除"+book_name,"确认要删除 “"+book_name+"” 吗？");
        if(ans!=QMessageBox::Yes)return;
        auto file=TOP_DIR+book_name;
        if(!QDir().remove(file))
        {
            QMessageBox::warning(Q_NULLPTR,"错误","删除操作失败！");
        }
        renew_file_browse_tree();
    });
    menu.exec(QCursor::pos());
}

void MainWindow::on_action_edit_chapter_triggered()
{

}

void MainWindow::on_note_content_search_returnPressed()
{
    auto key_word=ui->note_content_search->text();
    if(!key_word.size())return;

    //获取搜索关键字长度
    int key_length=key_word.length();
    if(key_length<1)return;
    //设置标注颜色
    QTextCharFormat fmt;
    auto back_col=QColor(245,255,152);
    auto font_col=QColor(0,85,255);
    fmt.setForeground(font_col);
    fmt.setBackground(back_col);
    fmt.setFontItalic(1);
    fmt.setFontUnderline(1);

    //获取源文本
    auto src=ui->note_textEdit->toPlainText();
    //获取光标
    auto cursor=ui->note_textEdit->textCursor();
    cursor.clearSelection();
    cursor.setPosition(0);
    ui->note_textEdit->setTextCursor(cursor);
    //开始搜索
    int pos=src.indexOf(key_word);
    while(pos>=0)
    {
        //选中搜索结果
        cursor.setPosition(pos);
        cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,key_length);
        //标注
        cursor.mergeCharFormat(fmt);
        cursor.clearSelection();

        //搜索下一个
        pos=src.indexOf(key_word,pos+key_length);
    }
}

void MainWindow::on_L_tabWidget_currentChanged(int index)
{
    on_note_renew_key_list_btn_clicked();
}
