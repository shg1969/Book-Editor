#include "setting_dialog.h"
#include "ui_setting_dialog.h"

Setting_Dialog::Setting_Dialog(Setting_Data &data,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Setting_Dialog)
{
    ui->setupUi(this);

    //阅读界面
    //日-夜间模式
    if(data.dark_mode)
        ui->day_mode->setChecked(1);
    else
        ui->night_mode->setChecked(1);
    //读写模式
    if(data.R_W_mode)
        ui->reading_mode->setChecked(1);
    else
        ui->editing_mode->setChecked(1);
    //边距
    ui->margin->setValue(data.text_margin);
    //行距
    ui->line_height->setValue(data.line_height);
    //字体类型
    ui->font_type->setCurrentFont(data.fmt.font());
    //字体大小
    ui->font_space->setValue(data.fmt.fontPointSize());
    //字体颜色
    ui->font_color_edit->setText(data.fmt.foreground().color().name());
    //背景颜色
    ui->background_color_edit->setText(data.background_color.name());

    //书籍界面
    //自动保存
    ui->auto_save_book->setChecked(data.auto_save_book);
    ui->auto_save_book_frequency->setValue(data.auto_save_book_frequency);

    //笔记界面
    //自动保存
    ui->auto_save_notes->setChecked(data.auto_save_notes);
    ui->auto_save_notes_frequency->setValue(data.auto_save_notes_frequency);
    //自动聚焦
    ui->auto_focus->setChecked(data.auto_set_focus);

//    connect(ui->btn_ok,&QPushButton::clicked,[&data,this](){

//        //阅读界面
//        //日-夜间模式
//        if(ui->day_mode->isChecked())
//            data.dark_mode=0;
//        else if()
//            ui->night_mode->setChecked(1);
//        //读写模式
//        if(data.R_W_mode)
//            ui->reading_mode->setChecked(1);
//        else
//            ui->editing_mode->setChecked(1);
//        //边距
//        ui->margin->setValue(data.text_margin);
//        //行距
//        ui->line_height->setValue(data.line_height);
//        //字体类型
//        ui->font_type->setCurrentFont(data.charformat.font());
//        //字体大小
//        ui->font_space->setValue(data.charformat.fontPointSize());
//        //字体颜色
//        ui->font_color_edit->setText(data.charformat.foreground().color().name());
//        //背景颜色
//        ui->background_color_edit->setText(data.background_color.name());

//        //书籍界面
//        //自动保存
//        ui->auto_save_book->setChecked(data.auto_save_book);
//        ui->auto_save_book_frequency->setValue(data.auto_save_book_frequency);

//        //笔记界面
//        //自动保存
//        ui->auto_save_notes->setChecked(data.auto_save_notes);
//        ui->auto_save_notes_frequency->setValue(data.auto_save_notes_frequency);
//        //自动聚焦
//        ui->auto_focus->setChecked(data.auto_set_focus);
//    })
}

Setting_Dialog::~Setting_Dialog()
{
    delete ui;
}
