#include "BasePet.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QEasingCurve>
#include <QTimer>
#include <QGuiApplication> 
#include <QPropertyAnimation> // 用于突变体冲刺残影淡出特效
//加音效
#include <QUrl>
#include <QSoundEffect>

QList<BasePet*> BasePet::s_petList;

QSoundEffect* BasePet::s_reforgeEffect = nullptr;
QSoundEffect* BasePet::s_coinEffect = nullptr;

// 遍历全局名单，检查这个角色在不在
bool BasePet::isPetAlive(PetRole roleToCheck) {
    for (int i = 0; i < s_petList.size(); ++i) {
        if (s_petList[i]->m_role == roleToCheck) {
            return true; // 逮到了！他已经在屏幕上了！
        }
    }
    return false; // 屏幕上没这个桌宠
}

// 初始化全局指针
QSoundEffect* BasePet::s_bgmPlayer = nullptr;
// 初始化记忆变量为空
QString BasePet::s_currentBGMPath = "";
// 全局万能点唱机
void BasePet::playGlobalMusic(const QString& musicPath) 
{
    //如果要求放的歌，就是当前正在放的歌，直接无视！让它接着奏乐接着舞！
    if (s_currentBGMPath == musicPath)
        return;
    // 如果是新歌，更新记忆，准备切歌！
    s_currentBGMPath = musicPath;

    if (!s_bgmPlayer) {
        s_bgmPlayer = new QSoundEffect();
        s_bgmPlayer->setLoopCount(QSoundEffect::Infinite); // 依然无限循环
        s_bgmPlayer->setVolume(0.3f);
    }
    else {
        // 【核心操作】：如果点唱机已经存在，不管它在播什么，先给我强行闭嘴！
        s_bgmPlayer->stop();
        s_bgmPlayer->disconnect(); // 切断之前的加载监听，防止混乱
    }

    // 换上最新召唤的 Boss（或白天）的音乐磁带
    s_bgmPlayer->setSource(QUrl::fromLocalFile(musicPath));

    // 重新监听加载状态（防吞音效神技）
    QObject::connect(s_bgmPlayer, &QSoundEffect::statusChanged, []() {
        if (s_bgmPlayer->status() == QSoundEffect::Ready) {
            s_bgmPlayer->play();
        }
        });

    // 如果加载得够快，直接开播
    if (s_bgmPlayer->status() == QSoundEffect::Ready) {
        s_bgmPlayer->play();
    }
}

// 停止全局BGM
void BasePet::stopBGM() {
    if (s_bgmPlayer) {
        s_bgmPlayer->stop();
    }
}

BasePet::BasePet(PetRole role, const QString& imagePath, QWidget* parent)
    : QWidget(parent), m_role(role), m_isDragging(false), m_movie(nullptr)
{
    m_isFlipped = false;
    m_isPhase2 = false;

    // 去掉系统自带的灰色边框，并让窗口永远置顶在最前面！
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // 把窗口背景变成完全透明！
    setAttribute(Qt::WA_TranslucentBackground);
    // 准备用来显示图片的控件
    m_imageLabel = new QLabel(this);
    //m_imageLabel->setScaledContents(true); // 允许图片自动缩放填充，不行 这样会使图片变形

     //设定桌宠的理想大小
    int targetSize;
    if (m_role == Role_DoG)
        targetSize = 600;
    else if (m_role == Role_RainbowSlime)
        targetSize = 130;
    else if (m_role == Role_Skeletron)
        targetSize = 550;
    else if (m_role == Role_Mutant)
        targetSize = 300;
    else if (m_role == Role_Plantera)
        targetSize = 100;
    else if (m_role == Role_SCal)
        targetSize = 270;
    else
        targetSize = 180;
    // 判断是动图还是静图，分别处理
    if (imagePath.endsWith(".gif", Qt::CaseInsensitive)) {
        m_movie = new QMovie(imagePath, QByteArray(), this);
        m_movie->start(); // 先启动一下才能拿到尺寸

        // 拿到 GIF 的原始比例，然后智能缩小到 targetSize，不变形
        QSize originalSize = m_movie->currentImage().size();
        if (!originalSize.isEmpty()) {
            QSize newSize = originalSize.scaled(targetSize, targetSize, Qt::KeepAspectRatio);
            m_movie->setScaledSize(newSize);
        }
        m_imageLabel->setMovie(m_movie);
    }
    else {
        QPixmap pix(imagePath);
        // 让静态图片按比例智能缩放，不变形
        pix = pix.scaled(targetSize, targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // 缩放完之后，把这张图备份
        m_originalPixmap = pix;

        m_imageLabel->setPixmap(pix);
    }

    //让本体的布局里只有图片，尺寸完全锁定，不偏移
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_imageLabel);

    // 把文字/表情框当成独立悬浮层
    m_textLabel = new QLabel(this); // this认爹，防止野指针
    m_textLabel->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    m_textLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_textLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->hide();

    // 挂载一个专属的追踪定时器，每帧同步文字/表情的位置
    QTimer* textTracker = new QTimer(this);
    connect(textTracker, &QTimer::timeout, this, [=]() {
        if (!m_textLabel->isHidden()) {
            m_textLabel->adjustSize(); // 确保框能包住文字或动图
            QRect petRect = this->geometry();
            int popX = petRect.center().x() - m_textLabel->width() / 2;
            int popY = 0;

            // 哥布林、护士、军火商、强盗、操作员、税收官的表情在头顶冒出
            if (m_role == Role_Goblin || m_role == Role_Nurse || m_role == Role_ArmsDealer ||
                m_role == Role_Bandit || m_role == Role_TaxCollector || m_role == Role_Operator) {
                popY = petRect.top() - m_textLabel->height() - 5;
            }
            // 其他角色（如世纪之花）在脚底冒出
            else {
                popY = petRect.bottom() + 5;
            }
            m_textLabel->move(popX, popY);
        }
        });
    textTracker->start(16); // 60FPS实时刷新位置



    if (m_role == Role_DoG) 
    {
        // 设置神吞的台词
        m_textLabel->setText(QString::fromLocal8Bit("你不是神...但你的灵魂仍是我的盛宴！\n你确实出类拔萃，但别狂妄自大！ \n 还没完呢！小子！\n神！不惧死亡！"));
        //紫色字体，加粗
        m_textLabel->setStyleSheet("color: #a349a4; font-weight: bold; font-size: 20px;");
        m_textLabel->show(); // 显示字幕
    }

    //初始化大脑和双腿
    m_actionTimer = new QTimer(this);
    m_moveAnimation = new QPropertyAnimation(this, "pos", this); // "pos"代表我们要改变窗口的坐标位置
    //史莱姆跳来跳去
    if (m_role == Role_RainbowSlime) 
    {
        connect(m_actionTimer, &QTimer::timeout, this, [=]() {
            if (!m_isDragging) { // 如果正在用鼠标拖它，就不动
                randomJump();
            }
            });
        m_actionTimer->start(3000); // 设定每3秒触发一次跳跃
    }

    //刚出生，把自己登记到花名册里
    s_petList.append(this);
    //开启雷达，每隔 0.5 秒环顾一下四周
    m_radarTimer = new QTimer(this);
    connect(m_radarTimer, &QTimer::timeout, this, &BasePet::checkInteractions);
    m_radarTimer->start(500);

}

// ================= 鼠标拖拽功能实现 =================

void BasePet::mousePressEvent(QMouseEvent* event) 
{
	//右键点击，收回桌宠
    if (event->button() == Qt::RightButton) 
    {
        //先把它从名单里除名（此时它已经不在这份名单里了）
        s_petList.removeOne(this);
        //BGM 智能回退系统 (寻找最后一个幸存的 Boss)
        // ==========================================
        bool isBossAlive = false; // 标记桌面是否还有 Boss
        // 倒序遍历名单 (从最新召唤的开始往前找)
        for (int i = s_petList.size() - 1; i >= 0; --i) 
        {
            BasePet* survivor = s_petList[i];

            // 在这里列出你所有的 Boss！找到最新召唤的那一个，立刻切它的 BGM！
            if (survivor->m_role == Role_Plantera && survivor->m_isAwakened) {
                // 注意：世花必须是已经被点醒的状态才放音乐
                BasePet::playGlobalMusic("tr-pet_material/Music-Plantera.wav");
                isBossAlive = true;
                break; // 找到了最新的 Boss，直接打断寻找！
            }
            else if (survivor->m_role == Role_SCal) {
                BasePet::playGlobalMusic("tr-pet_material/Stained Brutal Calamity.wav");
                isBossAlive = true;
                break;
            }
            else if (survivor->m_role == Role_Mutant) 
            {
                BasePet::playGlobalMusic("tr-pet_material/rePrologue.wav"); 
                isBossAlive = true;
                break;
            }
            else if (survivor->m_role == Role_DoG) 
            { 
                BasePet::playGlobalMusic("tr-pet_material/Stained Brutal Calamity.wav");
                isBossAlive = true;
                break;
            }
            else if (survivor->m_role == Role_Skeletron)
            {
                BasePet::playGlobalMusic("tr-pet_material/Ezfic.wav");
                isBossAlive = true;
                break;
            }
        }
        // 如果找了一大圈，发现桌面上NPC
        if (!isBossAlive) 
            // 切回白天肝疼小曲！
            BasePet::playGlobalMusic("tr-pet_material/Music-Overworld_Day.wav");
        //安全销毁这个宠物的窗口
        this->deleteLater();
        return;
    }

    if (event->button() == Qt::LeftButton) 
    {
        m_isDragging = true;
        // 算出鼠标点下的位置和图片左上角的距离差
        m_dragPosition = event->globalPosition().toPoint() - this->frameGeometry().topLeft();

        // 鼠标抓住它时，强行停止跳跃动画
        if (m_moveAnimation->state() == QAbstractAnimation::Running)
        {
            m_moveAnimation->stop();
        }

        //记住鼠标点下去的初始坐标
        m_pressPos = event->globalPosition().toPoint();
        event->accept();
    }
}

void BasePet::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 鼠标移动时，带着窗口一起飞
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void BasePet::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false; // 松开左键，停止拖拽

    //判断是点击还是拖拽？
    //如果鼠标松开的位置，和刚才按下的位置距离没超过 5 个像素，就认为只是“点了一下”，而不是“拖走”
    QPoint releasePos = event->globalPosition().toPoint();
    if ((releasePos - m_pressPos).manhattanLength() < 5) {
        onClick(); // 触发点击事件
    }
    event->accept();
}

void BasePet::onClick()
{
    // 如果被点到的是世纪之花
    if (m_role == Role_Plantera && !m_isAwakened)
    {
        // 掐断白天的肝疼小曲！Boss战开始！
        BasePet::stopBGM();
        //音乐
        BasePet::playGlobalMusic("tr-pet_material/Music-Plantera.wav");

        m_isAwakened = true; // 标记进入一阶段
        m_textLabel->hide(); // 瞬间掐断悬停字幕

        if (m_movie) {
            m_movie->stop();
            delete m_movie;
            m_movie = nullptr;
        }
        m_imageLabel->clear();

        // 加载一阶段动图
        m_movie = new QMovie("tr-pet_material/plantera1.gif", QByteArray(), this);
        m_movie->start(); // 必须先 start 才能拿到正确尺寸！

        QSize size1 = m_movie->currentImage().size();
        if (size1.isValid()) 
            m_movie->setScaledSize(size1.scaled(200, 200, Qt::KeepAspectRatio));
        m_imageLabel->setMovie(m_movie);
        this->adjustSize();

        // 七秒后自动进二阶段
        QTimer::singleShot(7000, this, [=]() {
            if (m_movie) {
                m_movie->stop();
                delete m_movie;
            }
            m_movie = new QMovie("tr-pet_material/plantera2.gif");
            m_movie->setParent(this);
            m_movie->start();

            // 改名 size2 防止变量重名报错！
            QSize size2 = m_movie->currentImage().size();
            if (size2.isValid()) {
                m_movie->setScaledSize(size2.scaled(220, 220, Qt::KeepAspectRatio));
            }
            m_imageLabel->setMovie(m_movie);
            this->adjustSize();

            m_textLabel->setText(QString::fromLocal8Bit("↑→→→↑→↓↓↓↓↓"));
            m_textLabel->setStyleSheet("color: #FF0000; font-weight: bold; font-size: 30px;");
            m_textLabel->show();

            m_isPhase2 = true; // 标记正式进入二阶段
            });
        return;
    }

    // 如果被点到的是哥布林工匠
    if (m_role == Role_Goblin) 
    {
        // 专属的重铸音效（使用无敌的静态播放器）
        if (!s_reforgeEffect) {
            s_reforgeEffect = new QSoundEffect();
            s_reforgeEffect->setSource(QUrl::fromLocalFile("tr-pet_material/Item.wav"));
            s_reforgeEffect->setVolume(1.0f);
        }
        s_reforgeEffect->play();

        //准备一个“负面词库”
        QStringList badWords = {
            QString::fromLocal8Bit("碎裂 "),
            QString::fromLocal8Bit("惹恼 "),
            QString::fromLocal8Bit("粗劣 ")
        };

        //随机抽一个词汇
        int randomIndex = QRandomGenerator::global()->bounded(badWords.size());

        // 把抽到的词汇写到字幕框里，变成红色，假装是扣钱的提示！
        m_textLabel->setText(badWords[randomIndex] + " (-79 G) ");
        m_textLabel->setStyleSheet("color: #FF4500; font-weight: bold; font-size: 20px;");
        m_textLabel->show();

        // 等待 1500 毫秒（1.5秒）后，执行一次隐藏操作。文字弹出来一会儿就会自己消失，做出游戏气泡的感觉
        QTimer::singleShot(1500, this, [=]() {
            m_textLabel->hide();
            });
    }

    //如果被点到的是派对女孩
    if (m_role == Role_PartyGirl) {
        // 算出派对女孩在屏幕上的中心坐标
        QPoint centerPos = this->geometry().center();
        // 召唤物理礼花（就在她头顶炸开）
        ConfettiWidget* confetti = new ConfettiWidget(centerPos);
        confetti->show();
        return;
    }

    //如果被点到的是税收官
    if (m_role == Role_TaxCollector) 
    {     
        //金币音效(【终极解法：静态播放器】只读取一次硬盘，完美解决加载延迟和没声音！)
        if (!s_coinEffect) {
            s_coinEffect = new QSoundEffect();
            s_coinEffect->setSource(QUrl::fromLocalFile("tr-pet_material/Coin.wav"));
            s_coinEffect->setVolume(1.0f);
        }
        s_coinEffect->play();

        QPixmap moneyPix("tr-pet_material/coin.png");
        moneyPix = moneyPix.scaledToHeight(40, Qt::SmoothTransformation);

        m_textLabel->setPixmap(moneyPix);
        m_textLabel->show();

        // 1.5秒后自动隐藏并清空图片
        QTimer::singleShot(1500, this, [=]() {
            m_textLabel->hide();
            m_textLabel->clear();
            });

        return;
    }

    //如果被点到的是操作员
    if (m_role == Role_Operator)
    {
        // 【新增第1步】：进门先查锁！如果锁着（正在播放），直接return，不理她！
        if (this->property("isBagPlaying").toBool()) return;

        // 【新增第2步】：没锁的话，现在开始播放，马上把锁挂上！
        this->setProperty("isBagPlaying", true);

        // 清理旧的 GIF（防止内存泄漏）
        if (m_textLabel->movie()) {
            m_textLabel->movie()->deleteLater();
        }

        // 把宝藏袋 GIF 塞进统一的悬浮层 m_textLabel
        QMovie* bagMovie = new QMovie("tr-pet_material/treasurebag.gif", QByteArray(), this);
        bagMovie->jumpToFrame(0);
        QSize originalSize = bagMovie->currentImage().size();
        QSize newSize(60, 60);
        if (originalSize.isValid()) {
            newSize = originalSize.scaled(100, 100, Qt::KeepAspectRatio);
        }
        bagMovie->setScaledSize(newSize);

        m_textLabel->setMovie(bagMovie);
        bagMovie->start();
        m_textLabel->show();

        // 12150毫秒后，隐藏、删掉动图、并把锁解开
        QTimer::singleShot(12150, this, [=]() {
            m_textLabel->hide();
            if (m_textLabel->movie()) {
                m_textLabel->movie()->stop();
                m_textLabel->movie()->deleteLater();
                m_textLabel->setMovie(nullptr); // 清空动图
            }
            this->setProperty("isBagPlaying", false); // 解锁
            });

        return;
    }

    //如果被点到的是至尊女巫，灾厄
    if (m_role == Role_SCal) {
        // 弹幕从她的正中心射出
        QPoint centerPos = this->geometry().center();
        DanmakuWidget* danmaku = new DanmakuWidget(centerPos);
        danmaku->show();
        return;
    }

    //如果被点到的是突变体
    if (m_role == Role_Mutant)
    {
        if (this->property("isDashing").toBool()) return; // 防止狂点导致他在天上乱飞
        this->setProperty("isDashing", true);
        this->setProperty("crossed", false); // 记录是否穿过了屏幕边缘

        QPoint startPos = this->pos(); // 记住老家在哪
        int screenWidth = QGuiApplication::primaryScreen()->geometry().width();

        // 冲刺方向（根据当前面朝的方向，默认向右冲刺为 1，向左为 -1）
        int direction = m_isFlipped ? 1 : -1;
        int speed = 80 * direction; // 每帧飞 80 像素！极其生猛的速度！

        // 蓄力 0.5 秒 (500ms)
        QTimer::singleShot(500, this, [=]() {
            QTimer* dashTimer = new QTimer(this);
            connect(dashTimer, &QTimer::timeout, this, [=]() {
                QPoint curPos = this->pos();
                curPos.setX(curPos.x() + speed);

                // 穿透屏幕边缘的魔法空间跳跃
                if (direction > 0 && curPos.x() > screenWidth) {
                    curPos.setX(-this->width()); // 从右边飞出，瞬移到最左边
                    this->setProperty("crossed", true);
                }
                else if (direction < 0 && curPos.x() < -this->width()) {
                    curPos.setX(screenWidth); // 从左边飞出，瞬移到最右边
                    this->setProperty("crossed", true);
                }

                this->move(curPos); // 移动本体

                // --- 制造残影 ---
                QLabel* shadow = new QLabel();
                shadow->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
                shadow->setAttribute(Qt::WA_TranslucentBackground);

                // 【核心神技】瞬间抓拍 GIF 当前这一帧的画面！
                shadow->setPixmap(this->grab());
                shadow->resize(this->size());
                shadow->move(this->pos());
                shadow->setWindowOpacity(0.6); // 初始半透明
                shadow->show();

                // 残影在 150 毫秒内渐渐变透明并彻底消失
                QPropertyAnimation* anim = new QPropertyAnimation(shadow, "windowOpacity", shadow);
                anim->setDuration(150);
                anim->setStartValue(0.6);
                anim->setEndValue(0.0);
                connect(anim, &QPropertyAnimation::finished, shadow, &QLabel::deleteLater);
                anim->start();

                // 判断是否绕一圈回去了
                bool crossed = this->property("crossed").toBool();
                if (crossed) {
                    if ((direction > 0 && curPos.x() >= startPos.x()) ||
                        (direction < 0 && curPos.x() <= startPos.x())) {
                        dashTimer->stop();
                        dashTimer->deleteLater();
                        this->move(startPos); // 精确归位
                        this->setProperty("isDashing", false); // 冲刺结束
                    }
                }
                });
            dashTimer->start(16); // 60帧极速刷新
            });
        return;
    }
}

// ================= 史莱姆跳跃实现 =================
void BasePet::randomJump() {
    //获取你电脑屏幕的大小
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    //随机生成一个目标坐标（限制在屏幕范围内，不让它跑出屏幕）
    int targetX = QRandomGenerator::global()->bounded(0, screenGeometry.width() - this->width());
    int targetY = QRandomGenerator::global()->bounded(0, screenGeometry.height() - this->height());

    //跳跃动画
    m_moveAnimation->setDuration(1000); // 这一跳在空中花费 1秒（1000毫秒）
    m_moveAnimation->setStartValue(this->pos()); // 起点是当前位置
    m_moveAnimation->setEndValue(QPoint(targetX, targetY)); // 终点是随机新位置

    //使用 OutBounce 物理曲线，让它落地时有“Q弹”感觉
    m_moveAnimation->setEasingCurve(QEasingCurve::OutBounce);
    //起跳
    m_moveAnimation->start();
}

// ================= 宠物消失时的处理 =================
BasePet::~BasePet() {
    // 宠物被关闭时，从花名册里划掉名字
    s_petList.removeOne(this);
}

// ================= 鼠标悬停 =================
void BasePet::enterEvent(QEnterEvent* event) {
    // 如果是世纪之花
    if (m_role == Role_Plantera) {
        if (!m_isAwakened) {
            // 1. 花苞状态：显示名字
            m_textLabel->setText(QString::fromLocal8Bit("世纪之花灯泡 "));
            m_textLabel->setStyleSheet("color: #FF69B4; font-size: 20px;");
            m_textLabel->show();
        }
        else if (m_isAwakened && !m_isPhase2)
            m_textLabel->hide();
        else if (m_isPhase2)
            m_textLabel->show();
        return;
    }
}
void BasePet::leaveEvent(QEvent* event)
{
    if (m_role == Role_Operator && this->property("isBagPlaying").toBool()) 
        return;
    if (m_role == Role_DoG)
        return;
    if (m_role == Role_Plantera && m_isPhase2) {
        return;
    }
    m_textLabel->hide();
}

// ================= CP =================
void BasePet::checkInteractions() {
    // 如果我是护士，我就去满屏幕找军火商
    if (m_role == Role_Nurse || m_role == Role_ArmsDealer)
    {
        bool foundCP = false;

        // 确定我要找的人是谁？
        PetRole targetRole = (m_role == Role_Nurse) ? Role_ArmsDealer : Role_Nurse;

        //开始探测屏幕上的所有人
        for (BasePet* other : s_petList)
        {
            // 如果对方是我的CP
            if (other != this && other->getRole() == targetRole)
            {
                QPoint myCenter = this->geometry().center();
                QPoint hisCenter = other->geometry().center();
                int distance = (myCenter - hisCenter).manhattanLength();

                // 如果距离不到 200 像素，说明找到了！
                if (distance < 200)
                {
                    foundCP = true;
                    // 如果对方在我的右边，我就需要转身
                    bool shouldFlip = (hisCenter.x() > myCenter.x());
                    // 如果需要转身的状态发生了改变，翻转
                    if (m_isFlipped != shouldFlip) {
                        m_isFlipped = shouldFlip;
                        updateImageFlip();
                    }
                    break;
                }
            }
        }
        //两人互相靠近：加载并播放爱心动画
        if (foundCP)
        {
            // 如果目前没有显示爱心，就加载它
            if (m_textLabel->isHidden())
            {
                m_textLabel->setText(""); // 清空文字

                //加载爱心GIF
                QMovie* heartMovie = m_textLabel->movie();
                if (!heartMovie)
                {
                    // 第一次靠近，老老实实加载
                    heartMovie = new QMovie("tr-pet_material/Emotion_Love.gif", QByteArray(), this);
                    heartMovie->setScaledSize(QSize(80, 80));
                    m_textLabel->setMovie(heartMovie);
                }

                heartMovie->start();
                m_textLabel->show();
            }
        }
        else
        {
            // 如果走远了
            if (!m_textLabel->isHidden())
            {
                m_textLabel->hide();
                //只暂停，不 delete！保留动图，下次直接复用！
                if (m_textLabel->movie())
                    m_textLabel->movie()->stop();
            }
            //如果他们还在扭着头看对方，就让他们转回来
            if (m_isFlipped) {
                m_isFlipped = false;  // 恢复“不翻转”的状态
                updateImageFlip();    // 重新贴正向的图片
            }
        }
    }

    // ================= 强盗与哥布林 =================
    if (m_role == Role_Bandit || m_role == Role_Goblin) {
        bool foundTarget = false;
        // 强盗找哥布林，哥布林找强盗
        PetRole targetRole = (m_role == Role_Bandit) ? Role_Goblin : Role_Bandit;
        for (BasePet* other : s_petList) {
            if (other != this && other->getRole() == targetRole) {
                QPoint myCenter = this->geometry().center();
                QPoint hisCenter = other->geometry().center();
                int distance = (myCenter - hisCenter).manhattanLength();
                if (distance < 200) {
                    foundTarget = true;
                    // 判断左右并转身
                    bool shouldFlip = (hisCenter.x() > myCenter.x());
                    if (m_isFlipped != shouldFlip) {
                        m_isFlipped = shouldFlip;
                        updateImageFlip();
                    }
                    break;
                }
            }
        }
        //检查他们是不是“已经在附近了”
        bool wasNear = this->property("isNear").toBool();
        if (foundTarget) {
            // 如果是“刚靠近”（之前不在附近）
            if (!wasNear) {
                this->setProperty("isNear", true); // 记录状态：已经靠近了，防止重复触发
                if (m_role == Role_Goblin)
                {
                    // 哥布林：头顶冒出生气的 GIF，持续显示
                    m_textLabel->setText("");
                    QMovie* angryMovie = m_textLabel->movie();
                    if (!angryMovie) {
                        angryMovie = new QMovie("tr-pet_material/Emotion_Anger.gif", QByteArray(), this);
                        angryMovie->setScaledSize(QSize(60, 60)); // 根据你的生气图片调整大小
                        m_textLabel->setMovie(angryMovie);
                    }
                    angryMovie->start();
                    m_textLabel->show();
                }
                else if (m_role == Role_Bandit)
                {
                    // 强盗：头顶爆出金币图片，3秒后消失
                    QPixmap moneyPix("tr-pet_material/gold.png");
                    moneyPix = moneyPix.scaledToHeight(40, Qt::SmoothTransformation);
                    m_textLabel->setPixmap(moneyPix);
                    m_textLabel->show();

                    // 3秒后自动清空图片
                    QTimer::singleShot(3000, this, [=]() {
                        m_textLabel->hide();
                        m_textLabel->clear(); // 清空图层，保持干净
                        });
                }
            }
        }
        else
        {
            // 走远了
            if (wasNear)
            {
                this->setProperty("isNear", false); // 状态重置为“远离”
                // 哥布林消气，隐藏生气图片
                if (m_role == Role_Goblin)
                {
                    m_textLabel->hide();
                    if (m_textLabel->movie())
                        m_textLabel->movie()->stop();
                }
                // 转头，不理对方
                if (m_isFlipped)
                {
                    m_isFlipped = false;
                    updateImageFlip();
                }
            }
        }
    }
}

// ================= 图片镜像翻转 =================
QPixmap BasePet::getFlippedPixmap(const QPixmap& pix)
{
    // 使用 QImage 的 flipped
    return QPixmap::fromImage(pix.toImage().flipped(Qt::Horizontal));
}
void BasePet::updateImageFlip() {
    if (!m_originalPixmap.isNull()) {
        if (m_isFlipped)
            // 如果需要转身，就把角色的原图翻转后贴上去
            m_imageLabel->setPixmap(getFlippedPixmap(m_originalPixmap));

        else
            // 如果不需要转身，就贴正向的原图
            m_imageLabel->setPixmap(m_originalPixmap);
    }
}

// ================= 礼花爆炸 =================
ConfettiWidget::ConfettiWidget(QPoint startPos, QWidget* parent) : QWidget(parent) {
    // 设为无边框、置顶、全透明、穿透点击（这样礼花不会挡住你的鼠标操作）
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);
    // 让礼花画板铺满整个屏幕，这样碎纸片可以一直掉到屏幕最下面
    QRect screenGeom = QGuiApplication::primaryScreen()->geometry();
    setGeometry(screenGeom);
    // 生成 80 个五颜六色的碎片
    QList<QColor> colors = { Qt::red, Qt::yellow, Qt::blue, Qt::green, Qt::magenta, Qt::cyan, QColor("#FF69B4") };
    for (int i = 0; i < 80; i++) {
        Particle p;
        p.x = startPos.x();
        p.y = startPos.y();
        // 给一个随机的初速度，模拟“砰”的一声炸开！
        p.vx = (QRandomGenerator::global()->bounded(200) - 100) / 10.0f;     // 左右乱飞 (-10 到 10)
        p.vy = -(QRandomGenerator::global()->bounded(150)) / 10.0f - 8.0f;   // 向上喷发 (-8 到 -23)
        p.color = colors[QRandomGenerator::global()->bounded(colors.size())];
        p.size = QRandomGenerator::global()->bounded(5, 12); // 大小不一
        m_particles.append(p);
    }
    m_lifeTime = 120; // 存活 120 帧（大约 2 秒）
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [=]() {
        m_lifeTime--;
        if (m_lifeTime <= 0) {
            this->deleteLater(); // 播完自动销毁，绝不占用内存！
            return;
        }
        // 物理引擎核心：让碎片移动，并且加上重力下落！
        for (int i = 0; i < m_particles.size(); i++) {
            m_particles[i].x += m_particles[i].vx;
            m_particles[i].y += m_particles[i].vy;
            m_particles[i].vy += 0.5f; // 重力加速度：越掉越快
        }
        update(); // 触发重绘
        });
    m_timer->start(16); // 60帧极致丝滑刷新率
}
void ConfettiWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    for (const auto& p : m_particles) {
        painter.setBrush(p.color);
        painter.drawRect(QRectF(p.x, p.y, p.size, p.size)); // 画出彩纸片
    }
}

// ================= 灾厄弹幕 =================
DanmakuWidget::DanmakuWidget(QPoint startPos, QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);
    setGeometry(QGuiApplication::primaryScreen()->geometry());
    // 后台加载两个 GIF 动图并让它们一直播放（用于提取每一帧）
    m_bigMovie = new QMovie("tr-pet_material/bigbullet.gif", QByteArray(), this);
    m_smallMovie = new QMovie("tr-pet_material/smallbullet.gif", QByteArray(), this);
    m_bigMovie->start();
    m_smallMovie->start();
    // 阶段1：生成 3 个大弹幕，角度分别为 0(上), 120(右下), 240(左下)
    for (int i = 0; i < 3; i++) {
        Bullet b;
        b.x = startPos.x();
        b.y = startPos.y();
        b.angle = i * 120.0f;
        // 将角度转换为 X Y 轴速度 (0度为正上方)
        float rad = qDegreesToRadians(b.angle);
        float speed = 6.0f; // 大弹幕飞行速度
        b.vx = qSin(rad) * speed;
        b.vy = -qCos(rad) * speed;
        m_bigBullets.append(b);
    }

    m_phase = 1;
    m_phaseTimer = 40; // 大弹幕飞一会儿，然后分裂！

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [=]() {
        m_phaseTimer--;

        if (m_phase == 1) { // 第一阶段：大弹幕飞行
            for (int i = 0; i < m_bigBullets.size(); i++) {
                m_bigBullets[i].x += m_bigBullets[i].vx;
                m_bigBullets[i].y += m_bigBullets[i].vy;
            }
            if (m_phaseTimer <= 0) {
                // 触发分裂！大弹幕消失，原地爆出小弹幕
                m_phase = 2;
                m_phaseTimer = 90; // 小弹幕飞 90 帧（1.5秒）

                for (const auto& big : m_bigBullets) {
                    for (int j = 0; j < 8; j++) { // 每个大弹幕爆出8个小弹幕
                        Bullet sb;
                        sb.x = big.x;
                        sb.y = big.y;
                        sb.angle = j * 45.0f; // 360度 / 8 = 每 45 度一个
                        float rad = qDegreesToRadians(sb.angle);
                        float sSpeed = 4.0f; // 小弹幕速度
                        sb.vx = qSin(rad) * sSpeed;
                        sb.vy = -qCos(rad) * sSpeed;
                        m_smallBullets.append(sb);
                    }
                }
                m_bigBullets.clear(); // 清除大弹幕
            }
        }
        else if (m_phase == 2) { // 第二阶段：小弹幕飞行
            for (int i = 0; i < m_smallBullets.size(); i++) {
                m_smallBullets[i].x += m_smallBullets[i].vx;
                m_smallBullets[i].y += m_smallBullets[i].vy;
            }
            if (m_phaseTimer <= 0) {
                this->deleteLater(); // 弹幕消散，彻底销毁
                return;
            }
        }
        update(); // 触发重绘
        });
    m_timer->start(16); // 60 FPS
}

DanmakuWidget::~DanmakuWidget() {}

void DanmakuWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 核心魔法：提取当前 GIF 帧，旋转并绘制
    auto drawBullet = [&](const Bullet& b, QMovie* movie, float size) {
        QImage currentFrame = movie->currentImage();
        if (currentFrame.isNull()) return;

        painter.save(); // 保存当前画笔状态
        painter.translate(b.x, b.y); // 移动到弹幕中心
        painter.rotate(b.angle);     // 按照弹幕飞行方向自动旋转！

        // 绘制提取出的动图帧 (限制了大小，居中绘制)
        painter.drawImage(QRectF(-size / 2, -size / 2, size, size), currentFrame);
        painter.restore(); // 恢复画笔状态
        };

    if (m_phase == 1) {
        for (const auto& b : m_bigBullets) drawBullet(b, m_bigMovie, 80.0f); // 大弹幕大小为 100
    }
    else if (m_phase == 2) {
        for (const auto& b : m_smallBullets) drawBullet(b, m_smallMovie, 30.0f); // 小弹幕大小为 40
    }
}