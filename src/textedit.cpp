#include "../inc/textedit.h"
#include<QMenu>
#include<QContextMenuEvent>
#include<QDialog>
#include<QSpinBox>
TextEdit::TextEdit(bool read,QWidget *parent) : QTextEdit(parent)
{
    count=0;

    setReadOnly(read);
    set_line_height(45);
//    set_letter_space(10);

    //字数
    connect(this,&QTextEdit::textChanged,[this]{
        auto str=toPlainText();
        count=0;
        for(auto i:str)
        {
            if(!i.isSpace())
                count++;
        }
        emit letter_count(count);
    });
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

int TextEdit::get_line_height()
{
    QTextCursor cursor=textCursor();
    QTextBlockFormat blockFormat=cursor.blockFormat();
    //        int &line_height_type
    //        line_height_type=blockFormat.lineHeightType();
    return blockFormat.lineHeight();
}

//int TextEdit::get_letter_space()
//{
//    auto text_format=currentCharFormat();
//    auto font=text_format.font();
//    return font.letterSpacing();
//}

//void TextEdit::set_letter_space(qreal spacing, QFont::SpacingType type)
//{
//    auto text_format=currentCharFormat();
//    auto font=text_format.font();
//    font.setLetterSpacing(type,spacing);
//    text_format.setFont(font);

//    auto cursor=textCursor();
//    cursor.select(QTextCursor::Document);
//    cursor.setCharFormat(text_format);
//    cursor.clearSelection();

//    setFont(font);
//}

void TextEdit::set_line_height(qreal height, int heightType)
{
    QTextCursor cursor=textCursor();
    QTextBlockFormat blockFormat=cursor.blockFormat();
    blockFormat. setLineHeight ( height, heightType); //设置行间距
    cursor.select(QTextCursor::Document);
    cursor.setBlockFormat(blockFormat);
    cursor.clearSelection();

    //将光标放回原来的位置
    cursor.setPosition(textCursor().position());
    setTextCursor(cursor);//更换当前光标
}

int TextEdit::get_letter_number()
{
    return count;
}

void TextEdit::clear()
{
    QTextEdit::clear();
    count=0;
}

void TextEdit::setText(const QString &text, int line_height)
{
    QTextEdit::setText(text);
//    set_letter_space(letter_space);
    set_line_height(line_height);
}
