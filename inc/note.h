#ifndef NOTE_H
#define NOTE_H

#include<QHash>
#include<QStringList>
#include<QString>
#include<QList>
#include<QFile>
#include<QDataStream>
#include<QMessageBox>
#include<QTreeWidget>
#include<QDebug>
#include<QTreeWidgetItem>

//笔记版本
#define NOTE_VERSION_0 (QString("0_***version_&&&***"))
//#define NOTE_VERSION_1 (QString("1_***version_&&&***"))


class Note_Item
{
public:
    Note_Item(QTreeWidgetItem*item,const QString &Key_structure,QStringList Content=QStringList());


public:
    bool operator==(const Note_Item& obj)
    {
        if(this->key_structure==obj.key_structure
                &&this->content==obj.content)
            return 1;
        else
            return 0;
    }
    QString getKey_structure() const;
    void setKey_structure(const QString &value);

    QStringList getContent() const;
    void setContent(const QStringList &value);

    QTreeWidgetItem *getWidget_item() const;

    void append(const QString &piece);
    inline int count_piece(){return content.size();}
    void remove_at(int i)
    {
        if(i<0||i>=content.size())return;
        content.removeAt(i);
    }
    void set_piece_at(int i,const QString &value)
    {
        if(i<0||i>=content.size())return;
        content[i]=value;
    }
    QString get_piece_at(int i)
    {
        return content.value(i);
    }
    inline QString get_key_name()
    {
        return key_structure.split('-').back();
    }

private:
    QString key_structure;          //层级名：层1-层2-...层n-key
    QStringList content;            //内容
    QTreeWidgetItem* widget_item;   //显示窗体
};
Q_DECLARE_METATYPE(Note_Item*);


class Note
{
public:
    Note();

    bool read(QString file_dir,QTreeWidget*tree);
    void save(QString file_dir);

    Note_Item* add(const QString &lever_key,const QString &content,QTreeWidget*tree);
    Note_Item* node_at(int i);
    Note_Item* find_node(const QString &lever_key);
    void rename(QString old_key,QString new_key,QTreeWidget*tree);
//    void set_tree_widget(QTreeWidget*p);

    QStringList getKey_structure() const;
    bool contains(const QString &lever_key)
    {
        return key_structure.contains(lever_key);
    }
    void remove_at(int i)
    {
        if(i<0||i>data.size()-1)return;
        delete data[i].getWidget_item();
        data.removeAt(i);
    }
    void remove_one(const Note_Item &i)
    {
        delete i.getWidget_item();
        data.removeOne(i);
    }
private:
    Note_Item*get_item(const QString &lever_key);
    int version;
    QStringList key_structure;              //储存key的层级结构，以-分隔
    QList<Note_Item> data;                  //每个QStringList为一个key本身和与key对应的笔记的集合
};

#endif // NOTE_H
