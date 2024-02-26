#include "../inc/textedit.h"
#include<QMenu>
#include<QContextMenuEvent>
TextEdit::TextEdit(QWidget *parent) : QTextEdit(parent)
{

}

void TextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    auto menu=QTextEdit::createStandardContextMenu();

    auto font_color=this->textColor();
    auto r=font_color.red();
    auto g=font_color.green();
    auto b=font_color.blue();
    auto sheet="color:rgb("+QString::number(r)+","+QString::number(g)+","+QString::number(b)+")";
    menu->setStyleSheet(sheet);

    menu->exec(e->globalPos());
    delete menu;
}
