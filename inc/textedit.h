#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>
#include <QDebug>
#include <QFont>

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEdit(bool read=1,QWidget *parent = nullptr);
    void contextMenuEvent(QContextMenuEvent *e) override;

    int get_line_height();
//    int get_letter_space();
//    void set_letter_space(qreal spacing,enum QFont::SpacingType type=QFont::AbsoluteSpacing);

    void set_line_height(qreal height, int heightType=QTextBlockFormat::FixedHeight);
    int get_letter_number();
    void clear();
    void setText(const QString &text,int line_height);

signals:
    void letter_count(int count);//字数统计

private:
    int count;

};

#endif // TEXTEDIT_H
