#include "mainwindow.h"

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QIcon>
#include <QDir>
#include <QKeyEvent>

#include <QtMultimedia/QAudioBufferOutput>
#include <QtMultimedia/QAudioOutput>

#include "audioanalyzer.h"
#include "spectrumwidget.h"
#include "lyricscontroller.h"
#include "karaokewidget.h"

QString style_pushBtn1 = QString::fromUtf8(
    "QPushButton{"
    "height: 30px;"
    "width: 140px;"
    "background-color: white;"
    "}"
    "QPushButton:hover{"
    "background-color: #f0f0f0;"
    "border: 1px solid blue;"
    "}"
    "QPushButton:pressed{"
    "background-color: #dcdcdc;"
    "}"
    );

QString style_pushBtn2 = QString::fromUtf8(
    "QPushButton{"
    "background-color: white;"
    "}"
    "QPushButton:hover{"
    "background-color: #f0f0f0;"
    "border: 1px solid blue;"
    "}"
    "QPushButton:pressed{"
    "background-color: #dcdcdc;"
    "}"
    "QPushButton:checked{"
    "background-color: #c0c0c0;"
    "border: 1px solid blue;"
    "}"
    );

QString style_groupBox = QString::fromUtf8(
    "QGroupBox{"
    "border: 1px solid gray;"
    "background-color:transparent;"
    "}"
    );

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setUpUi(this);

    list_Music->installEventFilter(this);
    list_Music->setDragEnabled(true);
    list_Music->setDragDropMode(QAbstractItemView::InternalMove);

    player = new QMediaPlayer(this);

    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(sld_Sound->value() / 100.0);

    audioBufferOutput = new QAudioBufferOutput(this);
    player->setAudioBufferOutput(audioBufferOutput);

    audioAnalyzer = new AudioAnalyzer(this);
    lyricsController = new LyricsController(this);

    karaokeWidget->setLyricsController(lyricsController);

    connect(audioBufferOutput, &QAudioBufferOutput::audioBufferReceived,
            audioAnalyzer, &AudioAnalyzer::processBuffer);

    connect(audioAnalyzer, &AudioAnalyzer::spectrumReady,
            spectrumWidget, &SpectrumWidget::setSpectrumData);

    positionTime = "00:00";
    durationTime = "00:00";
    lab_Duration->setText(positionTime + "/" + durationTime);

    btn_Pause->setEnabled(false);
    btn_Stop->setEnabled(false);

    connect(btn_Add, &QPushButton::clicked, this, &MainWindow::on_btn_Add_clicked);
    connect(btn_Remove, &QPushButton::clicked, this, &MainWindow::on_btn_Remove_clicked);
    connect(btn_Clear, &QPushButton::clicked, this, &MainWindow::on_btn_Clear_clicked);
    connect(btn_Exit, &QPushButton::clicked, this, &QWidget::close);
    connect(list_Music, &QListWidget::doubleClicked, this, &MainWindow::on_list_Music_doubleClicked);
    connect(btn_Last, &QPushButton::clicked, this, &MainWindow::on_btn_Last_clicked);
    connect(btn_Next, &QPushButton::clicked, this, &MainWindow::on_btn_Next_clicked);
    connect(btn_Play, &QPushButton::clicked, this, &MainWindow::on_btn_Play_clicked);
    connect(btn_Pause, &QPushButton::clicked, this, &MainWindow::on_btn_Pause_clicked);
    connect(btn_Stop, &QPushButton::clicked, this, &MainWindow::on_btn_Stop_clicked);
    connect(spn_Rate, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::on_doubleSpinBox_valueChanged);
    connect(btn_Loop, &QPushButton::clicked, this, &MainWindow::on_btn_Loop_clicked);
    connect(sld_Position, &QSlider::sliderMoved, this, &MainWindow::on_sld_Position_valueChanged);
    connect(btn_Sound, &QPushButton::clicked, this, &MainWindow::on_btn_Sound_clicked);
    connect(sld_Sound, &QSlider::valueChanged, this, &MainWindow::on_sld_Sound_valueChanged);

    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::do_positionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::do_durationChanged);
    connect(player, &QMediaPlayer::sourceChanged, this, &MainWindow::do_sourceChanged);
    connect(player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::do_stateChanged);
    connect(player, &QMediaPlayer::metaDataChanged, this, &MainWindow::do_metaDataChanged);
}

void MainWindow::setUpUi(QWidget *parent)
{
    Q_UNUSED(parent);

    resize(800, 600);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWindowTitle(QString::fromUtf8("音频播放器"));
    setWindowIcon(QIcon(":/images/Music.ico"));

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QGroupBox *groupBox1 = new QGroupBox(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *layout1 = new QHBoxLayout;

    btn_Add = new QPushButton(groupBox1);
    btn_Add->setIcon(QIcon(":/images/images/316.bmp"));
    btn_Add->setIconSize(QSize(24, 24));
    btn_Add->setText(QString::fromUtf8("添加"));
    btn_Add->setStyleSheet(style_pushBtn1);

    btn_Remove = new QPushButton(groupBox1);
    btn_Remove->setIcon(QIcon(":/images/images/318.bmp"));
    btn_Remove->setIconSize(QSize(24, 24));
    btn_Remove->setText(QString::fromUtf8("移除"));
    btn_Remove->setStyleSheet(style_pushBtn1);

    btn_Clear = new QPushButton(groupBox1);
    btn_Clear->setIcon(QIcon(":/images/images/214.bmp"));
    btn_Clear->setIconSize(QSize(24, 24));
    btn_Clear->setText(QString::fromUtf8("清空"));
    btn_Clear->setStyleSheet(style_pushBtn1);

    btn_Exit = new QPushButton(groupBox1);
    btn_Exit->setIcon(QIcon(":/images/images/214.bmp"));
    btn_Exit->setIconSize(QSize(24, 24));
    btn_Exit->setText(QString::fromUtf8("退出"));
    btn_Exit->setStyleSheet(style_pushBtn1);

    layout1->addWidget(btn_Add);
    layout1->addWidget(btn_Remove);
    layout1->addWidget(btn_Clear);
    layout1->addWidget(btn_Exit);
    layout1->setContentsMargins(10, 5, 10, 5);
    layout1->setSpacing(10);
    groupBox1->setLayout(layout1);
    groupBox1->setStyleSheet(style_groupBox);

    QGroupBox *groupBox2 = new QGroupBox(centralWidget);
    QHBoxLayout *layout2 = new QHBoxLayout;
    QVBoxLayout *rightLayout = new QVBoxLayout;

    list_Music = new QListWidget;
    list_Music->setBackgroundRole(QPalette::Light);

    pic_Music = new QLabel;
    pic_Music->setText(QString::fromUtf8("pic"));
    pic_Music->setAlignment(Qt::AlignCenter);
    pic_Music->setBackgroundRole(QPalette::Light);

    spectrumWidget = new SpectrumWidget;
    karaokeWidget = new KaraokeWidget;

    rightLayout->addWidget(pic_Music, 3);
    rightLayout->addWidget(spectrumWidget, 2);
    rightLayout->addWidget(karaokeWidget, 2);

    layout2->addWidget(list_Music, 3);
    layout2->addLayout(rightLayout, 2);
    layout2->setContentsMargins(10, 5, 10, 5);
    layout2->setSpacing(6);

    groupBox2->setLayout(layout2);
    groupBox2->setStyleSheet(style_groupBox);
    groupBox2->resize(780, 300);

    QGroupBox *groupBox3 = new QGroupBox(centralWidget);
    groupBox3->setStyleSheet(style_groupBox);

    QHBoxLayout *layout3 = new QHBoxLayout(groupBox3);
    layout3->setContentsMargins(10, 5, 10, 5);
    layout3->setSpacing(6);

    auto initSmallBtn = [&](QPushButton *btn, const QString &iconPath)
    {
        btn->setStyleSheet(style_pushBtn2);
        btn->setFixedSize(36, 30);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(22, 22));
        btn->setContentsMargins(0, 0, 0, 0);
    };

    btn_Play = new QPushButton(groupBox3);
    btn_Pause = new QPushButton(groupBox3);
    btn_Stop = new QPushButton(groupBox3);
    btn_Last = new QPushButton(groupBox3);
    btn_Next = new QPushButton(groupBox3);

    initSmallBtn(btn_Play,  ":/images/images/620.bmp");
    initSmallBtn(btn_Pause, ":/images/images/622.bmp");
    initSmallBtn(btn_Stop,  ":/images/images/624.bmp");
    initSmallBtn(btn_Last,  ":/images/images/616.bmp");
    initSmallBtn(btn_Next,  ":/images/images/630.bmp");

    layout3->addWidget(btn_Play);
    layout3->addWidget(btn_Pause);
    layout3->addWidget(btn_Stop);
    layout3->addWidget(btn_Last);
    layout3->addWidget(btn_Next);
    layout3->addSpacing(18);

    spn_Rate = new QDoubleSpinBox(groupBox3);
    spn_Rate->setDecimals(1);
    spn_Rate->setRange(0.5, 3.0);
    spn_Rate->setValue(1.0);
    spn_Rate->setSuffix(QString::fromUtf8("倍速"));
    spn_Rate->setSingleStep(0.1);
    spn_Rate->setFixedHeight(28);
    spn_Rate->setMinimumWidth(90);
    spn_Rate->setAlignment(Qt::AlignCenter);
    spn_Rate->setButtonSymbols(QAbstractSpinBox::UpDownArrows);

    spn_Rate->setStyleSheet(R"(
    QDoubleSpinBox {
        padding-right: 10px;
    }
    QDoubleSpinBox::up-button {
        subcontrol-origin: border;
        subcontrol-position: top right;
        width: 16px;
        height: 13px;
    }
    QDoubleSpinBox::down-button {
        subcontrol-origin: border;
        subcontrol-position: bottom right;
        width: 16px;
        height: 13px;
    }
)");

    btn_Loop = new QPushButton(groupBox3);
    btn_Loop->setStyleSheet(style_pushBtn2);
    btn_Loop->setCheckable(true);
    btn_Loop->setChecked(true);
    btn_Loop->setFixedSize(76, 30);
    btn_Loop->setIcon(QIcon(":/images/images/refresh16.png"));
    btn_Loop->setIconSize(QSize(16, 16));
    btn_Loop->setText(QString::fromUtf8("循环"));

    btn_Sound = new QPushButton(groupBox3);
    btn_Sound->setStyleSheet(style_pushBtn2);
    btn_Sound->setFixedSize(36, 30);
    btn_Sound->setIcon(QIcon(":/images/images/volumn.bmp"));
    btn_Sound->setIconSize(QSize(20, 20));

    sld_Sound = new QSlider(Qt::Horizontal, groupBox3);
    sld_Sound->setFixedWidth(200);
    sld_Sound->setRange(0, 100);
    sld_Sound->setTickInterval(1);
    sld_Sound->setValue(100);

    layout3->addWidget(spn_Rate);
    layout3->addWidget(btn_Loop);
    layout3->addStretch();
    layout3->addWidget(btn_Sound);
    layout3->addWidget(sld_Sound);

    layout->addWidget(groupBox1);
    layout->addWidget(groupBox2);
    layout->addWidget(groupBox3);
    centralWidget->setLayout(layout);

    lab_Name = new QLabel(QString::fromUtf8("无曲目"), this);
    lab_Name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lab_Name->setMinimumHeight(20);
    lab_Name->setMinimumWidth(200);

    sld_Position = new QSlider(this);
    sld_Position->setRange(0, 0);
    sld_Position->setTickInterval(1);
    sld_Position->setOrientation(Qt::Horizontal);
    sld_Position->setValue(0);
    sld_Position->setFixedWidth(400);

    lab_Duration = new QLabel(QString::fromUtf8("00:00/00:00"), this);
    lab_Duration->setMinimumWidth(170);
    lab_Duration->setMinimumHeight(20);
    lab_Duration->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    statusBar()->addWidget(lab_Name);
    statusBar()->addWidget(sld_Position);
    statusBar()->addWidget(lab_Duration);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == list_Music && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            QListWidgetItem *item = list_Music->takeItem(list_Music->currentRow());
            delete item;

            if (list_Music->count() == 0) {
                player->stop();
                player->setSource(QUrl());
                pic_Music->clear();
                lab_Name->setText(QString::fromUtf8("无曲目"));
                positionTime = "00:00";
                durationTime = "00:00";
                sld_Position->setRange(0, 0);
                sld_Position->setSliderPosition(0);
                lab_Duration->setText(positionTime + "/" + durationTime);

                lyricsController->clear();
                audioAnalyzer->reset();
                spectrumWidget->clearData();
            }
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_btn_Add_clicked()
{
    QString curPath = QDir::homePath();
    QString title = QString::fromUtf8("选择音频文件");
    QString filter = QString::fromUtf8("音频文件(*.mp3 *.wav *.wma);;所有文件(*.*)");
    QStringList fileList = QFileDialog::getOpenFileNames(this, title, curPath, filter);

    if (fileList.count() < 1)
        return;

    for (int i = 0; i < fileList.count(); ++i) {
        QString fileName = fileList.at(i);
        QFileInfo fileInfo(fileName);
        QListWidgetItem *fileItem = new QListWidgetItem(fileInfo.fileName());
        fileItem->setIcon(QIcon(":/images/images/musicFile.png"));
        fileItem->setData(Qt::UserRole, QUrl::fromLocalFile(fileName));
        list_Music->addItem(fileItem);
    }

    if (player->playbackState() != QMediaPlayer::PlayingState) {
        list_Music->setCurrentRow(0);
        QUrl source = getUrlFromItem(list_Music->currentItem());
        if (!source.isEmpty())
            player->setSource(source);
    }

    player->play();
}

QUrl MainWindow::getUrlFromItem(QListWidgetItem *item)
{
    if (item == nullptr)
        return QUrl();

    QVariant itemData = item->data(Qt::UserRole);
    return itemData.toUrl();
}

void MainWindow::on_btn_Remove_clicked()
{
    int currentRow = list_Music->currentRow();
    if (currentRow < 0)
        return;

    QListWidgetItem *item = list_Music->takeItem(currentRow);
    delete item;

    if (list_Music->count() == 0) {
        player->stop();
        player->setSource(QUrl());
        pic_Music->clear();
        lab_Name->setText(QString::fromUtf8("无曲目"));
        positionTime = "00:00";
        durationTime = "00:00";
        sld_Position->setRange(0, 0);
        sld_Position->setSliderPosition(0);
        lab_Duration->setText(positionTime + "/" + durationTime);

        lyricsController->clear();
        audioAnalyzer->reset();
        spectrumWidget->clearData();
        return;
    }

    if (currentRow >= list_Music->count())
        currentRow = list_Music->count() - 1;

    list_Music->setCurrentRow(currentRow);
}

void MainWindow::on_btn_Clear_clicked()
{
    list_Music->clear();
    player->stop();
    player->setSource(QUrl());
    pic_Music->clear();
    lab_Name->setText(QString::fromUtf8("无曲目"));
    positionTime = "00:00";
    durationTime = "00:00";
    sld_Position->setRange(0, 0);
    sld_Position->setSliderPosition(0);
    lab_Duration->setText(positionTime + "/" + durationTime);

    lyricsController->clear();
    audioAnalyzer->reset();
    spectrumWidget->clearData();
}

void MainWindow::on_list_Music_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    QUrl source = getUrlFromItem(list_Music->currentItem());
    if (source.isEmpty())
        return;

    player->setSource(source);
    player->play();
    loopPlay = btn_Loop->isChecked();
}

void MainWindow::on_btn_Last_clicked()
{
    int count = list_Music->count();
    if (count <= 0)
        return;

    int currentRow = list_Music->currentRow();
    if (currentRow < 0)
        currentRow = 0;
    else
        currentRow--;

    currentRow = currentRow < 0 ? 0 : currentRow;
    list_Music->setCurrentRow(currentRow);

    QUrl source = getUrlFromItem(list_Music->currentItem());
    if (source.isEmpty())
        return;

    loopPlay = false;
    player->setSource(source);
    player->play();
    loopPlay = btn_Loop->isChecked();
}

void MainWindow::on_btn_Next_clicked()
{
    int count = list_Music->count();
    if (count <= 0)
        return;

    int currentRow = list_Music->currentRow();
    if (currentRow < 0)
        currentRow = 0;
    else
        currentRow++;

    currentRow = currentRow >= count ? 0 : currentRow;
    list_Music->setCurrentRow(currentRow);

    QUrl source = getUrlFromItem(list_Music->currentItem());
    if (source.isEmpty())
        return;

    loopPlay = false;
    player->setSource(source);
    player->play();
    loopPlay = btn_Loop->isChecked();
}

void MainWindow::do_sourceChanged(const QUrl &source)
{
    if (source.isEmpty()) {
        lab_Name->setText(QString::fromUtf8("无曲目"));
        lyricsController->clear();
        audioAnalyzer->reset();
        spectrumWidget->clearData();
        pic_Music->clear();
        return;
    }

    lab_Name->setText(source.fileName());

    if (source.isLocalFile())
        lyricsController->loadFromAudioFile(source.toLocalFile());
    else
        lyricsController->clear();

    audioAnalyzer->reset();
    spectrumWidget->clearData();
}

void MainWindow::do_metaDataChanged()
{
    QMediaMetaData metaData = player->metaData();
    QVariant metaImg = metaData.value(QMediaMetaData::ThumbnailImage);

    if (!metaImg.isNull() && metaImg.isValid()) {
        QImage img = metaImg.value<QImage>();
        QPixmap musicPixmap = QPixmap::fromImage(img);

        if (pic_Music->width() < musicPixmap.width() ||
            pic_Music->height() < musicPixmap.height()) {
            musicPixmap = musicPixmap.scaled(QSize(pic_Music->width(), pic_Music->height()),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        }

        pic_Music->setPixmap(musicPixmap);
    } else {
        pic_Music->clear();
    }
}

void MainWindow::do_durationChanged(qint64 dur)
{
    if (dur <= 0) {
        sld_Position->setRange(0, 0);
        durationTime = "00:00";
        lab_Duration->setText(positionTime + "/" + durationTime);
        lyricsController->setDuration(0);
        return;
    }

    sld_Position->setRange(0, int(dur));

    int secs = static_cast<int>(dur / 1000);
    int mins = secs / 60;
    secs = secs % 60;
    durationTime = QString("%1:%2")
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));

    lab_Duration->setText(positionTime + "/" + durationTime);
    lyricsController->setDuration(dur);
}

void MainWindow::do_positionChanged(qint64 pos)
{
    if (!sld_Position->isSliderDown())
        sld_Position->setSliderPosition(int(pos));

    int secs = static_cast<int>(pos / 1000);
    int mins = secs / 60;
    secs = secs % 60;
    positionTime = QString("%1:%2")
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));

    lab_Duration->setText(positionTime + "/" + durationTime);
    lyricsController->setPosition(pos);
}

void MainWindow::do_stateChanged(QMediaPlayer::PlaybackState state)
{
    btn_Play->setEnabled(state != QMediaPlayer::PlayingState);
    btn_Pause->setEnabled(state == QMediaPlayer::PlayingState);
    btn_Stop->setEnabled(state == QMediaPlayer::PlayingState);

    if (loopPlay &&
        state == QMediaPlayer::StoppedState &&
        player->mediaStatus() == QMediaPlayer::EndOfMedia) {
        int count = list_Music->count();
        if (count <= 0)
            return;

        int currentRow = list_Music->currentRow();
        currentRow++;
        currentRow = currentRow >= count ? 0 : currentRow;
        list_Music->setCurrentRow(currentRow);

        QUrl source = getUrlFromItem(list_Music->currentItem());
        if (source.isEmpty())
            return;

        player->setSource(source);
        player->play();
    }
}

void MainWindow::on_btn_Play_clicked()
{
    if (list_Music->count() <= 0)
        return;

    if (list_Music->currentRow() < 0)
        list_Music->setCurrentRow(0);

    QUrl source = getUrlFromItem(list_Music->currentItem());
    if (source.isEmpty())
        return;

    if (player->source() != source)
        player->setSource(source);

    loopPlay = btn_Loop->isChecked();
    player->play();
}

void MainWindow::on_btn_Pause_clicked()
{
    player->pause();
}

void MainWindow::on_btn_Stop_clicked()
{
    player->stop();
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    player->setPlaybackRate(arg1);
}

void MainWindow::on_btn_Loop_clicked(bool checked)
{
    loopPlay = checked;
}

void MainWindow::on_sld_Position_valueChanged(int value)
{
    player->setPosition(value);
}

void MainWindow::on_btn_Sound_clicked()
{
    bool mute = audioOutput->isMuted();
    audioOutput->setMuted(!mute);

    if (mute)
        btn_Sound->setIcon(QIcon(":/images/images/volumn.bmp"));
    else
        btn_Sound->setIcon(QIcon(":/images/images/mute.bmp"));
}

void MainWindow::on_sld_Sound_valueChanged(int value)
{
    audioOutput->setVolume(value / 100.0);
}

MainWindow::~MainWindow() = default;