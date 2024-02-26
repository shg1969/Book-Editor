#include "inc/note.h"

Note_Tree::Note_Tree(QWidget *parent):QTreeWidget(parent)
{
    setAcceptDrops(true);                      // 设置窗口部件可以接收拖入
    setHeaderHidden(1);
    setItemsExpandable(1);
    setExpandsOnDoubleClick(1);
    setAutoExpandDelay(100);

    QTreeWidgetItem *a=new QTreeWidgetItem({"a"});
    QTreeWidgetItem *b=new QTreeWidgetItem({"b"});
    QTreeWidgetItem *c=new QTreeWidgetItem({"c"});
    QTreeWidgetItem *d=new QTreeWidgetItem({"d"});
    QTreeWidgetItem *e=new QTreeWidgetItem({"e"});
    a->addChild(new QTreeWidgetItem({"1"}));
    a->addChild(new QTreeWidgetItem({"2"}));
    a->addChild(new QTreeWidgetItem({"3"}));
    e->addChild(new QTreeWidgetItem({"4"}));
    e->addChild(new QTreeWidgetItem({"5"}));

    addTopLevelItem(a);
    addTopLevelItem(b);
    addTopLevelItem(c);
    addTopLevelItem(d);
    addTopLevelItem(e);
}

Note_Tree::~Note_Tree()
{

}

//void Note_Tree::mousePressEvent(QMouseEvent *event)   //鼠标按下事件
//{
//    // 第一步：获取Note
//    // 将鼠标指针所在位置的部件强制转换为QTreeWidgetItem类型
//    QTreeWidgetItem *item = itemAt(event->pos());
//    if(item==Q_NULLPTR) return; // 如果部件不是QTreeWidgetItem则直接返回
//    QString key=item->text(0);

//    // 第二步：自定义MIME类型
//    QByteArray itemData;                                     // 创建字节数组
//    QDataStream dataStream(&itemData, QIODevice::WriteOnly); // 创建数据流
//    // 将key，位置信息输入到字节数组中
//    dataStream << key;

//    // 第三步：将数据放入QMimeData中
//    QMimeData *mimeData = new QMimeData;  // 创建QMimeData用来存放要移动的数据
//    // 将字节数组放入QMimeData中，这里的MIME类型是我们自己定义的
//    mimeData->setData("myimage/png", itemData);

//    // 第四步：将QMimeData数据放入QDrag中
//    QDrag *drag = new QDrag(this);      // 创建QDrag，它用来移动数据
//    drag->setMimeData(mimeData);
//    drag->setPixmap(QPixmap());//在移动过程中显示图片，若不设置则默认显示一个小矩形
////    drag->setHotSpot(event->pos() - item->pos()); // 拖动时鼠标指针的位置不变

////    // 第五步：给原笔记节点添加标记
////    item->setBackground(0,Qt::gray);

//    // 第六步：执行拖放操作
//    if (drag->exec(Qt::MoveAction)
//            == Qt::MoveAction)  // 设置拖放可以是移动和复制操作，默认是复制操作
//        delete item;        // 如果是移动操作，那么拖放完成后关闭原标签
//}

//void Note_Tree::dragEnterEvent(QDragEnterEvent *event) // 拖动进入事件
//{
//      // 如果有我们定义的MIME类型数据，则进行移动操作
//     if (event->mimeData()->hasText()) {
//             event->setDropAction(Qt::MoveAction);
//             event->accept();
//     } else {
//         event->ignore();
//     }
//}
//void Note_Tree::dragMoveEvent(QDragMoveEvent *event)   // 拖动事件
//{
//     if (event->mimeData()->hasText()) {
//             event->setDropAction(Qt::MoveAction);
//             event->accept();
//     } else {
//         event->ignore();
//     }
//}

//void Note_Tree::dropEvent(QDropEvent *event) // 放下事件
//{
//    if (event->mimeData()->hasText()) {
//         QByteArray itemData = event->mimeData()->data("text/plain");
//         QDataStream dataStream(&itemData, QIODevice::ReadOnly);
//         QString key;
//         // 使用数据流将字节数组中的数据读入到key中
//         dataStream >> key;
//         // 新建项
//         // 将鼠标指针所在位置的部件强制转换为QTreeWidgetItem类型
//         QTreeWidgetItem *item = itemAt(event->pos());
//         if(item==Q_NULLPTR) return; // 如果部件不是QTreeWidgetItem则直接返回
//         item->addChild(new QTreeWidgetItem({key}));
//         event->setDropAction(Qt::MoveAction);
//         event->accept();
//     } else {
//         event->ignore();
//     }
//}


Notes::Notes(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlag(Qt::FramelessWindowHint);

    QVBoxLayout *main_layout=new QVBoxLayout(this);
    main_layout->setMargin(0);

    toolbar=new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    key_view=new QTreeWidget(this);
    key_view->setHeaderHidden(1);

    main_layout->addWidget(toolbar);
    main_layout->addWidget(key_view);

    auto act=toolbar->addAction("123");
}

