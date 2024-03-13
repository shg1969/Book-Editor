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
    completer=Q_NULLPTR;
    qDebug()<<QDir::currentPath();
    ui->setupUi(this);
    //创建必须的文件夹
    if(!QDir(TOP_DIR).exists())
        QDir().mkpath(TOP_DIR);

    //检查文件夹与文件存在与否
    QDir().mkpath(NOTE_DIR);
    if(!QFile::exists(SETTING_FILE))
    {
        QFile f(SETTING_FILE);
        f.open(QIODevice::WriteOnly);
        f.close();
    }

    if(!QFile::exists(WORD_LIST_FILE))
    {
        QFile f(WORD_LIST_FILE);
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
        note.read(NOTE_CONTENT,ui->note_key_treeWidget);
    }

    auto_set_focus=0;
    R_W_mode=0;
    text_margin=49;
    line_height=45;
//    letter_space=45;
    //载入配置信息
    readSettings();

    label_rw_mode=new QLabel(this);
    on_action_read_write_mode_triggered();
    this->ui->statusBar->addPermanentWidget(label_rw_mode);

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
        auto w=dynamic_cast<TextEdit*>(ui->tabWidget->currentWidget());
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

    renew_completer(note.getKey_structure());

    //打开上次打开的章节
    for(auto i:last_opened_chapter)
        open_chapter(book.chapter_at(i.first,i.second));

    //最大化显示
    //    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint& ~Qt::WindowMinimizeButtonHint);
    //    setWindowFlags(windowFlags()&~(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint));
    showMaximized();

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
    if(dynamic_cast<TextEdit*>(ui->tabWidget->currentWidget()))
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
    ui->statusBar->showMessage("更新目录成功！");
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
    ui->statusBar->showMessage("加载完毕！");
}

TextEdit *MainWindow::current_page()
{
    if(ui->tabWidget->currentWidget()==Q_NULLPTR)return Q_NULLPTR;
    else
    {
        return dynamic_cast<TextEdit*>(ui->tabWidget->currentWidget());
    }
}

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

void MainWindow::renew_completer(QStringList list)
{
    wordList<<list;
    auto old_completer=completer;
    completer = new QCompleter(wordList, this);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->note_key_LineEdit->setCompleter(completer);
    ui->note_search_key_lineEdit->setCompleter(completer);
    if(old_completer)old_completer->deleteLater();
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

    label_count_word->setText(COUNT_WORD_MSG(w->get_letter_number()));
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

void MainWindow::open_chapter(Chapter *chapter_p)
{
    if(chapter_p==Q_NULLPTR)return;
    if(chapter_p->get_tab_pointer()==Q_NULLPTR)
    {
        TextEdit *new_tab=new TextEdit(R_W_mode,this);
        new_tab->document()->setDocumentMargin(text_margin);
        new_tab->setAcceptRichText(0);
        chapter_p->open(new_tab);
        if(!last_opened_chapter.contains({chapter_p->vol_index,chapter_p->chapter_index}))
            last_opened_chapter.push_back({chapter_p->vol_index,chapter_p->chapter_index});

        ui->tabWidget->addTab(new_tab,chapter_p->name);
        connect(new_tab,&TextEdit::textChanged,[this]{
            is_modefied=1;
            renew_tab_name();
            renew_word_num_showing();
        });
        //自动聚焦
        connect(new_tab,&TextEdit::selectionChanged,[this]{
            auto current_edit=current_page();
            if(current_edit==Q_NULLPTR)return;
            ui->note_content_LineEdit->setText(current_edit->textCursor().selectedText());

            if(auto_set_focus==0)return;
            ui->note_key_LineEdit->setFocus();
            ui->note_key_LineEdit->selectAll();
        });
    }
    ui->tabWidget->setCurrentWidget(chapter_p->get_tab_pointer());
    ui->tabWidget->currentWidget()->setFocus();
    renew_word_num_showing();
}

void MainWindow::on_actionSave_triggered()
{
    //更新正文
    for(auto i=0;i<ui->tabWidget->count();i++)
    {
        auto tab=dynamic_cast<TextEdit*>(ui->tabWidget->widget(i));
        Chapter*c=book.find_chapter(tab);
        if(c!=Q_NULLPTR)
            c->txt=tab->toPlainText();
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
    ui->statusBar->showMessage("保存操作完成");
}

void MainWindow::on_actionSave_as_triggered()
{
    if(is_modefied)
    {
        auto ans=QMessageBox::question(Q_NULLPTR,"另存为","所作出的更改是否保存到文件？");
        if(ans==QMessageBox::Yes)
        {
            //将编辑框内的内容写入book对应的内存
            for(auto i=0;i<ui->tabWidget->count();i++)
            {
                auto tab=dynamic_cast<TextEdit*>(ui->tabWidget->widget(i));
                Chapter*c=book.find_chapter(tab);
                if(c!=Q_NULLPTR)
                    c->txt=tab->toPlainText();
            }
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
    ui->statusBar->showMessage("已关闭:"+book.info.name);
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
    Chapter* obj_chapter=book.find_chapter(dynamic_cast<TextEdit*>(ui->tabWidget->widget(index)));
    if(obj_chapter==Q_NULLPTR)return;

    if(ui->tabWidget->count()<=0)return;
    //将编辑过的内容加载到内存
    if(ui->tabWidget->tabText(index).left(2)=="* ")
    {
        auto ans=QMessageBox::question(Q_NULLPTR,"关闭章节","该章节内容经过了编辑，是否保存到内存（更未保存到文件）");
        if(ans==QMessageBox::Yes)
        {
            obj_chapter->close(1,ui->tabWidget);
            return;
        }
    }
    obj_chapter->close(0,ui->tabWidget);
    last_opened_chapter.removeOne({obj_chapter->vol_index,obj_chapter->chapter_index});
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    if(ui->tabWidget->count()<=0)return;
    //统计本章字数
    auto w=current_page();
    if(w==Q_NULLPTR)return;
    w->set_line_height(line_height);
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
        book.insert_chapter(Chapter(volume_index,chapter_index,edit_name.text(),edit_txt.toPlainText()));
        w.close();
        //更新目录
        if(ui->show_volume_checkBox->isChecked())
        {
            auto child=new QTreeWidgetItem({edit_name.text()});
            child->setData(0,Qt::UserRole,QVariant::fromValue(book.chapter_at(volume_index,chapter_index)));
            ui->dir_treeWidget->topLevelItem(volume_index)->insertChild(chapter_index,child);
        }
        else
        {
            renew_dir();
        }
        ui->statusBar->showMessage("添加章节"+edit_name.text());
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

    ui->statusBar->showMessage("添加章节"+chapter_p->name);

    //关闭章节
    chapter_p->close(0,ui->tabWidget);
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
                //关闭章节
                c.close(0,ui->tabWidget);
                //删除
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
        edit->setText(str,line_height);
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
        edit->setText(s,line_height);
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

    if(c->get_tab_pointer()==Q_NULLPTR)//对于未打开浏览的章节
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
        auto current_edit_win=c->get_tab_pointer();
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
    if(arg1)ui->note_key_LineEdit->setFocus();
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
    if(c_p->get_tab_pointer()==Q_NULLPTR)
    {
        TextEdit *new_tab=new TextEdit(1,this);
        c_p->open(new_tab);
        ui->tabWidget->addTab(new_tab,c_p->name);
        connect(new_tab,&TextEdit::undoAvailable,[this]{
            is_modefied=1;
            renew_tab_name();
            renew_word_num_showing();
        });
    }
    auto textedit=c_p->get_tab_pointer();
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

            if(c.get_tab_pointer()!=Q_NULLPTR)
            {
                c.get_tab_pointer()->setText(c.txt,line_height);
            }
        }
    }
    is_modefied=1;
    on_find_all_btn_clicked();
}

void MainWindow::pop_up_to_add_note(QString the_content_to_add_note)
{
    auto ans=QInputDialog::getText(Q_NULLPTR,"添加笔记","索引：");
    if(ans.size()==0)return;
    if(the_content_to_add_note.size()==0)return;
    ans=ans.trimmed();
    add_note(ans,the_content_to_add_note);
}

void MainWindow::add_note(QString key, QString content)
{
    key=key.trimmed();
    if(key.size()==0)return;
    if(!wordList.contains(key))
    {
        renew_completer({key});
    }
    if(note.add(key,content,ui->note_key_treeWidget))
    {
        ui->statusBar->showMessage("成功添加："+content+"-->"+key);
//        show_key_content(note.find_node(key)->getContent());
    }
    else
        ui->statusBar->showMessage("空白无效，添加失败！");
}

void MainWindow::show_key_content(const QStringList &content)
{
    //页面设置
    ui->note_textEdit->clear();
    ui->note_select_row_spinBox->setRange(0,content.size()-1);
    ui->note_textEdit->setFontPointSize(ui->note_content_showing_font_size->value());
    //获取内容
    QString s;
    int count=0;
    for(auto &i:content)
    {
        s+="\n------"+QString::number(count)+"------\n"+i;
        count++;
    }
    //开始显示
    ui->note_textEdit->setText(s);
    ui->show_nodes_dockWidget->show();
}

void MainWindow::on_note_key_LineEdit_returnPressed()
{
    auto key=ui->note_key_LineEdit->text();
    auto content=ui->note_content_LineEdit->text();

    add_note(key,content);
    is_modefied=1;
}

void MainWindow::on_note_save_btn_clicked()
{
    note.save(NOTE_CONTENT);
}

void MainWindow::on_node_rm_row_btn_clicked()
{
    //修改
    if(!the_Item_showing)return;
    int obj=ui->note_select_row_spinBox->value();
    the_Item_showing->remove_at(obj);
    //刷新显示
    show_key_content(the_Item_showing->getContent());
    is_modefied=1;
}

void MainWindow::on_node_edit_note_btn_clicked()
{
    int obj_index=ui->note_select_row_spinBox->value();

    QString msg="编辑“"+the_Item_showing->get_key_name()+"”中的第"+QString::number(obj_index)+"项";
    QString old_str=the_Item_showing->get_piece_at(obj_index);

    QDialog dialog;
    dialog.setWindowTitle(msg);
    dialog.resize(600,800);
    QTextEdit edit(old_str.trimmed());
    edit.selectAll();
    QVBoxLayout layout(&dialog);
    layout.setMargin(0);
    layout.addWidget(&edit);
    QPushButton ok("确定"),cancel("取消");
    QHBoxLayout btn_layout;
    btn_layout.setMargin(0);
    btn_layout.addWidget(&ok);
    btn_layout.addWidget(&cancel);
    layout.addLayout(&btn_layout);
    connect(&ok,&QPushButton::clicked,[&]{
        auto ans=edit.toPlainText();
        if(ans.size()==0)return;
        //修改
        the_Item_showing->set_piece_at(obj_index,ans.trimmed());

        //刷新显示
        show_key_content(the_Item_showing->getContent());
        auto res=ui->note_textEdit->find("------"+QString::number(obj_index)+"------");
        if(res)ui->note_textEdit->textCursor().clearSelection();

        is_modefied=1;
        dialog.close();
    });
    connect(&cancel,&QPushButton::clicked,[&]{
        dialog.close();
    });
    dialog.exec();
}

void MainWindow::on_note_select_row_spinBox_valueChanged(int arg1)
{
    int max=the_Item_showing->count_piece()-1;
    if(arg1>=max)
        ui->note_select_row_spinBox->setValue(max);
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
        edit->setText(s,line_height);
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
    open_chapter(chapter_p);
}

//章节目录处右击
void MainWindow::on_dir_treeWidget_customContextMenuRequested(const QPoint &pos)
{
    pos.x();
    QMenu menu;
    auto act_modify=menu.addAction("重命名");
    menu.addAction(ui->action_add_comment);
    menu.addAction(ui->actionadd_imitating);
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

    //重命名
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
                if(chapter_p->get_tab_pointer()!=Q_NULLPTR)
                {
                    auto old_page= ui->tabWidget->currentIndex();
                    ui->tabWidget->setCurrentWidget(chapter_p->get_tab_pointer());
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
        ui->statusBar->showMessage("添加卷："+edit_name.text());
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

    ui->statusBar->showMessage("删除卷："+book.volume_names[volume_index]);
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
            for(auto &v:book.data)//每一卷
            {
                for(auto &c:v)
                {
                    if(c.get_tab_pointer()!=Q_NULLPTR)
                        c.txt=c.get_tab_pointer()->toPlainText();
                }
            }
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
        auto c=book.find_chapter(current_page());
        if(c!=Q_NULLPTR)
        {
            vol=c->vol_index;
            ch=c->chapter_index;
        }
        else
        {
            vol=0;
            ch=0;
        }
    }
    out<<vol;
    out<<ch;
    out<<this->text_margin;
    out<<this->line_height;
    out<<this->R_W_mode;
    out<<auto_set_focus;

    out<<last_opened_chapter.size();
    for(auto i:last_opened_chapter)
    {
        out<<i.first;
        out<<i.second;
    }
    f.close();
}

void MainWindow::readSettings()
{
    QFile f(SETTING_FILE);
    if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开配置文件失败！");
        text_margin=40;
        line_height=45;
        R_W_mode=1;
        auto_set_focus=1;
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
    In>>this->text_margin;
    if(text_margin<1)text_margin=40;
    In>>this->line_height;
    if(line_height<10)line_height=45;
    In>>this->R_W_mode;
    In>>auto_set_focus;

    int size=0;
    In>>size;
    for(int i=0;i<size;i++)
    {
        int temp_vol=0;
        int temp_ch=0;
        In>>temp_vol;
        In>>temp_ch;
        if(!last_opened_chapter.contains({temp_vol,temp_ch}))last_opened_chapter.push_back({temp_vol,temp_ch});
    }

    /*********************************读取完毕，完成相关设置*****************************************/
    //设置是否打开自动聚焦
    if(auto_set_focus)ui->auto_focus_checkBox->setChecked(auto_set_focus);
    //打开上次打开的书籍、章节
    if(last_book.size()&&!last_book.contains("undifine")&&book.info.name!=last_book)
    {
        book.open(TOP_DIR+last_book);
        if(book.info.name!=last_book)return;
        set_saved_flag();
        refresh();
        //展开上次展开的卷目录
        if(book.data.size()>vol&&book.data[vol].size()>ch)
        {
            auto vol_item=ui->dir_treeWidget->topLevelItem(vol);
            vol_item->setExpanded(1);
            ui->dir_treeWidget->scrollToItem(vol_item->child(ch));
        }
    }

    f.close();

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
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

        TextEdit reader(1,&dialog);

        reader.setContextMenuPolicy(Qt::CustomContextMenu);
        reader.setText(w->toPlainText(),line_height);

        reader.document()->setDocumentMargin(text_margin);
        reader.setReadOnly(R_W_mode);

        reader.setCurrentCharFormat(fmt);
        reader.setStyleSheet(background_color_sheet);

        auto cursor=reader.textCursor();
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(fmt);
        cursor.clearSelection();

        layout.addWidget(&reader);

        dialog.showMaximized();

        auto chapter_current=book.find_chapter(current_page());
        if(chapter_current==Q_NULLPTR)return;

        connect(&reader,&QDialog::customContextMenuRequested,[&]
        {
//            qDebug()<<"customContextMenuRequested";
            QMenu menu;
            auto pre_chapter=menu.addAction("上一章");
//            pre_chapter->setShortcut(Qt::LeftButton);
            
            auto next_chapter=menu.addAction("下一章");
//            pre_chapter->setShortcut(Qt::RightButton);
            menu.addSeparator();
            auto act_add_note=menu.addAction("添加笔记");
            menu.addSeparator();
            auto exit=menu.addAction("退出全屏显示");

            int v=chapter_current->vol_index;
            int c=chapter_current->chapter_index;
            if(v==0&&c==0)
                menu.removeAction(pre_chapter);
            if(v==book.data.size()-1&&c==book.data[v].size()-1)
                menu.removeAction(next_chapter);
            if(!reader.textCursor().hasSelection())
                menu.removeAction(act_add_note);

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
                reader.setText(chapter_current->txt,line_height);
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
                reader.setText(chapter_current->txt,line_height);
            });
            connect(exit,&QAction::triggered,[&]{
                dialog.close();
            });
            connect(act_add_note,&QAction::triggered,[&]{
                pop_up_to_add_note(reader.textCursor().selectedText().trimmed());
            });
            menu.exec(QCursor::pos());
        });

        hide();//打开阅读窗口后关闭主窗口
        dialog.exec();
        //在非全屏模式下打开结束全屏模式前阅读的最后一章
        auto cur_w=current_page();
        if(book.find_chapter(cur_w)!=chapter_current)
            open_chapter(chapter_current);

        show();//重新显示主窗口
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
    Q_UNUSED(pos);

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

        QDialog dialog;
        dialog.setWindowTitle("编辑"+book_name+"的简介");
        dialog.resize(600,800);
        QTextEdit edit(b.info.description.trimmed());
        edit.selectAll();
        QVBoxLayout layout(&dialog);
        layout.setMargin(0);
        layout.addWidget(&edit);
        QPushButton ok("确定"),cancel("取消");
        QHBoxLayout btn_layout;
        btn_layout.setMargin(0);
        btn_layout.addWidget(&ok);
        btn_layout.addWidget(&cancel);
        layout.addLayout(&btn_layout);
        connect(&ok,&QPushButton::clicked,[&]{
            auto ans=edit.toPlainText();
            if(ans.size()==0)return;
            b.info.description=ans;
            b.save();
            renew_file_browse_tree();
            dialog.close();
        });
        connect(&cancel,&QPushButton::clicked,[&]{
            dialog.close();
        });
        dialog.exec();
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

void MainWindow::on_note_content_showing_font_size_valueChanged(int arg1)
{
    auto cursor=ui->note_textEdit->textCursor();
    cursor.select(QTextCursor::Document);
    auto fmt=cursor.charFormat();
    fmt.setFontPointSize(arg1);
    cursor.setCharFormat(fmt);
    cursor.clearSelection();
    ui->note_textEdit->setTextCursor(cursor);
}

//右键菜单显示
void MainWindow::on_note_key_treeWidget_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    auto item=ui->note_key_treeWidget->currentItem();
    if(!item)
    {
        QMessageBox::warning(Q_NULLPTR,"编辑项目","选择无效！");
        return;
    }

    QMenu menu;
    auto open=menu.addAction("打开");
    auto rename=menu.addAction("重命名");
    auto remove=menu.addAction("删除该项");

    bool is_key=1;
    if(item->childCount())
    {
        rename->setEnabled(0);
        open->setEnabled(0);
        is_key=0;
    }

    //从树状表中获取数据
    auto obj_node=item->data(0,Qt::UserRole).value<Note_Item*>();
    if(!obj_node)return;
    //更新当前笔记项
    ui->show_nodes_dockWidget->hide();
    the_Item_showing=obj_node;
    //打开
    connect(open,&QAction::triggered,[&]{
        show_key_content(obj_node->getContent());
    });
    //重命名
    connect(rename,&QAction::triggered,[&]{
        auto ans=QInputDialog::getText(Q_NULLPTR,"对"+obj_node->getKey_structure()+"重命名","命名为：",QLineEdit::Normal,obj_node->getKey_structure());
        ans=ans.trimmed();
        if(ans.size()==0||ans==obj_node->getKey_structure())return;
        note.rename(obj_node->getKey_structure(),ans,ui->note_key_treeWidget);
        renew_completer({ans});
        is_modefied=1;
    });
    //删除
    connect(remove,&QAction::triggered,[&]{
        auto ans=QMessageBox::question(Q_NULLPTR,"删除","这将会删除整个索引以及所有相关笔记，确认码？\n"+obj_node->getKey_structure(),
                              QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return ;
        note.remove_one(*obj_node);
        is_modefied=1;
    });
    menu.exec(QCursor::pos());
}

void MainWindow::on_note_search_key_lineEdit_returnPressed()
{
    auto obj_key=ui->note_search_key_lineEdit->text();
    if(obj_key.size()==0)return;

    //先刷新列表再搜索
    ui->note_key_listWidget->clear();
    QStringList a;

    for(auto i:note.getKey_structure())
    {
        if(i.contains(obj_key))
            a.append(i);
    }
    a.sort(Qt::CaseInsensitive);
    for(auto i:a)
    {
        if(i.size()==0)continue;
        auto item= new QListWidgetItem(i);
        item->setData(Qt::UserRole,QVariant::fromValue(note.find_node(i)));
        ui->note_key_listWidget->addItem(item);
    }
    //如果只有一个结果，直接打开
    if(a.size()==1)
        show_key_content(note.find_node(a.value(0))->getContent());
}

void MainWindow::on_action_read_write_mode_triggered()
{
    R_W_mode=!R_W_mode;
    if(R_W_mode)//阅读模式
    {
        label_rw_mode->setText(" Read-Mode ");
//        label_rw_mode->setStyleSheet("background:rgb(220,180,180)");
    }
    else//写作模式下，不能自动聚焦到添加笔记的key输入框
    {
        ui->auto_focus_checkBox->setChecked(0);
        ui->statusBar->showMessage("写作模式下，自动关闭自动聚焦");
        label_rw_mode->setText(" Write-Mode ");
//        label_rw_mode->setStyleSheet("background:rgb(180,220,180)");
    }
    auto w=current_page();
    if(w!=Q_NULLPTR)
            w->setReadOnly(R_W_mode);
}

void MainWindow::on_action_Margin_triggered()
{
    int temp_margin=4;
    auto current_w=current_page();
    if(current_w!=Q_NULLPTR)
        text_margin=current_w->document()->documentMargin();

    QDialog w;
    w.resize(450,100);
    QVBoxLayout layout(&w);
    w.setWindowTitle("设置边距");
    QSpinBox box;
    layout.addWidget(&box);

    if(current_w!=Q_NULLPTR)
        box.setRange(1,current_w->document()->textWidth()/2);
    else
        box.setRange(1,50);
    if(text_margin>1)
        box.setValue(text_margin);

    QHBoxLayout btn(&w);
    layout.addLayout(&btn);
    QPushButton ok;
    btn.addWidget(&ok);
    ok.setText("确认");
    QPushButton cancel;
    btn.addWidget(&cancel);
    cancel.setText("取消");

    connect(&box,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),[&temp_margin,current_w](int i){
        if(current_w!=Q_NULLPTR)
            current_w->document()->setDocumentMargin(i);
        temp_margin=i;
    });

    connect(&cancel,&QPushButton::clicked,[current_w,this,&w]{
        current_w->document()->setDocumentMargin(text_margin);
        w.close();
    });
    connect(&ok,&QPushButton::clicked,[&temp_margin,current_w,this,&w]{
        this->text_margin=temp_margin;
        current_w->document()->setDocumentMargin(text_margin);
        w.close();
    });
    w.exec();
}

void MainWindow::on_action_line_high_triggered()
{
    auto current_w=current_page();

    QDialog w;
    w.resize(450,100);
    QVBoxLayout layout(&w);
    w.setWindowTitle("设置行距");
    QSpinBox box;
    layout.addWidget(&box);
    box.setRange(1,100);
    box.setValue(line_height);

    QHBoxLayout btn(&w);
    layout.addLayout(&btn);
    QPushButton ok;
    btn.addWidget(&ok);
    ok.setText("确认");
    QPushButton cancel;
    btn.addWidget(&cancel);
    cancel.setText("取消");

    connect(&box,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),[current_w](int i){
        if(current_w!=Q_NULLPTR)
            current_w->set_line_height(i);
    });

    connect(&cancel,&QPushButton::clicked,[current_w,this,&w]{
        if(current_w!=Q_NULLPTR)
            current_w->set_line_height(line_height);
        w.close();
    });
    connect(&ok,&QPushButton::clicked,[&box,current_w,this,&w]{
        this->line_height=box.value();
        if(current_w!=Q_NULLPTR)
            current_w->set_line_height(line_height);
        w.close();
    });
    w.exec();
}


//void MainWindow::on_action_letter_space_triggered()
//{
//    auto current_w=current_page();
//    QDialog w;
//    w.resize(450,100);
//    QVBoxLayout layout(&w);
//    w.setWindowTitle("设置字距");
//    QSpinBox box;
//    layout.addWidget(&box);
//    box.setRange(1,200);
//    box.setValue(100);

//    QHBoxLayout btn(&w);
//    layout.addLayout(&btn);
//    QPushButton ok;
//    btn.addWidget(&ok);
//    ok.setText("确认");
//    QPushButton cancel;
//    btn.addWidget(&cancel);
//    cancel.setText("取消");

//    connect(&box,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),[current_w](int i){
//        if(current_w!=Q_NULLPTR)
//            current_w->set_letter_space(i);
//    });

//    connect(&cancel,&QPushButton::clicked,[current_w,&w]{
//        if(current_w!=Q_NULLPTR)
//            current_w->set_letter_space(100);
//        w.close();
//    });
//    connect(&ok,&QPushButton::clicked,[&box,current_w,this,&w]{
//        this->letter_space=box.value();
//        if(current_w!=Q_NULLPTR)
//            current_w->set_letter_space(letter_space);
//        w.close();
//    });
//    w.exec();
//}


void MainWindow::on_dir_tabWidget_tabBarClicked(int index)
{
    if(index==1)
    {
        qDebug()<<"查看笔记页";
        ui->chapter_note_listWidget->clear();
        for(auto &v:book.data)
            for(auto &c:v)
                if(c.comment.size())
                {
                    auto new_item=new QListWidgetItem(c.name,ui->chapter_note_listWidget);
                    new_item->setData(Qt::UserRole,QVariant::fromValue(&c));
                }
    }
    else if(index==2)
    {
        qDebug()<<"查看仿写页";
        ui->chapter_imitating_listWidget->clear();
        for(auto &v:book.data)
            for(auto &c:v)
                if(c.imitating.size())
                {
                    auto new_item=new QListWidgetItem(c.name,ui->chapter_imitating_listWidget);
                    new_item->setData(Qt::UserRole,QVariant::fromValue(&c));
                }
    }
}

void MainWindow::open_chapter_comment(Chapter *chapter_p)
{
    if(chapter_p==Q_NULLPTR)
    {
        QMessageBox::warning(Q_NULLPTR,"错误","选择无效章节");
        return;
    }
    QDockWidget *w=chapter_p->getDock_add_comment();

    if(w!=Q_NULLPTR)
    {
        w->show();
        return ;
    }
    else
        w=new QDockWidget("章节笔记@"+chapter_p->name,this);
    w->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea,w);
    w->resize(1000,800);

    QWidget *group_widget=new QWidget(w);
    w->setWidget(group_widget);
    auto main_layout=new QVBoxLayout(group_widget);
    main_layout->setMargin(0);
    auto edit=new QTextEdit(w);
    edit->setFontPointSize(16);
    edit->setFocus();
    edit->setText(chapter_p->comment);
//    edit->textCursor().movePosition()
    main_layout->addWidget(edit);

    auto btn_layout=new QHBoxLayout;
    btn_layout->setMargin(2);
    main_layout->addLayout(btn_layout);

    QPushButton *ok=new QPushButton("确认",w);
    QPushButton *cancel=new QPushButton("取消",w);
    btn_layout->addWidget(ok);
    btn_layout->addWidget(cancel);

    connect(ok,&QPushButton::clicked,[chapter_p,edit,w]{
        chapter_p->comment=edit->toPlainText();
        w->close();
        w->deleteLater();
    });
    connect(cancel,&QPushButton::clicked,[w]{
        w->close();
        w->deleteLater();
    });
    connect(btn_layout,&QHBoxLayout::destroyed,[]{qDebug()<<"btn_layout is distoryed.";});
}

void MainWindow::open_chapter_imitating(Chapter*chapter_p)
{
    if(chapter_p==Q_NULLPTR)
    {
        QMessageBox::warning(Q_NULLPTR,"错误","选择无效章节");
        return;
    }
    QDockWidget *w=chapter_p->getAdd_imitating();

    if(w!=Q_NULLPTR)
    {
        w->show();
        return ;
    }
    else
        w=new QDockWidget("仿写@"+chapter_p->name,this);
    w->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea,w);
    w->resize(1000,800);

    QWidget *group_widget=new QWidget(w);
    w->setWidget(group_widget);
    auto main_layout=new QVBoxLayout(group_widget);
    main_layout->setMargin(0);
    auto edit=new QTextEdit(w);
    edit->setFontPointSize(16);
    edit->setFocus();
    edit->setText(chapter_p->imitating);
    edit->setFocus();
    main_layout->addWidget(edit);

    auto btn_layout=new QHBoxLayout;
    btn_layout->setMargin(2);
    main_layout->addLayout(btn_layout);

    QPushButton *ok=new QPushButton("确认",w);
    QPushButton *cancel=new QPushButton("取消",w);
    btn_layout->addWidget(ok);
    btn_layout->addWidget(cancel);

    connect(ok,&QPushButton::clicked,[chapter_p,edit,w]{
        chapter_p->imitating=edit->toPlainText();
        w->close();
        w->deleteLater();
    });
    connect(cancel,&QPushButton::clicked,[w]{
        w->close();
        w->deleteLater();
    });
    connect(btn_layout,&QHBoxLayout::destroyed,[]{qDebug()<<"btn_layout is distoryed.";});
}
void MainWindow::on_action_add_comment_triggered()
{
    auto item=ui->dir_treeWidget->currentItem();
    if(item==Q_NULLPTR)return;
    auto chapter_p=item->data(0,Qt::UserRole).value<Chapter*>();
    open_chapter_comment(chapter_p);
}

void MainWindow::on_actionadd_imitating_triggered()
{
    auto item=ui->dir_treeWidget->currentItem();
    if(item==Q_NULLPTR)return;
    auto chapter_p=item->data(0,Qt::UserRole).value<Chapter*>();
    open_chapter_imitating(chapter_p);
}

void MainWindow::on_chapter_note_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    auto chapter_p=item->data(Qt::UserRole).value<Chapter*>();
    open_chapter_comment(chapter_p);
}

void MainWindow::on_chapter_imitating_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    auto chapter_p=item->data(Qt::UserRole).value<Chapter*>();
    open_chapter_imitating(chapter_p);
}

void MainWindow::on_actionget_chapter_notes_triggered()
{
    QString res,temp;

    for(auto v:book.data)
        for(auto c:v)
        {
            if(c.comment.size())
            {
                temp.resize(100);
                res+='\n'+temp.fill('-',(50-c.name.size())/2)
                        +c.name
                        +temp.fill('-',50-temp.size()-c.name.size())
                        +"\n\n"
                        +c.comment+"\n\n"  ;
            }
        }

    if(res.size()==0)
    {
        QMessageBox::warning(Q_NULLPTR,"警告","无相关记录");
        return;
    }
    auto file_name=QFileDialog::getSaveFileName(Q_NULLPTR,"导出笔记","../../","*.txt");
    if(file_name.size()==0)return;
    QFile f(file_name);
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开文件失败");
        return;
    }
    QTextStream Out(&f);
    Out<<res;
    f.close();
}

void MainWindow::on_actionget_chapter_imitating_triggered()
{
    QString res,temp;
    for(auto v:book.data)
        for(auto c:v)
        {
            if(c.imitating.size())
            {
                temp.resize(100);
                res+='\n'+temp.fill('-',(50-c.name.size())/2)
                        +c.name
                        +temp.fill('-',50-temp.size()-c.name.size())
                        +"\n\n"
                        +c.imitating+"\n\n"  ;
            }
        }
    if(res.size()==0)
    {
        QMessageBox::warning(Q_NULLPTR,"警告","无相关记录");
        return;
    }
    auto file_name=QFileDialog::getSaveFileName(Q_NULLPTR,"导出仿写记录","../../","*.txt");
    if(file_name.size()==0)return;
    QFile f(file_name);
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"错误","打开文件失败");
        return;
    }
    QTextStream Out(&f);
    Out<<res;
    f.close();
}

void MainWindow::on_note_key_listWidget_itemClicked(QListWidgetItem *item)
{
    ui->note_key_LineEdit->setText(item->text());
    ui->note_key_LineEdit->setFocus();
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    Q_UNUSED(index);
    QMenu menu;

    menu.addAction(ui->action_add_comment);
    menu.addAction(ui->actionadd_imitating);

    menu.exec(QCursor::pos());
}

void MainWindow::on_note_key_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    //从表中获取数据
    the_Item_showing=item->data(0,Qt::UserRole).value<Note_Item*>();
    if(the_Item_showing)
        show_key_content(the_Item_showing->getContent());
}

void MainWindow::on_note_key_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    //从表中获取数据
    the_Item_showing=item->data(Qt::UserRole).value<Note_Item*>();
    if(the_Item_showing)
        show_key_content(the_Item_showing->getContent());
}

void MainWindow::on_note_key_listWidget_customContextMenuRequested(const QPoint &pos)
{
    //检验
    Q_UNUSED(pos);
    auto item=ui->note_key_listWidget->currentItem();
    if(!item)
    {
        QMessageBox::warning(Q_NULLPTR,"编辑项目","选择无效！");
        return;
    }

    QMenu menu;
    auto open=menu.addAction("打开");
    auto rename=menu.addAction("重命名");
    auto remove=menu.addAction("删除该项");

    //从树状表中获取数据
    auto obj_node=item->data(Qt::UserRole).value<Note_Item*>();
    if(!obj_node)return;
    //更新当前笔记项
    ui->show_nodes_dockWidget->hide();
    the_Item_showing=obj_node;
    //打开
    connect(open,&QAction::triggered,[&]{
        show_key_content(obj_node->getContent());
    });
    //重命名
    connect(rename,&QAction::triggered,[&]{
        //获取新名
        auto ans=QInputDialog::getText(Q_NULLPTR,"对"+obj_node->getKey_structure()+"重命名","命名为：",QLineEdit::Normal,obj_node->getKey_structure());
        ans=ans.trimmed();
        if(ans.size()==0||ans==obj_node->getKey_structure())return;

        note.rename(obj_node->getKey_structure(),ans,ui->note_key_treeWidget);
        item->setText(ans);
        renew_completer({ans});
        is_modefied=1;
    });
    //删除
    connect(remove,&QAction::triggered,[&]{
        auto ans=QMessageBox::question(Q_NULLPTR,"删除","这将会删除整个索引以及所有相关笔记，确认码？\n"+obj_node->getKey_structure(),
                              QMessageBox::Yes|QMessageBox::No);
        if(ans!=QMessageBox::Yes)return ;
        note.remove_one(*obj_node);
        delete item;
        is_modefied=1;
    });
    menu.exec(QCursor::pos());
}


void MainWindow::on_note_key_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    auto node=item->data(0,Qt::UserRole).value<Note_Item*>();
    if(!node)return;
    ui->note_key_LineEdit->setText(node->getKey_structure());
    ui->note_key_LineEdit->setFocus();
}

void MainWindow::on_action_next_chapter_triggered()
{
    //获取窗口
    auto current_tab=current_page();
    if(current_tab==Q_NULLPTR)return;

    //获取之前显示的一章
    auto last_chapter=book.find_chapter(current_tab);
    if(last_chapter==Q_NULLPTR)return;
    //关闭
    else on_tabWidget_tabCloseRequested(ui->tabWidget->currentIndex());

    Chapter* next_chapter=Q_NULLPTR;
    int c=last_chapter->chapter_index;
    int v=last_chapter->vol_index;
    if(c==book.data[v].size()-1)
    {
        v++;
        if(v>=book.data.size()||!book.data[v].size())return;//遇到空卷
        next_chapter=&book.data[v][0];
    }
    else
    {
        next_chapter=&book.data[v][c+1];
    }
    //打开
    if(next_chapter)open_chapter(next_chapter);
}

void MainWindow::on_action_pre_chapter_triggered()
{
    //获取窗口
    auto current_tab=current_page();
    if(current_tab==Q_NULLPTR)return;

    //获取之前显示的一章
    auto last_chapter=book.find_chapter(current_tab);
    if(last_chapter==Q_NULLPTR)return;
    //关闭
    else on_tabWidget_tabCloseRequested(ui->tabWidget->currentIndex());

    Chapter* next_chapter=Q_NULLPTR;
    int c=last_chapter->chapter_index;
    int v=last_chapter->vol_index;
    if(c==0)
    {
        v--;
        if(v<0||!book.data[v].size())return;//遇到空卷
        next_chapter=&book.data[v].back();
    }
    else
    {
        next_chapter=&book.data[v][c-1];
    }
    if(next_chapter)open_chapter(next_chapter);
}
