#include "../inc/note.h"

Note::Note()
{

}

bool Note::read(QString file_dir,QTreeWidget*tree)
{
    //打开文件，获取数据流
    if(file_dir.size()==0)return 0;
    bool res=0;
    QFile f(file_dir);
    if(!f.open(QIODevice::ReadOnly))
        return 0;
    QDataStream In(&f);
    //判断版本
    QString test;
    In>>test;
    if(test!=NOTE_VERSION_0)
    {
        In.device()->reset();
    }
    else
        version=0;

    //按版本读取
    QString lever_key;
    QStringList content_list;
    switch (version)
    {
    case 0:
        while(!f.atEnd())
        {
            In>>lever_key;
            In>>content_list;
            content_list.sort();
            for(int index=0;index<content_list.size();index++)
                res=this->add(lever_key,content_list[index],tree);
            content_list.clear();
        }
        break;
    default:
        while(!f.atEnd())
        {
            In>>content_list;
            content_list.sort();
            if(content_list.count())
                for(int index=1;index<content_list.size();index++)
                    res=this->add(content_list[0],content_list[index],tree);
            content_list.clear();
        }

    }

    f.close();
    return res;
}

void Note::save(QString file_dir)
{
    if(file_dir.size()==0)return;
    QFile f(file_dir);
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(Q_NULLPTR,"打开文件","打开word list文件失败！");
        return;
    }
    QDataStream Out(&f);
    Out<<NOTE_VERSION_0;

    for(auto &i:data)
    {
        Out<<i.getKey_structure();
        Out<<i.getContent();
    }
    f.close();
}

Note_Item* Note::add(const QString &lever_key, const QString &content,QTreeWidget*tree)
{
    if(lever_key.size()==0||content.size()==0||tree==Q_NULLPTR)return Q_NULLPTR;

    auto list=lever_key.split('-', QString::SkipEmptyParts);
    if(list.size()==0)return Q_NULLPTR;

    //若不存在，添加项目窗体
    if(!key_structure.contains(lever_key))
    {
        //确定顶层项目
        QTreeWidgetItem*root_item=Q_NULLPTR;
        for(int j=0;j<tree->topLevelItemCount();j++)
            if(tree->topLevelItem(j)->text(0)==list[0])
            {
                root_item=tree->topLevelItem(j);
                break;
            }
        if(root_item==Q_NULLPTR)
        {
            root_item=new QTreeWidgetItem({list[0]});
            tree->addTopLevelItem(root_item);
            tree->scrollToItem(root_item);
        }
        //确定次级到末尾
        QTreeWidgetItem* temp_item=root_item;
        bool is_exist=0;
        for(int i=1;i<list.size();i++)
        {
            is_exist=0;
            qDebug()<<list[i];
            //已存在该层级
            for(int j=0;j<temp_item->childCount();j++)
            {
                qDebug()<<temp_item->child(j)->text(0);
                if(temp_item->child(j)->text(0)==list[i])
                {
                    temp_item=temp_item->child(j);
//                    tree->expandItem(temp_item);
                    is_exist=1;
                    break;
                }
            }
            if(is_exist)continue;
            auto child=new QTreeWidgetItem({list[i]});
//            tree->expandItem(temp_item);
            temp_item->addChild(child);
            temp_item=child;
            tree->scrollToItem(temp_item);
        }
        key_structure.push_back(lever_key);
        key_structure.sort();
        //添加笔记
        Note_Item new_node(temp_item,lever_key,{content});
        data.append(new_node);
        temp_item->setData(0,Qt::UserRole,QVariant::fromValue(&data.back()));
        return &data.back();
    }
    //若存在，添加到数据库
    for(auto &node:data)
    {
        //若已经存在该key
        if(lever_key==node.getKey_structure())
        {
            node.append(content);
            return &node;
        }
    }
    return Q_NULLPTR;
}

Note_Item *Note::node_at(int i)
{
    if(i<0||i>data.size()-1)return Q_NULLPTR;
    return &data[i];
}

Note_Item *Note::find_node(const QString &lever_key)
{
    for(auto &i:data)
    {
        if(i.getKey_structure()==lever_key)
            return &i;
    }
    return Q_NULLPTR;
}

void Note::rename(QString old_key, QString new_key,QTreeWidget*tree)
{
    if(!key_structure.contains(old_key))return;
    if(key_structure.contains(new_key))
    {
        auto res=QMessageBox::question(Q_NULLPTR,"警告",new_key+"已存在，要将"+old_key+"中的元素转移到"+new_key+"中吗？");
        if(res!=QMessageBox::Yes)
            return;
    }

    auto obj=find_node(old_key);
    if(!obj)return;
    for(int i=0;i<obj->count_piece();i++)
    {
        add(new_key,obj->getContent()[i],tree);
    }
    //窗体上的删除
    delete obj->getWidget_item();
    //数据上的删除
    key_structure.removeOne(old_key);
    data.removeOne(*obj);
    return;
}


QStringList Note::getKey_structure() const
{
    return key_structure;
}

Note_Item::Note_Item(QTreeWidgetItem*item,const QString &Key_structure, QStringList Content)
{
    widget_item=item;
    key_structure=Key_structure;
    content=Content;
}

QString Note_Item::getKey_structure() const
{
    return key_structure;
}

void Note_Item::setKey_structure(const QString &value)
{
    key_structure = value;
}

QStringList Note_Item::getContent() const
{
    return content;
}

void Note_Item::setContent(const QStringList &value)
{
    content = value;
}

QTreeWidgetItem *Note_Item::getWidget_item() const
{
    return widget_item;
}

void Note_Item::append(const QString &piece)
{
    content<<piece;
}
