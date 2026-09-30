#include "mainwindow.h"

#include "audioanalyzer.h"
#include "karaokewidget.h"
#include "lyricscontroller.h"
#include "spectrumwidget.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QAudio>
#include <QAudioBufferOutput>
#include <QAudioOutput>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QListWidget>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QScreen>
#include <QSlider>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <limits>

namespace {

constexpr int UrlRole = Qt::UserRole;
constexpr int ArtistRole = Qt::UserRole + 1;
constexpr int PlayingRole = Qt::UserRole + 2;
constexpr int PlaybackStateRole = Qt::UserRole + 3;

const QColor kAccent(250, 45, 85);
const QColor kPrimaryText(29, 29, 31);
const QColor kSecondaryText(110, 110, 115);

enum class AppIcon {
    Add,
    Remove,
    Previous,
    Play,
    Pause,
    Next,
    Repeat,
    Volume,
    Mute,
    Music
};

QIcon makeIcon(AppIcon type, const QColor &color = kPrimaryText)
{
    QIcon icon;
    for (int scale = 1; scale <= 3; ++scale) {
        QPixmap pixmap(22 * scale, 22 * scale);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

    switch (type) {
    case AppIcon::Add:
        painter.drawLine(QPointF(5.0, 11.0), QPointF(17.0, 11.0));
        painter.drawLine(QPointF(11.0, 5.0), QPointF(11.0, 17.0));
        break;
    case AppIcon::Remove:
        painter.drawLine(QPointF(5.0, 11.0), QPointF(17.0, 11.0));
        break;
    case AppIcon::Previous: {
        painter.drawLine(QPointF(6.0, 5.0), QPointF(6.0, 17.0));
        QPainterPath path;
        path.moveTo(16.5, 5.2);
        path.lineTo(7.5, 11.0);
        path.lineTo(16.5, 16.8);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
        break;
    }
    case AppIcon::Play: {
        QPainterPath path;
        path.moveTo(7.0, 4.5);
        path.lineTo(17.0, 11.0);
        path.lineTo(7.0, 17.5);
        path.closeSubpath();
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPath(path);
        break;
    }
    case AppIcon::Pause:
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(6.0, 4.5, 3.5, 13.0), 1.0, 1.0);
        painter.drawRoundedRect(QRectF(12.5, 4.5, 3.5, 13.0), 1.0, 1.0);
        break;
    case AppIcon::Next: {
        painter.drawLine(QPointF(16.0, 5.0), QPointF(16.0, 17.0));
        QPainterPath path;
        path.moveTo(5.5, 5.2);
        path.lineTo(14.5, 11.0);
        path.lineTo(5.5, 16.8);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
        break;
    }
    case AppIcon::Repeat:
        painter.drawArc(QRectF(4.0, 5.0, 14.0, 9.0), 35 * 16, 140 * 16);
        painter.drawArc(QRectF(4.0, 8.0, 14.0, 9.0), 215 * 16, 140 * 16);
        painter.drawLine(QPointF(4.7, 7.2), QPointF(4.0, 11.0));
        painter.drawLine(QPointF(4.7, 7.2), QPointF(8.0, 8.0));
        painter.drawLine(QPointF(17.3, 14.8), QPointF(18.0, 11.0));
        painter.drawLine(QPointF(17.3, 14.8), QPointF(14.0, 14.0));
        break;
    case AppIcon::Volume:
    case AppIcon::Mute: {
        QPainterPath speaker;
        speaker.moveTo(4.0, 9.0);
        speaker.lineTo(7.2, 9.0);
        speaker.lineTo(11.5, 5.5);
        speaker.lineTo(11.5, 16.5);
        speaker.lineTo(7.2, 13.0);
        speaker.lineTo(4.0, 13.0);
        speaker.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(speaker);
        painter.setBrush(Qt::NoBrush);
        if (type == AppIcon::Volume) {
            painter.drawArc(QRectF(9.0, 7.0, 7.0, 8.0), -55 * 16, 110 * 16);
            painter.drawArc(QRectF(8.5, 4.5, 11.0, 13.0), -55 * 16, 110 * 16);
        } else {
            painter.drawLine(QPointF(14.5, 8.0), QPointF(19.0, 14.0));
            painter.drawLine(QPointF(19.0, 8.0), QPointF(14.5, 14.0));
        }
        break;
    }
    case AppIcon::Music:
        painter.setPen(QPen(color, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(10.0, 5.0), QPointF(10.0, 15.0));
        painter.drawLine(QPointF(10.0, 5.0), QPointF(17.0, 3.5));
        painter.drawLine(QPointF(17.0, 3.5), QPointF(17.0, 13.0));
        painter.setBrush(color);
        painter.drawEllipse(QRectF(5.2, 13.0, 5.0, 3.8));
        painter.drawEllipse(QRectF(12.2, 11.0, 5.0, 3.8));
        break;
    }

        icon.addPixmap(pixmap);
    }
    return icon;
}

QString normalizedPathKey(const QString &filePath)
{
    QFileInfo info(filePath);
    QString normalized = info.canonicalFilePath();
    if (normalized.isEmpty())
        normalized = info.absoluteFilePath();
    normalized = QDir::cleanPath(normalized);
#ifdef Q_OS_WIN
    return normalized.toCaseFolded();
#else
    return normalized;
#endif
}

bool isSupportedAudioFile(const QFileInfo &info)
{
    static const QSet<QString> suffixes = {
        QStringLiteral("mp3"), QStringLiteral("m4a"), QStringLiteral("aac"),
        QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("ogg"),
        QStringLiteral("opus"), QStringLiteral("wma"), QStringLiteral("mp4"),
        QStringLiteral("aif"), QStringLiteral("aiff"), QStringLiteral("caf")
    };
    return info.isFile() && suffixes.contains(info.suffix().toLower());
}

class TrackDelegate final : public QStyledItemDelegate
{
public:
    explicit TrackDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(260, 62);
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const QRect rowRect = option.rect.adjusted(3, 2, -3, -2);
        if (option.state.testFlag(QStyle::State_Selected)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 237, 242));
            painter->drawRoundedRect(rowRect, 9, 9);
        } else if (option.state.testFlag(QStyle::State_MouseOver)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(247, 247, 250));
            painter->drawRoundedRect(rowRect, 9, 9);
        }
        if (option.state.testFlag(QStyle::State_HasFocus)) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(kAccent, 1.2));
            painter->drawRoundedRect(QRectF(rowRect).adjusted(0.7, 0.7, -0.7, -0.7), 9, 9);
        }

        const bool isPlaying = index.data(PlayingRole).toBool();
        const auto playbackState =
            static_cast<QMediaPlayer::PlaybackState>(index.data(PlaybackStateRole).toInt());

        const QRect markerRect(rowRect.left() + 10, rowRect.top(), 28, rowRect.height());
        if (isPlaying) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(kAccent);
            if (playbackState == QMediaPlayer::PlayingState) {
                const int centerY = markerRect.center().y();
                painter->drawRoundedRect(QRectF(markerRect.left() + 5, centerY - 5, 3, 10), 1, 1);
                painter->drawRoundedRect(QRectF(markerRect.left() + 11, centerY - 8, 3, 16), 1, 1);
                painter->drawRoundedRect(QRectF(markerRect.left() + 17, centerY - 3, 3, 6), 1, 1);
            } else {
                QPainterPath triangle;
                triangle.moveTo(markerRect.left() + 8, markerRect.center().y() - 7);
                triangle.lineTo(markerRect.left() + 20, markerRect.center().y());
                triangle.lineTo(markerRect.left() + 8, markerRect.center().y() + 7);
                triangle.closeSubpath();
                painter->drawPath(triangle);
            }
        } else {
            QFont numberFont = option.font;
            if (numberFont.pointSizeF() > 0)
                numberFont.setPointSizeF(std::max(8.0, numberFont.pointSizeF() - 0.5));
            else if (numberFont.pixelSize() > 0)
                numberFont.setPixelSize(std::max(9, numberFont.pixelSize() - 1));
            painter->setFont(numberFont);
            painter->setPen(QColor(142, 142, 147));
            painter->drawText(markerRect, Qt::AlignCenter, QString::number(index.row() + 1));
        }

        const int textLeft = markerRect.right() + 8;
        const int textRight = rowRect.right() - 12;
        const QRect titleRect(textLeft, rowRect.top() + 10,
                              std::max(0, textRight - textLeft), 21);
        const QRect artistRect(textLeft, titleRect.bottom() + 2,
                               std::max(0, textRight - textLeft), 18);

        QFont titleFont = option.font;
        titleFont.setWeight(isPlaying ? QFont::DemiBold : QFont::Medium);
        painter->setFont(titleFont);
        painter->setPen(isPlaying ? kAccent : kPrimaryText);
        const QString title =
            QFontMetrics(titleFont).elidedText(index.data(Qt::DisplayRole).toString(),
                                               Qt::ElideRight, titleRect.width());
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

        QFont secondaryFont = option.font;
        if (secondaryFont.pointSizeF() > 0)
            secondaryFont.setPointSizeF(std::max(8.0, secondaryFont.pointSizeF() - 1.0));
        else if (secondaryFont.pixelSize() > 0)
            secondaryFont.setPixelSize(std::max(9, secondaryFont.pixelSize() - 1));
        painter->setFont(secondaryFont);
        painter->setPen(kSecondaryText);
        const QString artist =
            QFontMetrics(secondaryFont).elidedText(index.data(ArtistRole).toString(),
                                                   Qt::ElideRight, artistRect.width());
        painter->drawText(artistRect, Qt::AlignLeft | Qt::AlignVCenter, artist);

        painter->restore();
    }
};

} // namespace

class MusicListWidget final : public QListWidget
{
public:
    explicit MusicListWidget(QWidget *parent = nullptr)
        : QListWidget(parent)
    {
        setAcceptDrops(true);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDefaultDropAction(Qt::MoveAction);
        setDropIndicatorShown(true);
    }

    std::function<void(const QStringList &)> filesDropped;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->source() != this && event->mimeData()->hasUrls()) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
            return;
        }
        QListWidget::dragEnterEvent(event);
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (event->source() != this && event->mimeData()->hasUrls()) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
            return;
        }
        QListWidget::dragMoveEvent(event);
    }

    void dropEvent(QDropEvent *event) override
    {
        if (event->source() != this && event->mimeData()->hasUrls()) {
            QStringList paths;
            for (const QUrl &url : event->mimeData()->urls()) {
                if (url.isLocalFile())
                    paths.append(url.toLocalFile());
            }
            if (!paths.isEmpty() && filesDropped)
                filesDropped(paths);
            event->setDropAction(Qt::CopyAction);
            event->accept();
            return;
        }
        QListWidget::dropEvent(event);
    }
};

class ArtworkWidget final : public QWidget
{
public:
    explicit ArtworkWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(170, 170);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    QSize sizeHint() const override
    {
        return QSize(192, 192);
    }

    void setArtwork(const QImage &image)
    {
        m_image = image;
        update();
    }

    void clearArtwork()
    {
        m_image = QImage();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF target = QRectF(rect()).adjusted(3, 3, -3, -3);
        QPainterPath rounded;
        rounded.addRoundedRect(target, 17, 17);

        painter.setPen(QPen(QColor(0, 0, 0, 22), 1));
        painter.setBrush(QColor(255, 255, 255));
        painter.drawRoundedRect(target.translated(0, 1.5), 17, 17);

        painter.save();
        painter.setClipPath(rounded);
        if (!m_image.isNull()) {
            QImage scaled = m_image.scaled(target.size().toSize(),
                                           Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);
            const QPointF topLeft(target.center().x() - scaled.width() / 2.0,
                                  target.center().y() - scaled.height() / 2.0);
            painter.drawImage(topLeft, scaled);
        } else {
            QLinearGradient gradient(target.topLeft(), target.bottomRight());
            gradient.setColorAt(0.0, QColor(255, 214, 225));
            gradient.setColorAt(0.52, QColor(245, 230, 255));
            gradient.setColorAt(1.0, QColor(211, 230, 255));
            painter.fillPath(rounded, gradient);

            QFont noteFont = font();
            noteFont.setPixelSize(std::max(42, int(target.height() * 0.3)));
            noteFont.setWeight(QFont::DemiBold);
            painter.setFont(noteFont);
            painter.setPen(QColor(255, 255, 255, 225));
            painter.drawText(target, Qt::AlignCenter, QStringLiteral("♪"));
        }
        painter.restore();
    }

private:
    QImage m_image;
};

class ElidedLabel final : public QLabel
{
public:
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr)
        : QLabel(text, parent)
    {
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setMinimumWidth(0);
        setToolTip(text);
    }

    void setText(const QString &text)
    {
        QLabel::setText(text);
        setToolTip(text);
        update();
    }

    QSize sizeHint() const override
    {
        QSize hint = QLabel::sizeHint();
        hint.setWidth(std::min(hint.width(), 220));
        return hint;
    }

    QSize minimumSizeHint() const override
    {
        QSize hint = QLabel::minimumSizeHint();
        hint.setWidth(0);
        return hint;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setFont(font());
        painter.setPen(palette().color(foregroundRole()));
        const QString displayText =
            fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width());
        painter.drawText(contentsRect(), int(alignment()) | Qt::TextSingleLine, displayText);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setUpUi();

    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    setPlayerVolume(sld_Sound->value());

    audioBufferOutput = new QAudioBufferOutput(this);
    player->setAudioBufferOutput(audioBufferOutput);

    audioAnalyzer = new AudioAnalyzer(this);
    lyricsController = new LyricsController(this);
    karaokeWidget->setLyricsController(lyricsController);

    connect(audioBufferOutput, &QAudioBufferOutput::audioBufferReceived,
            audioAnalyzer, &AudioAnalyzer::processBuffer);
    connect(audioAnalyzer, &AudioAnalyzer::spectrumReady,
            spectrumWidget, &SpectrumWidget::setSpectrumData);

    connect(btn_Add, &QPushButton::clicked, this, &MainWindow::on_btn_Add_clicked);
    connect(btn_Remove, &QPushButton::clicked, this, &MainWindow::on_btn_Remove_clicked);
    connect(btn_Clear, &QPushButton::clicked, this, &MainWindow::on_btn_Clear_clicked);
    connect(list_Music, &QListWidget::doubleClicked,
            this, &MainWindow::on_list_Music_doubleClicked);
    connect(list_Music, &QListWidget::itemSelectionChanged,
            this, &MainWindow::updateLibraryCount);
    connect(list_Music->model(), &QAbstractItemModel::rowsMoved,
            this, [this] { updatePlayingIndicator(); });
    connect(btn_Last, &QPushButton::clicked, this, &MainWindow::on_btn_Last_clicked);
    connect(btn_Next, &QPushButton::clicked, this, &MainWindow::on_btn_Next_clicked);
    connect(btn_Play, &QPushButton::clicked, this, &MainWindow::on_btn_Play_clicked);
    connect(spn_Rate, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::on_doubleSpinBox_valueChanged);
    connect(btn_Loop, &QPushButton::toggled, this, &MainWindow::on_btn_Loop_clicked);
    connect(sld_Position, &QSlider::sliderMoved,
            this, &MainWindow::on_sld_Position_valueChanged);
    connect(sld_Position, &QSlider::actionTriggered, this,
            [this](int action) {
                if (action != QAbstractSlider::SliderMove)
                    on_sld_Position_valueChanged(sld_Position->sliderPosition());
            });
    connect(btn_Sound, &QPushButton::clicked, this, &MainWindow::on_btn_Sound_clicked);
    connect(sld_Sound, &QSlider::valueChanged,
            this, &MainWindow::on_sld_Sound_valueChanged);
    connect(searchEdit, &QLineEdit::textChanged,
            this, &MainWindow::on_searchTextChanged);

    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::do_positionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::do_durationChanged);
    connect(player, &QMediaPlayer::sourceChanged, this, &MainWindow::do_sourceChanged);
    connect(player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::do_stateChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged,
            this, &MainWindow::do_mediaStatusChanged);
    connect(player, &QMediaPlayer::metaDataChanged, this, &MainWindow::do_metaDataChanged);
    connect(player, &QMediaPlayer::errorOccurred, this, &MainWindow::do_errorOccurred);

    setUpShortcuts();
    qApp->installEventFilter(this);
    resetPlayback();
    updateLibraryCount();
    updatePlaybackButton(QMediaPlayer::StoppedState);
    updateVolumeButton();
}

void MainWindow::setUpUi()
{
    setMinimumSize(820, 430);
    QSize initialSize(1120, 720);
    if (QScreen *targetScreen = screen()) {
        const QSize available = targetScreen->availableGeometry().size();
        initialSize.setWidth(std::max(minimumWidth(),
                                      std::min(initialSize.width(), available.width() - 24)));
        initialSize.setHeight(std::max(minimumHeight(),
                                       std::min(initialSize.height(), available.height() - 56)));
    }
    resize(initialSize);
    setWindowTitle(QStringLiteral("Music · 本地音乐"));
    setWindowIcon(QIcon(QStringLiteral(":/images/images/music-app.svg")));

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    setCentralWidget(central);

    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *playerBar = new QFrame(central);
    playerBar->setObjectName(QStringLiteral("playerBar"));
    playerBar->setFixedHeight(88);
    auto *playerLayout = new QHBoxLayout(playerBar);
    playerLayout->setContentsMargins(24, 14, 24, 14);
    playerLayout->setSpacing(16);

    auto configureTransportButton = [](QPushButton *button, const QString &name) {
        button->setObjectName(name);
        button->setProperty("transport", true);
        button->setFixedSize(38, 38);
        button->setFocusPolicy(Qt::StrongFocus);
    };

    btn_Last = new QPushButton(playerBar);
    configureTransportButton(btn_Last, QStringLiteral("previousButton"));
    btn_Last->setIcon(makeIcon(AppIcon::Previous));
    btn_Last->setIconSize(QSize(20, 20));
    btn_Last->setToolTip(QStringLiteral("上一首"));
    btn_Last->setAccessibleName(QStringLiteral("上一首"));

    btn_Play = new QPushButton(playerBar);
    btn_Play->setObjectName(QStringLiteral("playButton"));
    btn_Play->setFixedSize(48, 48);
    btn_Play->setFocusPolicy(Qt::StrongFocus);
    btn_Play->setIconSize(QSize(23, 23));
    btn_Play->setAccessibleName(QStringLiteral("播放或暂停"));

    btn_Next = new QPushButton(playerBar);
    configureTransportButton(btn_Next, QStringLiteral("nextButton"));
    btn_Next->setIcon(makeIcon(AppIcon::Next));
    btn_Next->setIconSize(QSize(20, 20));
    btn_Next->setToolTip(QStringLiteral("下一首"));
    btn_Next->setAccessibleName(QStringLiteral("下一首"));

    auto *transportLayout = new QHBoxLayout;
    transportLayout->setSpacing(5);
    transportLayout->addWidget(btn_Last);
    transportLayout->addWidget(btn_Play);
    transportLayout->addWidget(btn_Next);
    playerLayout->addLayout(transportLayout);

    auto *trackLayout = new QVBoxLayout;
    trackLayout->setSpacing(5);
    lab_Name = new ElidedLabel(QStringLiteral("未选择歌曲"), playerBar);
    lab_Name->setObjectName(QStringLiteral("topTrackTitle"));
    lab_Name->setAlignment(Qt::AlignCenter);

    auto *timelineLayout = new QHBoxLayout;
    timelineLayout->setSpacing(9);
    lab_Position = new QLabel(QStringLiteral("00:00"), playerBar);
    lab_Position->setObjectName(QStringLiteral("timeLabel"));
    lab_Position->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lab_Position->setFixedWidth(46);

    sld_Position = new QSlider(Qt::Horizontal, playerBar);
    sld_Position->setObjectName(QStringLiteral("positionSlider"));
    sld_Position->setRange(0, 0);
    sld_Position->setValue(0);
    sld_Position->setTracking(true);
    sld_Position->setSingleStep(5000);
    sld_Position->setPageStep(10000);
    sld_Position->setAccessibleName(QStringLiteral("播放进度"));

    lab_Duration = new QLabel(QStringLiteral("00:00"), playerBar);
    lab_Duration->setObjectName(QStringLiteral("timeLabel"));
    lab_Duration->setFixedWidth(46);

    timelineLayout->addWidget(lab_Position);
    timelineLayout->addWidget(sld_Position, 1);
    timelineLayout->addWidget(lab_Duration);
    trackLayout->addWidget(lab_Name);
    trackLayout->addLayout(timelineLayout);
    playerLayout->addLayout(trackLayout, 1);

    btn_Loop = new QPushButton(playerBar);
    btn_Loop->setObjectName(QStringLiteral("loopButton"));
    btn_Loop->setProperty("utility", true);
    btn_Loop->setCheckable(true);
    btn_Loop->setChecked(false);
    btn_Loop->setFixedSize(36, 34);
    btn_Loop->setIcon(makeIcon(AppIcon::Repeat, kAccent));
    btn_Loop->setIconSize(QSize(19, 19));
    btn_Loop->setToolTip(QStringLiteral("重复播放列表"));
    btn_Loop->setAccessibleName(QStringLiteral("重复播放列表"));

    spn_Rate = new QDoubleSpinBox(playerBar);
    spn_Rate->setObjectName(QStringLiteral("rateSpin"));
    spn_Rate->setDecimals(1);
    spn_Rate->setRange(0.5, 3.0);
    spn_Rate->setSingleStep(0.1);
    spn_Rate->setValue(1.0);
    spn_Rate->setSuffix(QStringLiteral("×"));
    spn_Rate->setAlignment(Qt::AlignCenter);
    spn_Rate->setFixedSize(72, 34);
    spn_Rate->setToolTip(QStringLiteral("播放速度"));
    spn_Rate->setAccessibleName(QStringLiteral("播放速度"));

    btn_Sound = new QPushButton(playerBar);
    btn_Sound->setObjectName(QStringLiteral("soundButton"));
    btn_Sound->setProperty("utility", true);
    btn_Sound->setFixedSize(36, 34);
    btn_Sound->setIconSize(QSize(19, 19));
    btn_Sound->setAccessibleName(QStringLiteral("静音"));

    sld_Sound = new QSlider(Qt::Horizontal, playerBar);
    sld_Sound->setObjectName(QStringLiteral("volumeSlider"));
    sld_Sound->setRange(0, 100);
    sld_Sound->setValue(previousVolume);
    sld_Sound->setFixedWidth(112);
    sld_Sound->setToolTip(QStringLiteral("音量"));
    sld_Sound->setAccessibleName(QStringLiteral("音量"));

    auto *utilityLayout = new QHBoxLayout;
    utilityLayout->setSpacing(6);
    utilityLayout->addWidget(btn_Loop);
    utilityLayout->addWidget(spn_Rate);
    utilityLayout->addWidget(btn_Sound);
    utilityLayout->addWidget(sld_Sound);
    playerLayout->addLayout(utilityLayout);
    rootLayout->addWidget(playerBar);

    auto *body = new QWidget(central);
    body->setObjectName(QStringLiteral("body"));
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *sidebar = new QWidget(body);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setMinimumWidth(170);
    sidebar->setMaximumWidth(210);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 24, 18, 20);
    sidebarLayout->setSpacing(8);

    auto *brandLayout = new QHBoxLayout;
    brandLayout->setSpacing(10);
    auto *brandIcon = new QLabel(QStringLiteral("♪"), sidebar);
    brandIcon->setObjectName(QStringLiteral("brandIcon"));
    brandIcon->setAlignment(Qt::AlignCenter);
    brandIcon->setFixedSize(34, 34);
    auto *brandLabel = new QLabel(QStringLiteral("Music"), sidebar);
    brandLabel->setObjectName(QStringLiteral("brandLabel"));
    brandLayout->addWidget(brandIcon);
    brandLayout->addWidget(brandLabel);
    brandLayout->addStretch();
    sidebarLayout->addLayout(brandLayout);
    sidebarLayout->addSpacing(18);

    auto *librarySection = new QLabel(QStringLiteral("资料库"), sidebar);
    librarySection->setProperty("sectionTitle", true);
    sidebarLayout->addWidget(librarySection);

    auto *songsButton = new QPushButton(QStringLiteral("歌曲"), sidebar);
    songsButton->setProperty("navigation", true);
    songsButton->setProperty("selected", true);
    songsButton->setIcon(makeIcon(AppIcon::Music, kAccent));
    songsButton->setIconSize(QSize(18, 18));
    songsButton->setMinimumHeight(38);
    songsButton->setFocusPolicy(Qt::StrongFocus);
    connect(songsButton, &QPushButton::clicked, this, [this] {
        searchEdit->clear();
        on_searchTextChanged(QString());
        list_Music->setFocus(Qt::ShortcutFocusReason);
        setStatusMessage(QStringLiteral("已显示全部歌曲"));
    });
    sidebarLayout->addWidget(songsButton);

    sidebarLayout->addStretch();
    libraryCountLabel = new QLabel(QStringLiteral("0 首歌曲"), sidebar);
    libraryCountLabel->setObjectName(QStringLiteral("libraryCount"));
    sidebarLayout->addWidget(libraryCountLabel);

    statusLabel = new QLabel(QStringLiteral("将音频文件拖到歌曲列表即可导入"), sidebar);
    statusLabel->setObjectName(QStringLiteral("statusLabel"));
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(42);
    sidebarLayout->addWidget(statusLabel);
    bodyLayout->addWidget(sidebar);

    auto *content = new QWidget(body);
    content->setObjectName(QStringLiteral("content"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 18, 24, 18);
    contentLayout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(8);
    auto *titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);
    auto *title = new QLabel(QStringLiteral("歌曲"), content);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(QStringLiteral("你的本地音乐资料库"), content);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    searchEdit = new QLineEdit(content);
    searchEdit->setObjectName(QStringLiteral("searchEdit"));
    searchEdit->setPlaceholderText(QStringLiteral("搜索曲目或文件"));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setMinimumWidth(140);
    searchEdit->setMaximumWidth(218);
    searchEdit->setMinimumHeight(36);
    searchEdit->setAccessibleName(QStringLiteral("搜索歌曲"));
    headerLayout->addWidget(searchEdit);

    auto configureToolbarButton = [](QPushButton *button) {
        button->setProperty("toolbar", true);
        button->setMinimumHeight(36);
        button->setFocusPolicy(Qt::StrongFocus);
    };

    btn_Add = new QPushButton(QStringLiteral("添加"), content);
    configureToolbarButton(btn_Add);
    btn_Add->setIcon(makeIcon(AppIcon::Add, kAccent));
    btn_Add->setIconSize(QSize(18, 18));
    btn_Add->setToolTip(QStringLiteral("添加音频文件 (Ctrl/Cmd+O)"));

    btn_Remove = new QPushButton(content);
    configureToolbarButton(btn_Remove);
    btn_Remove->setFixedWidth(38);
    btn_Remove->setIcon(makeIcon(AppIcon::Remove));
    btn_Remove->setIconSize(QSize(18, 18));
    btn_Remove->setToolTip(QStringLiteral("移除所选歌曲 (Delete)"));
    btn_Remove->setAccessibleName(QStringLiteral("移除所选歌曲"));

    btn_Clear = new QPushButton(QStringLiteral("清空"), content);
    configureToolbarButton(btn_Clear);
    btn_Clear->setToolTip(QStringLiteral("清空资料库"));

    headerLayout->addWidget(btn_Add);
    headerLayout->addWidget(btn_Remove);
    headerLayout->addWidget(btn_Clear);
    contentLayout->addLayout(headerLayout);

    auto *splitter = new QSplitter(Qt::Horizontal, content);
    splitter->setObjectName(QStringLiteral("contentSplitter"));
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(12);

    auto *libraryCard = new QFrame(splitter);
    libraryCard->setObjectName(QStringLiteral("card"));
    auto *libraryLayout = new QVBoxLayout(libraryCard);
    libraryLayout->setContentsMargins(8, 8, 8, 8);
    libraryLayout->setSpacing(0);

    list_Music = new MusicListWidget(libraryCard);
    list_Music->setObjectName(QStringLiteral("musicList"));
    list_Music->setItemDelegate(new TrackDelegate(list_Music));
    list_Music->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_Music->setAlternatingRowColors(false);
    list_Music->setUniformItemSizes(true);
    list_Music->setMouseTracking(true);
    list_Music->setFrameShape(QFrame::NoFrame);
    list_Music->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_Music->setContextMenuPolicy(Qt::DefaultContextMenu);
    list_Music->installEventFilter(this);
    list_Music->filesDropped = [this](const QStringList &paths) { addFiles(paths); };
    libraryLayout->addWidget(list_Music);

    auto *nowPlayingCard = new QFrame(splitter);
    nowPlayingCard->setObjectName(QStringLiteral("card"));
    nowPlayingCard->setMinimumWidth(250);
    auto *nowPlayingLayout = new QVBoxLayout(nowPlayingCard);
    nowPlayingLayout->setContentsMargins(18, 14, 18, 16);
    nowPlayingLayout->setSpacing(6);

    auto *nowPlayingLabel = new QLabel(QStringLiteral("正在播放"), nowPlayingCard);
    nowPlayingLabel->setProperty("sectionTitle", true);
    nowPlayingLayout->addWidget(nowPlayingLabel);

    pic_Music = new ArtworkWidget(nowPlayingCard);
    pic_Music->setAccessibleName(QStringLiteral("专辑封面"));
    pic_Music->setMinimumSize(100, 100);
    pic_Music->setMaximumSize(192, 192);
    auto *artLayout = new QHBoxLayout;
    artLayout->addStretch();
    artLayout->addWidget(pic_Music);
    artLayout->addStretch();
    nowPlayingLayout->addLayout(artLayout);

    nowPlayingTitle = new ElidedLabel(QStringLiteral("未在播放"), nowPlayingCard);
    nowPlayingTitle->setObjectName(QStringLiteral("nowPlayingTitle"));
    nowPlayingTitle->setAlignment(Qt::AlignCenter);
    nowPlayingArtist = new ElidedLabel(QStringLiteral("添加歌曲以开始播放"), nowPlayingCard);
    nowPlayingArtist->setObjectName(QStringLiteral("nowPlayingArtist"));
    nowPlayingArtist->setAlignment(Qt::AlignCenter);
    nowPlayingLayout->addWidget(nowPlayingTitle);
    nowPlayingLayout->addWidget(nowPlayingArtist);

    spectrumWidget = new SpectrumWidget(nowPlayingCard);
    spectrumWidget->setAccessibleName(QStringLiteral("音频频谱"));
    spectrumWidget->setObjectName(QStringLiteral("spectrumWidget"));
    spectrumWidget->setMinimumHeight(44);
    spectrumWidget->setMaximumHeight(96);
    nowPlayingLayout->addWidget(spectrumWidget);

    karaokeWidget = new KaraokeWidget(nowPlayingCard);
    karaokeWidget->setAccessibleName(QStringLiteral("歌词"));
    karaokeWidget->setObjectName(QStringLiteral("karaokeWidget"));
    karaokeWidget->setMinimumHeight(55);
    nowPlayingLayout->addWidget(karaokeWidget, 1);

    splitter->addWidget(libraryCard);
    splitter->addWidget(nowPlayingCard);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({590, 360});
    contentLayout->addWidget(splitter, 1);

    bodyLayout->addWidget(content, 1);
    rootLayout->addWidget(body, 1);

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#central, QWidget#body, QWidget#content {
            background: #F5F5F7;
            color: #1D1D1F;
            font-family: "SF Pro Text", "Segoe UI", "Microsoft YaHei UI";
            font-size: 13px;
        }
        QFrame#playerBar {
            background: rgba(250, 250, 252, 248);
            border-bottom: 1px solid #D8D8DC;
        }
        QWidget#sidebar {
            background: #ECECF1;
            border-right: 1px solid #D9D9DE;
        }
        QLabel#brandIcon {
            background: #FA2D55;
            color: white;
            border-radius: 9px;
            font-size: 21px;
            font-weight: 700;
        }
        QLabel#brandLabel {
            color: #1D1D1F;
            font-size: 21px;
            font-weight: 700;
        }
        QLabel[sectionTitle="true"] {
            color: #6E6E73;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton[navigation="true"] {
            background: transparent;
            color: #1D1D1F;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 7px 10px;
            text-align: left;
            font-weight: 600;
        }
        QPushButton[navigation="true"][selected="true"] {
            background: rgba(255, 255, 255, 180);
            color: #FA2D55;
        }
        QPushButton[navigation="true"]:focus {
            border-color: #FA2D55;
        }
        QLabel#libraryCount, QLabel#statusLabel, QLabel#pageSubtitle,
        QLabel#nowPlayingArtist, QLabel#timeLabel {
            color: #6E6E73;
        }
        QLabel#statusLabel {
            font-size: 11px;
        }
        QLabel#pageTitle {
            color: #1D1D1F;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#pageSubtitle {
            font-size: 12px;
        }
        QLabel#topTrackTitle {
            color: #3A3A3C;
            font-size: 12px;
            font-weight: 600;
        }
        QLabel#timeLabel {
            font-size: 10px;
        }
        QLabel#nowPlayingTitle {
            color: #1D1D1F;
            font-size: 17px;
            font-weight: 650;
        }
        QLabel#nowPlayingArtist {
            font-size: 12px;
        }
        QFrame#card {
            background: #FFFFFF;
            border: 1px solid #E1E1E5;
            border-radius: 14px;
        }
        QSplitter#contentSplitter::handle {
            background: transparent;
        }
        QLineEdit#searchEdit {
            background: #EAEAEE;
            border: 1px solid transparent;
            border-radius: 9px;
            padding: 7px 12px;
            selection-background-color: #FA2D55;
        }
        QLineEdit#searchEdit:focus {
            background: #FFFFFF;
            border-color: rgba(250, 45, 85, 150);
        }
        QPushButton[toolbar="true"] {
            background: #FFFFFF;
            color: #3A3A3C;
            border: 1px solid #D8D8DC;
            border-radius: 9px;
            padding: 6px 12px;
        }
        QPushButton[toolbar="true"]:hover {
            background: #F4F4F6;
            border-color: #C8C8CC;
        }
        QPushButton[toolbar="true"]:pressed {
            background: #EAEAEE;
        }
        QPushButton[toolbar="true"]:focus {
            border-color: #FA2D55;
        }
        QPushButton[toolbar="true"]:disabled {
            color: #AEAEB2;
            background: #F4F4F6;
        }
        QPushButton[transport="true"], QPushButton[utility="true"] {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 9px;
        }
        QPushButton[transport="true"]:hover, QPushButton[utility="true"]:hover {
            background: #EAEAEE;
        }
        QPushButton[transport="true"]:focus, QPushButton[utility="true"]:focus {
            border-color: #FA2D55;
        }
        QPushButton#playButton {
            background: #FA2D55;
            border: 2px solid transparent;
            border-radius: 24px;
        }
        QPushButton#playButton:hover {
            background: #E9264D;
        }
        QPushButton#playButton:pressed {
            background: #D51F44;
        }
        QPushButton#playButton:focus {
            border-color: #FFFFFF;
        }
        QPushButton#playButton:disabled {
            background: #C7C7CC;
        }
        QPushButton#loopButton:checked {
            background: #FFE5EC;
        }
        QDoubleSpinBox#rateSpin {
            background: #EAEAEE;
            color: #3A3A3C;
            border: 1px solid transparent;
            border-radius: 9px;
            padding: 5px 7px;
        }
        QDoubleSpinBox#rateSpin:focus {
            border-color: #FA2D55;
        }
        QDoubleSpinBox#rateSpin::up-button,
        QDoubleSpinBox#rateSpin::down-button {
            width: 0px;
            height: 0px;
        }
        QSlider {
            border: 1px solid transparent;
            border-radius: 5px;
        }
        QSlider:focus {
            border-color: rgba(250, 45, 85, 180);
        }
        QSlider::groove:horizontal {
            height: 4px;
            background: #D1D1D6;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal {
            background: #FA2D55;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF;
            border: 1px solid #B8B8BD;
            width: 13px;
            height: 13px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            border-color: #FA2D55;
        }
        QSlider::handle:horizontal:focus {
            border: 2px solid #FA2D55;
        }
        QListWidget#musicList {
            background: #FFFFFF;
            border: none;
            outline: none;
            padding: 2px;
            selection-background-color: transparent;
        }
        QListWidget#musicList::item {
            border: none;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 4px 2px;
        }
        QScrollBar::handle:vertical {
            background: #C7C7CC;
            border-radius: 4px;
            min-height: 28px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0px;
        }
        QToolTip {
            background: #2C2C2E;
            color: white;
            border: none;
            padding: 5px 8px;
        }
    )"));
}

void MainWindow::setUpShortcuts()
{
    auto *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, &MainWindow::on_btn_Add_clicked);

    auto *findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, searchEdit,
            qOverload<>(&QLineEdit::setFocus));

    auto *muteShortcut =
        new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_M), this);
    connect(muteShortcut, &QShortcut::activated, this, &MainWindow::on_btn_Sound_clicked);

    auto *mediaPreviousShortcut =
        new QShortcut(QKeySequence(Qt::Key_MediaPrevious), this);
    connect(mediaPreviousShortcut, &QShortcut::activated,
            this, &MainWindow::on_btn_Last_clicked);

    auto *mediaNextShortcut =
        new QShortcut(QKeySequence(Qt::Key_MediaNext), this);
    connect(mediaNextShortcut, &QShortcut::activated,
            this, &MainWindow::on_btn_Next_clicked);

    auto *mediaPlayShortcut =
        new QShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause), this);
    connect(mediaPlayShortcut, &QShortcut::activated,
            this, &MainWindow::on_btn_Play_clicked);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == list_Music && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            removeSelectedTracks();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (list_Music->currentItem() && !list_Music->currentItem()->isHidden())
                playRow(list_Music->currentRow());
            return true;
        case Qt::Key_Space:
            if (keyEvent->modifiers() == Qt::NoModifier) {
                on_btn_Play_clicked();
                return true;
            }
            break;
        default:
            break;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space
            && keyEvent->modifiers() == Qt::NoModifier
            && QApplication::activeWindow() == this
            && QApplication::activeModalWidget() == nullptr) {
            QWidget *focusWidget = QApplication::focusWidget();
            if (!qobject_cast<QLineEdit *>(focusWidget)
                && !qobject_cast<QPushButton *>(focusWidget)
                && !qobject_cast<QDoubleSpinBox *>(focusWidget)) {
                on_btn_Play_clicked();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!pic_Music || !spectrumWidget)
        return;

    if (height() < 560) {
        spectrumWidget->setVisible(false);
        pic_Music->setMaximumSize(100, 100);
    } else if (height() < 640) {
        spectrumWidget->setVisible(false);
        pic_Music->setMaximumSize(150, 150);
    } else {
        spectrumWidget->setVisible(true);
        pic_Music->setMaximumSize(192, 192);
    }
}

void MainWindow::addFiles(const QStringList &filePaths)
{
    QSet<QString> existingPaths;
    for (int row = 0; row < list_Music->count(); ++row) {
        const QUrl url = getUrlFromItem(list_Music->item(row));
        if (url.isLocalFile())
            existingPaths.insert(normalizedPathKey(url.toLocalFile()));
    }

    int addedCount = 0;
    int skippedCount = 0;
    int firstAddedRow = -1;
    for (const QString &path : filePaths) {
        const QFileInfo info(path);
        if (!isSupportedAudioFile(info)) {
            ++skippedCount;
            continue;
        }

        const QString pathKey = normalizedPathKey(info.absoluteFilePath());
        if (existingPaths.contains(pathKey)) {
            ++skippedCount;
            continue;
        }
        existingPaths.insert(pathKey);

        auto *item = new QListWidgetItem(info.completeBaseName());
        item->setData(UrlRole, QUrl::fromLocalFile(info.absoluteFilePath()));
        item->setData(ArtistRole,
                      QStringLiteral("本地文件 · %1").arg(info.suffix().toUpper()));
        item->setToolTip(QDir::toNativeSeparators(info.absoluteFilePath()));
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
        list_Music->addItem(item);

        if (firstAddedRow < 0)
            firstAddedRow = list_Music->count() - 1;
        ++addedCount;
    }

    on_searchTextChanged(searchEdit->text());
    if (firstAddedRow >= 0 && player->source().isEmpty()) {
        int rowToPrepare = firstAddedRow;
        while (rowToPrepare < list_Music->count()
               && list_Music->item(rowToPrepare)->isHidden()) {
            ++rowToPrepare;
        }
        if (rowToPrepare < list_Music->count())
            playRow(rowToPrepare, false);
    }

    if (addedCount > 0) {
        QString message = QStringLiteral("已添加 %1 首歌曲").arg(addedCount);
        if (skippedCount > 0)
            message += QStringLiteral("，跳过 %1 个重复或不支持的文件").arg(skippedCount);
        setStatusMessage(message);
    } else if (skippedCount > 0) {
        setStatusMessage(QStringLiteral("没有可添加的文件：文件可能重复或格式不受支持"), true);
    }
}

void MainWindow::removeSelectedTracks()
{
    const QList<QListWidgetItem *> selected = list_Music->selectedItems();
    if (selected.isEmpty())
        return;

    const QUrl activeSource = player->source();
    const int activeRow = rowForUrl(activeSource);
    const bool wasPlaying = player->playbackState() == QMediaPlayer::PlayingState;
    bool removesActiveSource = false;
    int firstRemovedRow = list_Music->count();
    QVector<int> rows;
    rows.reserve(selected.size());

    for (QListWidgetItem *item : selected) {
        const int row = list_Music->row(item);
        failedPlaybackSources.remove(getUrlFromItem(item));
        if (row >= 0) {
            rows.append(row);
            firstRemovedRow = std::min(firstRemovedRow, row);
        }
        if (!activeSource.isEmpty() && getUrlFromItem(item) == activeSource)
            removesActiveSource = true;
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    const int removedBeforeActive = static_cast<int>(std::count_if(
        rows.cbegin(), rows.cend(),
        [activeRow](int row) { return activeRow >= 0 && row < activeRow; }));
    for (int row : rows)
        delete list_Music->takeItem(row);

    if (list_Music->count() == 0) {
        resetPlayback();
    } else if (removesActiveSource) {
        player->stop();
        player->setSource(QUrl());
        const int insertionRow =
            std::clamp(activeRow - removedBeforeActive, 0, list_Music->count() - 1);
        int nextRow = nextVisibleRow(insertionRow - 1, 1, false);
        if (nextRow < 0)
            nextRow = nextVisibleRow(insertionRow, -1, false);
        if (nextRow >= 0) {
            playRow(nextRow, wasPlaying);
        } else {
            playbackRequested = false;
            list_Music->clearSelection();
            list_Music->setCurrentRow(-1);
        }
    } else {
        const int insertionRow =
            std::clamp(firstRemovedRow, 0, list_Music->count() - 1);
        int nextSelection = nextVisibleRow(insertionRow - 1, 1, false);
        if (nextSelection < 0)
            nextSelection = nextVisibleRow(insertionRow, -1, false);
        list_Music->setCurrentRow(nextSelection);
        updatePlayingIndicator();
    }

    updateLibraryCount();
    setStatusMessage(QStringLiteral("已从资料库移除 %1 首歌曲").arg(rows.size()));
}

void MainWindow::resetPlayback()
{
    playbackRequested = false;
    failedPlaybackSources.clear();
    if (player) {
        player->stop();
        player->setSource(QUrl());
    }
    currentDuration = 0;
    sld_Position->setRange(0, 0);
    sld_Position->setValue(0);
    lab_Position->setText(QStringLiteral("00:00"));
    lab_Duration->setText(QStringLiteral("00:00"));
    lab_Name->setText(QStringLiteral("未选择歌曲"));
    nowPlayingTitle->setText(QStringLiteral("未在播放"));
    nowPlayingArtist->setText(QStringLiteral("添加歌曲以开始播放"));
    pic_Music->clearArtwork();

    if (lyricsController)
        lyricsController->clear();
    if (audioAnalyzer)
        audioAnalyzer->reset();
    if (spectrumWidget)
        spectrumWidget->clearData();

    updatePlayingIndicator();
    if (player)
        updatePlaybackButton(player->playbackState());
}

void MainWindow::playRow(int row, bool startPlayback, bool resetFailureHistory)
{
    if (row < 0 || row >= list_Music->count())
        return;

    QListWidgetItem *item = list_Music->item(row);
    const QUrl source = getUrlFromItem(item);
    if (source.isEmpty())
        return;

    const bool reloadInvalidSource =
        resetFailureHistory && player->source() == source
        && player->mediaStatus() == QMediaPlayer::InvalidMedia;
    if (resetFailureHistory)
        failedPlaybackSources.clear();
    playbackRequested = startPlayback;

    list_Music->setCurrentItem(item);
    list_Music->scrollToItem(item, QAbstractItemView::EnsureVisible);
    if (reloadInvalidSource) {
        player->setSource(QUrl());
        player->setSource(source);
    } else if (player->source() != source) {
        player->setSource(source);
    }

    lab_Name->setText(item->text());
    nowPlayingTitle->setText(item->text());
    nowPlayingArtist->setText(item->data(ArtistRole).toString());

    if (startPlayback) {
        player->play();
        setStatusMessage(QStringLiteral("正在播放“%1”").arg(item->text()));
    }
    updatePlayingIndicator();
}

void MainWindow::updateLibraryCount()
{
    int visibleCount = 0;
    for (int row = 0; row < list_Music->count(); ++row) {
        if (!list_Music->item(row)->isHidden())
            ++visibleCount;
    }

    const int totalCount = list_Music->count();
    if (visibleCount == totalCount) {
        libraryCountLabel->setText(QStringLiteral("%1 首歌曲").arg(totalCount));
    } else {
        libraryCountLabel->setText(
            QStringLiteral("显示 %1 / 共 %2 首").arg(visibleCount).arg(totalCount));
    }

    btn_Remove->setEnabled(!list_Music->selectedItems().isEmpty());
    btn_Clear->setEnabled(totalCount > 0);
    const bool hasActiveSource = player && !player->source().isEmpty();
    btn_Play->setEnabled(visibleCount > 0 || hasActiveSource);
    btn_Last->setEnabled(visibleCount > 0);
    btn_Next->setEnabled(visibleCount > 0);
}

void MainWindow::updatePlayingIndicator()
{
    if (!list_Music)
        return;

    const QUrl source = player ? player->source() : QUrl();
    const int playbackState =
        player ? static_cast<int>(player->playbackState())
               : static_cast<int>(QMediaPlayer::StoppedState);

    for (int row = 0; row < list_Music->count(); ++row) {
        QListWidgetItem *item = list_Music->item(row);
        item->setData(PlayingRole, !source.isEmpty() && getUrlFromItem(item) == source);
        item->setData(PlaybackStateRole, playbackState);
    }
    list_Music->viewport()->update();
}

void MainWindow::updatePlaybackButton(QMediaPlayer::PlaybackState state)
{
    const bool playing = state == QMediaPlayer::PlayingState;
    btn_Play->setIcon(makeIcon(playing ? AppIcon::Pause : AppIcon::Play, Qt::white));
    btn_Play->setToolTip(playing ? QStringLiteral("暂停 (Space)")
                                 : QStringLiteral("播放 (Space)"));
    btn_Play->setAccessibleName(playing ? QStringLiteral("暂停") : QStringLiteral("播放"));
}

void MainWindow::updateVolumeButton()
{
    if (!audioOutput)
        return;

    const bool muted = audioOutput->isMuted() || sld_Sound->value() == 0;
    btn_Sound->setIcon(makeIcon(muted ? AppIcon::Mute : AppIcon::Volume));
    btn_Sound->setToolTip(muted ? QStringLiteral("取消静音 (Ctrl+M)")
                                : QStringLiteral("静音 (Ctrl+M)"));
    btn_Sound->setAccessibleName(muted ? QStringLiteral("取消静音") : QStringLiteral("静音"));
}

void MainWindow::setPlayerVolume(int value)
{
    if (!audioOutput)
        return;

    value = std::clamp(value, 0, 100);
    if (value > 0)
        previousVolume = value;
    const qreal linearVolume = QAudio::convertVolume(
        value / 100.0,
        QAudio::LogarithmicVolumeScale,
        QAudio::LinearVolumeScale);
    audioOutput->setVolume(linearVolume);
    audioOutput->setMuted(value == 0);
    updateVolumeButton();
}

void MainWindow::setStatusMessage(const QString &message, bool isError)
{
    statusLabel->setText(message);
    statusLabel->setStyleSheet(isError ? QStringLiteral("color: #D70015;")
                                       : QStringLiteral("color: #6E6E73;"));
}

QUrl MainWindow::getUrlFromItem(const QListWidgetItem *item) const
{
    return item ? item->data(UrlRole).toUrl() : QUrl();
}

int MainWindow::rowForUrl(const QUrl &url) const
{
    if (url.isEmpty())
        return -1;

    for (int row = 0; row < list_Music->count(); ++row) {
        if (getUrlFromItem(list_Music->item(row)) == url)
            return row;
    }
    return -1;
}

int MainWindow::nextVisibleRow(int fromRow, int direction, bool wrap) const
{
    const int count = list_Music->count();
    if (count <= 0 || direction == 0)
        return -1;

    int row = fromRow;
    for (int checked = 0; checked < count; ++checked) {
        row += direction > 0 ? 1 : -1;
        if (row < 0 || row >= count) {
            if (!wrap)
                return -1;
            row = direction > 0 ? 0 : count - 1;
        }
        if (!list_Music->item(row)->isHidden())
            return row;
    }
    return -1;
}

int MainWindow::nextRecoverableRow(int fromRow, bool wrap) const
{
    const int count = list_Music->count();
    if (count <= 0)
        return -1;

    int row = fromRow;
    for (int checked = 0; checked < count; ++checked) {
        ++row;
        if (row >= count) {
            if (!wrap)
                return -1;
            row = 0;
        }

        QListWidgetItem *item = list_Music->item(row);
        const QUrl source = getUrlFromItem(item);
        if (!item->isHidden() && !source.isEmpty()
            && !failedPlaybackSources.contains(source)) {
            return row;
        }
    }
    return -1;
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    milliseconds = std::max<qint64>(0, milliseconds);
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::on_btn_Add_clicked()
{
    const QString filter = QStringLiteral(
        "音频文件 (*.mp3 *.m4a *.aac *.wav *.flac *.ogg *.opus *.wma *.mp4 "
        "*.aif *.aiff *.caf);;"
        "所有文件 (*.*)");
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("添加到音乐资料库"), QDir::homePath(), filter);
    if (!files.isEmpty())
        addFiles(files);
}

void MainWindow::on_btn_Remove_clicked()
{
    removeSelectedTracks();
}

void MainWindow::on_btn_Clear_clicked()
{
    if (list_Music->count() == 0)
        return;

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("清空音乐资料库"),
        QStringLiteral("要从当前列表中移除全部歌曲吗？\n磁盘上的音频文件不会被删除。"),
        QMessageBox::Cancel | QMessageBox::Yes,
        QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    resetPlayback();
    list_Music->clear();
    updateLibraryCount();
    setStatusMessage(QStringLiteral("音乐资料库已清空"));
}

void MainWindow::on_list_Music_doubleClicked(const QModelIndex &index)
{
    if (index.isValid())
        playRow(index.row());
}

void MainWindow::on_btn_Last_clicked()
{
    const int count = list_Music->count();
    if (count <= 0)
        return;

    int row = rowForUrl(player->source());
    if (row < 0)
        row = list_Music->currentRow();
    if (row < 0)
        row = 0;

    if (player->position() > 3000) {
        audioAnalyzer->reset();
        spectrumWidget->clearData();
        player->setPosition(0);
        if (player->playbackState() != QMediaPlayer::PlayingState)
            player->play();
        return;
    }

    row = nextVisibleRow(row, -1, true);
    if (row >= 0)
        playRow(row);
}

void MainWindow::on_btn_Next_clicked()
{
    const int count = list_Music->count();
    if (count <= 0)
        return;

    int row = rowForUrl(player->source());
    if (row < 0)
        row = list_Music->currentRow();
    if (row < 0)
        row = -1;

    row = nextVisibleRow(row, 1, true);
    if (row >= 0)
        playRow(row);
}

void MainWindow::on_btn_Play_clicked()
{
    if (list_Music->count() <= 0)
        return;

    if (player->playbackState() == QMediaPlayer::PlayingState) {
        playbackRequested = false;
        player->pause();
        return;
    }

    int row = rowForUrl(player->source());
    if (row < 0) {
        row = list_Music->currentRow();
        if (row < 0 || list_Music->item(row)->isHidden())
            row = nextVisibleRow(-1, 1, false);
    }
    if (row < 0)
        return;
    playRow(row);
}

void MainWindow::on_doubleSpinBox_valueChanged(double value)
{
    player->setPlaybackRate(value);
}

void MainWindow::on_btn_Loop_clicked(bool checked)
{
    setStatusMessage(checked ? QStringLiteral("已开启列表重复播放")
                             : QStringLiteral("已关闭列表重复播放"));
}

void MainWindow::on_sld_Position_valueChanged(int value)
{
    if (currentDuration <= 0)
        return;
    audioAnalyzer->reset();
    spectrumWidget->clearData();
    player->setPosition(value);
}

void MainWindow::on_btn_Sound_clicked()
{
    if (audioOutput->isMuted() || sld_Sound->value() == 0) {
        const int restoredVolume = std::clamp(previousVolume, 1, 100);
        if (sld_Sound->value() == 0)
            sld_Sound->setValue(restoredVolume);
        audioOutput->setMuted(false);
    } else {
        previousVolume = std::max(1, sld_Sound->value());
        audioOutput->setMuted(true);
    }
    updateVolumeButton();
}

void MainWindow::on_sld_Sound_valueChanged(int value)
{
    setPlayerVolume(value);
}

void MainWindow::on_searchTextChanged(const QString &text)
{
    const QString needle = text.trimmed();
    for (int row = 0; row < list_Music->count(); ++row) {
        QListWidgetItem *item = list_Music->item(row);
        const QString haystack =
            item->text() + QLatin1Char('\n')
            + item->data(ArtistRole).toString() + QLatin1Char('\n')
            + item->toolTip();
        const bool hidden =
            !needle.isEmpty() && !haystack.contains(needle, Qt::CaseInsensitive);
        item->setHidden(hidden);
        if (hidden && item->isSelected())
            item->setSelected(false);
    }
    if (list_Music->currentItem() && list_Music->currentItem()->isHidden()) {
        QListWidgetItem *replacement = nullptr;
        const QList<QListWidgetItem *> selected = list_Music->selectedItems();
        for (QListWidgetItem *item : selected) {
            if (!item->isHidden()) {
                replacement = item;
                break;
            }
        }
        if (replacement) {
            list_Music->setCurrentItem(replacement, QItemSelectionModel::NoUpdate);
        } else {
            list_Music->setCurrentRow(-1, QItemSelectionModel::NoUpdate);
        }
    }
    updateLibraryCount();
}

void MainWindow::do_stateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState)
        playbackRequested = true;
    else if (state == QMediaPlayer::PausedState)
        playbackRequested = false;

    updatePlaybackButton(state);
    updatePlayingIndicator();
    if (state == QMediaPlayer::StoppedState) {
        audioAnalyzer->reset();
        spectrumWidget->clearData();
    }
}

void MainWindow::do_mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        failedPlaybackSources.clear();
        audioAnalyzer->reset();
        spectrumWidget->clearData();

        const int currentRow = rowForUrl(player->source());
        const int nextRow = nextVisibleRow(currentRow, 1, false);
        if (nextRow >= 0) {
            playRow(nextRow);
        } else if (btn_Loop->isChecked()) {
            const int firstVisibleRow = nextVisibleRow(-1, 1, false);
            if (firstVisibleRow >= 0)
                playRow(firstVisibleRow);
            else {
                playbackRequested = false;
                setStatusMessage(QStringLiteral("当前筛选结果中没有可播放歌曲"));
            }
        } else {
            playbackRequested = false;
            setStatusMessage(QStringLiteral("播放队列已结束"));
        }
    } else if (status == QMediaPlayer::InvalidMedia) {
        const QUrl failedSource = player->source();
        if (!failedSource.isEmpty())
            failedPlaybackSources.insert(failedSource);

        const int nextRow = nextRecoverableRow(rowForUrl(failedSource),
                                               btn_Loop->isChecked());

        if (nextRow >= 0) {
            setStatusMessage(QStringLiteral("已跳过无法播放的文件"), true);
            playRow(nextRow, playbackRequested, false);
        } else {
            playbackRequested = false;
            setStatusMessage(QStringLiteral("无法读取当前音频文件"), true);
        }
    }
}

void MainWindow::do_sourceChanged(const QUrl &source)
{
    currentDuration = 0;
    sld_Position->setRange(0, 0);
    sld_Position->setValue(0);
    lab_Position->setText(QStringLiteral("00:00"));
    lab_Duration->setText(QStringLiteral("00:00"));
    audioAnalyzer->reset();
    spectrumWidget->clearData();
    lyricsController->clear();
    pic_Music->clearArtwork();

    if (source.isEmpty()) {
        lab_Name->setText(QStringLiteral("未选择歌曲"));
        nowPlayingTitle->setText(QStringLiteral("未在播放"));
        nowPlayingArtist->setText(QStringLiteral("添加歌曲以开始播放"));
        updatePlayingIndicator();
        return;
    }

    const int row = rowForUrl(source);
    QListWidgetItem *item = row >= 0 ? list_Music->item(row) : nullptr;
    const QString fallbackTitle =
        item ? item->text() : QFileInfo(source.toLocalFile()).completeBaseName();
    lab_Name->setText(fallbackTitle);
    nowPlayingTitle->setText(fallbackTitle);
    nowPlayingArtist->setText(
        item ? item->data(ArtistRole).toString() : QStringLiteral("本地文件"));

    if (source.isLocalFile())
        lyricsController->loadFromAudioFile(source.toLocalFile());
    updatePlayingIndicator();
}

void MainWindow::do_positionChanged(qint64 position)
{
    if (!sld_Position->isSliderDown()) {
        const qint64 safePosition =
            std::clamp<qint64>(position, 0, std::numeric_limits<int>::max());
        sld_Position->setValue(static_cast<int>(safePosition));
    }
    lab_Position->setText(formatTime(position));
    lyricsController->setPosition(position);
}

void MainWindow::do_durationChanged(qint64 duration)
{
    currentDuration = std::max<qint64>(0, duration);
    const qint64 sliderMaximum =
        std::min<qint64>(currentDuration, std::numeric_limits<int>::max());
    sld_Position->setRange(0, static_cast<int>(sliderMaximum));
    lab_Duration->setText(formatTime(currentDuration));
    lyricsController->setDuration(currentDuration);
}

void MainWindow::do_metaDataChanged()
{
    if (player->source().isEmpty())
        return;

    const QMediaMetaData metaData = player->metaData();
    QString title = metaData.stringValue(QMediaMetaData::Title).trimmed();
    QString artist = metaData.stringValue(QMediaMetaData::AlbumArtist).trimmed();
    if (artist.isEmpty())
        artist = metaData.stringValue(QMediaMetaData::ContributingArtist).trimmed();
    if (artist.isEmpty())
        artist = metaData.stringValue(QMediaMetaData::Author).trimmed();

    const int row = rowForUrl(player->source());
    QListWidgetItem *item = row >= 0 ? list_Music->item(row) : nullptr;
    if (title.isEmpty()) {
        title = item ? item->text()
                     : QFileInfo(player->source().toLocalFile()).completeBaseName();
    }
    if (artist.isEmpty())
        artist = item ? item->data(ArtistRole).toString() : QStringLiteral("未知艺术家");

    lab_Name->setText(title);
    nowPlayingTitle->setText(title);
    nowPlayingArtist->setText(artist);
    if (item) {
        item->setText(title);
        item->setData(ArtistRole, artist);
    }
    if (!searchEdit->text().trimmed().isEmpty())
        on_searchTextChanged(searchEdit->text());

    QVariant artwork = metaData.value(QMediaMetaData::CoverArtImage);
    if (!artwork.isValid() || artwork.isNull())
        artwork = metaData.value(QMediaMetaData::ThumbnailImage);

    QImage image;
    if (artwork.canConvert<QImage>())
        image = artwork.value<QImage>();
    else if (artwork.canConvert<QPixmap>())
        image = artwork.value<QPixmap>().toImage();

    if (image.isNull())
        pic_Music->clearArtwork();
    else
        pic_Music->setArtwork(image);
    updatePlayingIndicator();
}

void MainWindow::do_errorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    if (error == QMediaPlayer::NoError)
        return;

    const QString details =
        errorString.trimmed().isEmpty() ? QStringLiteral("未知错误") : errorString.trimmed();
    setStatusMessage(QStringLiteral("播放失败：%1").arg(details), true);
    updatePlaybackButton(player->playbackState());
    updatePlayingIndicator();
}

MainWindow::~MainWindow() = default;
