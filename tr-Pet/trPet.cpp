#include "trPet.h"
#include "BasePet.h" // 引入pet图纸
#include <QPushButton>
#include <QString> // 确保引入了QString，Windows用GBK 编码，而现代代码通常用UTF-8，之前出现了乱码

//加音效
#include <QUrl>
#include <QSoundEffect>

trPet::trPet(QWidget* parent)
    : QMainWindow(parent)
{
    // 1. 设置召唤面板的大小和标题
    this->resize(500, 450);
    // 使用 QString::fromLocal8Bit() 包裹中文字符串，专治VS各种乱码！
    this->setWindowTitle(QString::fromLocal8Bit("泰拉桌宠召唤器"));
    // 2. 在面板上放一个“召唤按钮”

    //召唤彩虹史莱姆
    QPushButton* btnSpawnSlime = new QPushButton(QString::fromLocal8Bit("召唤彩虹史莱姆"), this);
    btnSpawnSlime->setGeometry(50, 60, 200, 50); // 设置按钮的位置和大小
    connect(btnSpawnSlime, &QPushButton::clicked, this, [=]() {
        QString imagePath = "tr-pet_material/Rainbow_Slime.gif";
        BasePet* slime = new BasePet(Role_RainbowSlime, imagePath);
        slime->show();
        }); 
    //召唤世纪之花
    QPushButton* btnPlantera = new QPushButton(QString::fromLocal8Bit("召唤世纪之花 "), this);
    btnPlantera->setGeometry(50, 110, 200, 50);
    connect(btnPlantera, &QPushButton::clicked, this, [=]() {
        // 一阶段的gif
        BasePet* plantera = new BasePet(Role_Plantera, "tr-pet_material/plantera1.gif");
        plantera->show();
        });
    //召唤机械骷髅王
    QPushButton* btnSkeletron = new QPushButton(QString::fromLocal8Bit("召唤骷髅王 "), this);
    btnSkeletron->setGeometry(50, 160, 200, 50); 
    connect(btnSkeletron, &QPushButton::clicked, this, [=]() {
        BasePet* skeletron = new BasePet(Role_Skeletron, "tr-pet_material/skeletron.png");
        skeletron->show();
        });
    //召唤神吞(DoG)
    QPushButton* btnSpawnDoG = new QPushButton(QString::fromLocal8Bit("召唤神明吞噬者(话痨虫(不是))"), this);
    btnSpawnDoG->setGeometry(260, 60, 200, 50); 
    connect(btnSpawnDoG, &QPushButton::clicked, this, [=]() {
        BasePet* dog = new BasePet(Role_DoG, "tr-pet_material/dog.png");
        dog->show();
        // 神吞独有的压迫感出场音效
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl::fromLocalFile("tr-pet_material/Universal_Collapse.wav"));
        effect->setVolume(1.0f); // 1.0是最大音量
        effect->play();
        });
    //召唤至尊女巫，灾厄
    QPushButton* btnSCal = new QPushButton(QString::fromLocal8Bit("召唤至尊女巫，灾厄 SCal " ), this);
    btnSCal->setGeometry(260, 110, 200, 50);
    connect(btnSCal, &QPushButton::clicked, this, [=]() {
        BasePet* scal = new BasePet(Role_SCal, "tr-pet_material/scal.png");
        scal->show();
        });
    //召唤突变体
    QPushButton* btnMutant = new QPushButton(QString::fromLocal8Bit("召唤突变体 "), this);
    btnMutant->setGeometry(260, 160, 200, 50);
    connect(btnMutant, &QPushButton::clicked, this, [=]() {
        BasePet* mutant = new BasePet(Role_Mutant, "tr-pet_material/mutant.gif");
        mutant->show();
        });
    // 召唤护士
    QPushButton* btnNurse = new QPushButton(QString::fromLocal8Bit("召唤护士 "), this);
    btnNurse->setGeometry(50, 230, 95, 50); // 按钮放左边
    connect(btnNurse, &QPushButton::clicked, this, [=]() {
        // 请准备一张 nurse.png 放在D盘
        BasePet* nurse = new BasePet(Role_Nurse, "tr-pet_material/nurse.png");
        nurse->show();
        });
    //召唤军火商
    QPushButton* btnDealer = new QPushButton(QString::fromLocal8Bit("召唤军火商 "), this);
    btnDealer->setGeometry(155, 230, 95, 50); // 按钮放右边
    connect(btnDealer, &QPushButton::clicked, this, [=]() {
        BasePet* dealer = new BasePet(Role_ArmsDealer, "tr-pet_material/dealer.png");
        dealer->show();
        });
    //召唤强盗
    QPushButton* btnbandit = new QPushButton(QString::fromLocal8Bit("召唤强盗 "), this);
    btnbandit->setGeometry(50, 280, 200, 50);
    connect(btnbandit, &QPushButton::clicked, this, [=]() {
        BasePet* bandit = new BasePet(Role_Bandit, "tr-pet_material/bandit.png");
        bandit->show();
        });
    //召唤哥布林
    QPushButton* btnSpawnGoblin = new QPushButton(QString::fromLocal8Bit("召唤哥布林"), this);
    btnSpawnGoblin->setGeometry(50, 330, 200, 50);
    connect(btnSpawnGoblin, &QPushButton::clicked, this, [=]() {
        BasePet* goblin = new BasePet(Role_Goblin, "tr-pet_material/goblin.png");
        goblin->show();
        });
	 //召唤派对女孩
    QPushButton* btnPartyGirl = new QPushButton(QString::fromLocal8Bit("召唤派对女孩 "), this);
    btnPartyGirl->setGeometry(260, 230, 200, 50);
    connect(btnPartyGirl, &QPushButton::clicked, this, [=]() {
        BasePet* partyGirl = new BasePet(Role_PartyGirl, "tr-pet_material/partygirl.png");
        partyGirl->show();
        });
    //召唤税收官
    QPushButton* btnTaxCollector = new QPushButton(QString::fromLocal8Bit("召唤税收官 "), this);
    btnTaxCollector->setGeometry(260, 280, 200, 50); 
    connect(btnTaxCollector, &QPushButton::clicked, this, [=]() {
        BasePet* taxCollector = new BasePet(Role_TaxCollector, "tr-pet_material/taxcollector.png");
        taxCollector->show();
        });
	//召唤操作员
    QPushButton* btnOperator = new QPushButton(QString::fromLocal8Bit("召唤操作员 "), this);
    btnOperator->setGeometry(260, 330, 200, 50);
    connect(btnOperator, &QPushButton::clicked, this, [=]() {
        BasePet* op = new BasePet(Role_Operator, "tr-pet_material/operator.png");
        op->show();
        });
}

trPet::~trPet()
{
}