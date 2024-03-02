#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "inc/book.h"
#include<QIcon>
#include<QDir>
#include<QScrollBar>
#include<QFont>
#include<QColor>
#include<QLabel>
#include<QPair>
#include<QHash>
#include<QVector>
#include<QSettings>
#include"inc/textedit.h"
#include<QPdfWriter>
#include<QToolBox>
#include<QDialog>
#include<QInputDialog>
#include<QToolBar>
#include<QTextStream>
#include<QMessageBox>
#include<QListWidget>
#include <QCompleter>
#include<QFileDialog>
#include<QColorDialog>
#include <QMainWindow>
#include<QFontComboBox>
#include<QTextCharFormat>
#include<QListWidgetItem>
#include<QFileSystemModel>
#include <QTreeWidgetItem>
#include <QStringListModel>

#define APP_NAME QString("读写助手")
#define COUNT_WORD_MSG(num) (QString("本章字数：%1").arg(num))

#define NOTE_DIR        (QString("./data/note/"))
#define NOTE_CONTENT    (QString("./data/note/content.txt"))
#define SETTING_FILE    (QString("./data/setting.txt"))
#define WORD_LIST_FILE    (QString("./data/wordlist.txt"))

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

Q_DECLARE_METATYPE(QString);
Q_DECLARE_METATYPE(QTextCharFormat);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void theme_changed(bool mode);
public slots:
    void on_actionDark_Mode_triggered(bool checked);    //夜间模式
private:
    Ui::MainWindow *ui;
    Book book;
    QLabel *label_count_word;
    QLabel *label_rw_mode;
    bool is_modefied;
    bool auto_set_focus;
    QVector<QPair<int,int>> last_opened_chapter;//最后展开的卷、章节号
    QString background_color_sheet;         //背景颜色
    QTextCharFormat fmt;                    //字体格式
    int search_option;                      //0:对当前章节正文，1：对全书章节名，2：对全书正文
    QHash<QString,QStringList> key_notes;   //每个QStringList为一个key本身和与key对应的笔记的集合
    QString the_key_showing;                //当前右侧窗口显示的笔记所属的key
    bool R_W_mode;                          //读写模式
    int text_margin;                        //文本编辑框内的文本边距
    int line_height;                          //文本编辑框内的文本行距
//    int letter_space;                       //文本编辑框内的文本字距

    //获取当前编辑页面
    TextEdit *current_page();
    //在某一章内检索
    void search_in_textedit(Chapter *c, QString key_word);
    //显示检索结果
    void show_search_result();
    //更新保存状态标志
    void set_saved_flag();
    //更新字体格式
    void renew_viewport_format();
    void renew_viewport_format(int size);
    void renew_viewport_format(QColor font_color);
    void renew_viewport_format(const QString &font_family);
    //刷新主窗口标题
    void renew_window_title();
    //读写配置信息
    void writeSettings();
    void readSettings();
    //窗口关闭事件
    void closeEvent(QCloseEvent *event);

    void open_chapter_comment(Chapter *chapter_p);
    void open_chapter_imitating(Chapter *chapter_p);
private slots:
    //自定义
    void refresh();                     //整个软件的显示全部刷新一遍
    void renew_dir(void);               //以book为根据，刷新目录的显示
    void renew_word_num_showing();      //刷新当前页面的字数显示
    void renew_tab_name(void);;         //刷新已打开tab页面的名称
    void renew_file_browse_tree();      //刷新书架内容

    void open_chapter(Chapter *chapter_p);
    //书籍
    void on_actionOpen_triggered();     //打开
    void on_actionNew_triggered();      //新建

    void on_actionSave_triggered();     //保存
    void on_actionSave_as_triggered();  //另存为

    void on_actionClose_triggered();    //关闭
    void on_actionRemove_triggered();   //删除

    void on_actionLoad_triggered();     //加载
    //编辑
    void on_actionAdd_volume_triggered();       //添加卷
    void on_actionRm_volume_triggered();        //删除卷
    void on_actionAdd_Chapter_triggered();      //添加章节
    void on_actionRemove_Chapter_triggered();   //删除章节

    void on_actionExport_triggered();           //导出pdf
    //关于
    void on_actionAuthor_triggered();       //关于作者
    void on_actionQt_triggered();           //关于Qt
    void on_actionBook_info_triggered();    //书籍信息
    //功能
    void on_actionAdd_Blank_Lines_triggered();          //去除空行
    void on_actionRemove_Space_triggered();             //去除所有空格
    void on_actionRemove_Blank_Chapter_triggered();     //去除空文章节
    void on_actionDelete_bank_row_triggered();          //去除空行
    void on_actionAuto_subchapter_triggered();          //自动分章
    void on_action_read_mode_triggered();               //全屏阅读

    //edit_Tab
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_currentChanged(int index);

    //书架
    void on_file_browse_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_file_browse_listWidget_customContextMenuRequested(const QPoint &pos);

    //目录
    void on_show_volume_checkBox_clicked(bool checked); //显示模式（分卷与否）
    void on_dir_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_dir_treeWidget_customContextMenuRequested(const QPoint &pos);
    void on_action_edit_chapter_triggered();
    //搜索
    void on_search_option_comboBox_currentTextChanged(const QString &arg1);
    void on_find_all_btn_clicked();
    void on_search_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_find_next_btn_clicked();
    void on_replace_current_btn_clicked();
    void on_replace_all_btn_clicked();
    //摘录
    void export_note_to_list_file();
    void pop_up_to_add_note(QString the_content_to_add_note);
    void add_note(QString key,QString content);
    void show_key_content(QString key);
    void on_auto_focus_checkBox_stateChanged(int arg1);//自动聚焦到key编辑框
    void on_note_key_LineEdit_returnPressed();
    void on_note_save_btn_clicked();
    void on_note_renew_key_list_btn_clicked();
    void on_node_rm_row_btn_clicked();
    void on_node_edit_note_btn_clicked();
    void on_note_select_row_spinBox_valueChanged(int arg1);
    void on_note_content_LineEdit_returnPressed();
    void on_note_content_search_returnPressed();
    void on_note_content_showing_font_size_valueChanged(int arg1);
    void on_note_key_listWidget_customContextMenuRequested(const QPoint &pos);
    void on_note_key_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_note_search_key_lineEdit_returnPressed();
    void on_action_read_write_mode_triggered();
    void on_action_Margin_triggered();
    void on_action_line_high_triggered();
//    void on_action_letter_space_triggered();
    void on_dir_tabWidget_tabBarClicked(int index);
    void on_action_add_comment_triggered();
    void on_actionadd_imitating_triggered();
    void on_chapter_note_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_chapter_imitating_listWidget_itemDoubleClicked(QListWidgetItem *item);
};
#endif // MAINWINDOW_H
