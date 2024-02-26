#ifndef NOTE_H
#define NOTE_H

#include <QWidget>
#include <QTreeView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMouseEvent>
#include <QToolBar>
#include <QVBoxLayout>
#include <QMimeData>
#include <QDrag>
#include <QPixmap>
#include <QListWidget>

class Note_Tree : public QTreeWidget
{
    Q_OBJECT

public:
    explicit Note_Tree(QWidget *parent = 0);
    ~Note_Tree();

private:

protected:
//    void mousePressEvent(QMouseEvent *event);      // 鼠标按下事件
//    void dragEnterEvent(QDragEnterEvent *event);   // 拖动进入事件
//    void dragMoveEvent(QDragMoveEvent *event);     // 拖动事件
//    void dropEvent(QDropEvent *event);             // 放下事件  .

};

class Note{
public:
    Note(const QString &KEY,const QString &Content):key(KEY),content(Content){};
    QString key;
    QStringList content;

    bool operator==(Note a)
    {
        if(key==a.key&&content==a.content)return 1;
        else return 0;
    }
};

class Notes : public QWidget
{
    Q_OBJECT
public:
    explicit Notes(QWidget *parent = nullptr);

    bool contains(QString key)
    {
        if(key.size()==0||notes.size()==0)return 0;
        key=key.trimmed();
        bool res=0;
        for(auto i:notes)
        {
            if(i.key==key)
            {
                res=1;
                return res;
            }
        }
        return res;
    }

    Note* find(const QString &KEY)
    {
        if(KEY.size()==0||notes.size()==0)return Q_NULLPTR;
        for(auto &i:notes)
        {
            if(i.key==KEY)
            {
                return &i;
            }
        }
        return Q_NULLPTR;
    }
    void clear()
    {
        for(auto &i:notes)
        {
            i.key.clear();
            i.content.clear();
        }
        notes.clear();
    }
    bool read();
    bool save();

    void add(QString key, QString content)
    {
        if(key.size()==0||content.size()==0)return;

        key=key.trimmed();
        content=content.trimmed();

        auto list=find(key);
        if(list!=Q_NULLPTR)
        {
            list->content.append(content);
        }
        else
        {
            notes.append({key,content});
        }
    }
    void rename_key(const QString &old_key)
    {
        auto ans=QInputDialog::getText(Q_NULLPTR,"对"+old_key+"重命名","命名为：",QLineEdit::Normal,old_key);
        ans=ans.trimmed();

        if(ans.size()==0||ans==old_key)return;

        if(contains(ans))
        {
            QMessageBox::warning(Q_NULLPTR,"命名","名称已存在，命名无效！");
            return;
        }
        auto obj=find(old_key);
        for(int i=1;i<obj->content.size();i++)
            add(ans,obj->content[i]);
        notes.removeOne(*obj);
        is_modefied=1;
    }
    void remove(const QString &key)
    {
        if(key.size()==0)return;
        auto obj=find(key);
        if(obj==Q_NULLPTR)return;
        notes.removeOne(*obj);
        is_modefied=1;
    }
    QStringList get_key_list(const QString &key)
    {
        QStringList res;
        if(key.size()==0)return res;
        auto obj=find(key);
        if(obj==Q_NULLPTR)return res;
        for(int i=0;i<obj->content.size();i++)
        {
            res.push_back(obj->content[i]);
        }
        return res;
    }

    void show_item(QTreeWidget *view)
    {
        if(view==Q_NULLPTR)return;
        view->clear();

    }

private:
    QList<Note> notes;   //每个QStringList为一个key本身和与key对应的笔记的集合
    bool is_modefied;

    QToolBar *toolbar;
    QTreeWidget *key_view;
    QListWidget *content_view;

signals:

};

#endif // NOTE_H
