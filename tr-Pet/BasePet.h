#pragma once
#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QString>
#include <QMovie>
#include <QTimer>
#include <QPropertyAnimation>
#include <QList> 
#include <QEnterEvent> // 用来支持鼠标悬停
#include <QSoundEffect>
#include <QUrl>

// 这里把所有角色都列出来，以后加新角色只要在这里加个名字就行
enum PetRole {
    Role_RainbowSlime,    // 史莱姆
    Role_Plantera,        // 世花
    Role_Skeletron,       // 骷髅王
    Role_DoG,             // 神吞
    Role_SCal,            // 至尊灾厄
    Role_Mutant,          // 突变体

    Role_Nurse,           // 护士
    Role_ArmsDealer,      // 军火商
    Role_PartyGirl,       // 派对女孩
    Role_Goblin,          // 哥布林工匠
    Role_Bandit,          // 强盗
    Role_TaxCollector,    // 税收官
    Role_Operator         // 操作员
};

class BasePet : public QWidget {
    Q_OBJECT
public:
    // 构造函数：告诉程序“这是什么角色”以及“它的图片在哪”
    explicit BasePet(PetRole role, const QString& imagePath, QWidget* parent = nullptr);
    //析构函数：宠物消失时要从花名册除名
    ~BasePet(); 
    //静态花名册，记录当前屏幕上所有的宠物
    static QList<BasePet*> s_petList;
    //查户口
    static bool isPetAlive(PetRole roleToCheck);
    //让别人能知道自己的身份
    PetRole getRole() const { return m_role; }
    //全局背景音乐总控
    static QSoundEffect* s_bgmPlayer;
    static QString s_currentBGMPath;//用来记住当前正在播放的音乐路径
    static void playGlobalMusic(const QString& musicPath);
    static void stopBGM();

    static QSoundEffect* s_reforgeEffect;
    static QSoundEffect* s_coinEffect;

protected:
    // 重写鼠标的三大事件，为了能用鼠标拖着它满屏幕跑
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    //鼠标悬停
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    PetRole m_role;
    QLabel* m_imageLabel;   // 用来装图片的画框
    QMovie* m_movie;        // 用来播放GIF的放映机
    QLabel* m_textLabel;    // 用来显示字幕的文本框

    bool m_isFlipped;       // 记录当前是不是镜像状态
    void updateImageFlip(); // 专门用来执行翻转的函数
    QPixmap getFlippedPixmap(const QPixmap& pix); 
    QPixmap m_originalPixmap;                     // 翻转图片的工具

    // 拖拽需要的两个小工具
    bool m_isDragging;
    QPoint m_dragPosition;
    QPoint m_pressPos;  // 记录鼠标刚按下去时的位置
    void onClick();     // 专门用来处理“被点击”后该干嘛的函数

    QTimer* m_actionTimer;              // 定时器：决定隔多久动一次
    QPropertyAnimation* m_moveAnimation;// 动画引擎：负责平滑移动
    void randomJump();// 专门用来执行随机跳跃的函数

    //互相探测的雷达功能
    QTimer* m_radarTimer;
    void checkInteractions();

    //记录世纪之花是否进入了二阶段
    bool m_isPhase2 = false ;
    bool m_isAwakened = false;
};

//礼花
#include <QPainter>
class ConfettiWidget : public QWidget {
    Q_OBJECT
public:
    ConfettiWidget(QPoint startPos, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    struct Particle {
        float x, y, vx, vy; // 坐标和速度
        QColor color;       // 颜色
        float size;         // 大小
    };
    QList<Particle> m_particles;
    QTimer* m_timer;
    int m_lifeTime;
};

//弹幕控制系统
#include <QtMath> // 需要用到数学计算角度
class DanmakuWidget : public QWidget 
{
    Q_OBJECT
public:
    DanmakuWidget(QPoint startPos, QWidget* parent = nullptr);
    ~DanmakuWidget();
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    struct Bullet {
        float x, y;
        float vx, vy;
        float angle; // 飞行角度（0度为正上方）
    };
    QList<Bullet> m_bigBullets;
    QList<Bullet> m_smallBullets;

    QMovie* m_bigMovie;
    QMovie* m_smallMovie;
    QTimer* m_timer;

    int m_phase;      // 1: 大弹幕飞行中, 2: 分裂成小弹幕
    int m_phaseTimer; // 当前阶段的倒计时
};