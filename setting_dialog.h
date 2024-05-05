#ifndef SETTING_DIALOG_H
#define SETTING_DIALOG_H

#include <QDialog>
#include <QTextCharFormat>

namespace Ui {
class Setting_Dialog;
}

typedef struct _Setting_Data
{
    QString background_color_sheet;         //背景颜色
    QTextCharFormat fmt;                    //字体格式

    bool auto_set_focus;
    bool dark_mode;                         //夜间模式
    bool R_W_mode;                          //读写模式
    int text_margin;                        //文本编辑框内的文本边距
    int line_height;                        //文本编辑框内的文本行距

    bool auto_save_book;
    bool auto_save_notes;
    bool auto_save_book_frequency;
    bool auto_save_notes_frequency;
    QColor background_color;

    int time_auto_save_book_min=10;
    int time_auto_save_notes_min=5;
}Setting_Data;


class Setting_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Setting_Dialog(Setting_Data &data,QWidget *parent = nullptr);
    ~Setting_Dialog();

    void static renew(Setting_Data &data)
    {
//        Setting_Dialog w;
//        connect()
//        w.exec();
    }

private:
    Ui::Setting_Dialog *ui;
};

#endif // SETTING_DIALOG_H
