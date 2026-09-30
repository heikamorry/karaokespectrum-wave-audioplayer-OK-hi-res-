#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QSet>
#include <QStringList>
#include <QUrl>

class ArtworkWidget;
class AudioAnalyzer;
class ElidedLabel;
class KaraokeWidget;
class LyricsController;
class MusicListWidget;
class QAudioBufferOutput;
class QAudioOutput;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidgetItem;
class QPushButton;
class QResizeEvent;
class QSlider;
class SpectrumWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setUpUi();
    void setUpShortcuts();
    void addFiles(const QStringList &filePaths);
    void removeSelectedTracks();
    void resetPlayback();
    void playRow(int row, bool startPlayback = true, bool resetFailureHistory = true);
    void updateLibraryCount();
    void updatePlayingIndicator();
    void updatePlaybackButton(QMediaPlayer::PlaybackState state);
    void updateVolumeButton();
    void setPlayerVolume(int value);
    void setStatusMessage(const QString &message, bool isError = false);
    QUrl getUrlFromItem(const QListWidgetItem *item) const;
    int rowForUrl(const QUrl &url) const;
    int nextVisibleRow(int fromRow, int direction, bool wrap) const;
    int nextRecoverableRow(int fromRow, bool wrap) const;
    static QString formatTime(qint64 milliseconds);

    void on_btn_Add_clicked();
    void on_btn_Remove_clicked();
    void on_btn_Clear_clicked();
    void on_list_Music_doubleClicked(const QModelIndex &index);
    void on_btn_Last_clicked();
    void on_btn_Next_clicked();
    void on_btn_Play_clicked();
    void on_doubleSpinBox_valueChanged(double value);
    void on_btn_Loop_clicked(bool checked);
    void on_sld_Position_valueChanged(int value);
    void on_btn_Sound_clicked();
    void on_sld_Sound_valueChanged(int value);
    void on_searchTextChanged(const QString &text);

private slots:
    void do_stateChanged(QMediaPlayer::PlaybackState state);
    void do_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void do_sourceChanged(const QUrl &media);
    void do_positionChanged(qint64 position);
    void do_durationChanged(qint64 duration);
    void do_metaDataChanged();
    void do_errorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    QMediaPlayer *player = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QAudioBufferOutput *audioBufferOutput = nullptr;

    AudioAnalyzer *audioAnalyzer = nullptr;
    SpectrumWidget *spectrumWidget = nullptr;
    LyricsController *lyricsController = nullptr;
    KaraokeWidget *karaokeWidget = nullptr;

    QPushButton *btn_Add = nullptr;
    QPushButton *btn_Remove = nullptr;
    QPushButton *btn_Clear = nullptr;
    MusicListWidget *list_Music = nullptr;
    QLineEdit *searchEdit = nullptr;
    QLabel *libraryCountLabel = nullptr;

    ArtworkWidget *pic_Music = nullptr;
    ElidedLabel *nowPlayingTitle = nullptr;
    ElidedLabel *nowPlayingArtist = nullptr;

    QPushButton *btn_Play = nullptr;
    QPushButton *btn_Last = nullptr;
    QPushButton *btn_Next = nullptr;
    QDoubleSpinBox *spn_Rate = nullptr;
    QPushButton *btn_Loop = nullptr;
    QPushButton *btn_Sound = nullptr;
    QSlider *sld_Sound = nullptr;

    ElidedLabel *lab_Name = nullptr;
    QLabel *lab_Position = nullptr;
    QSlider *sld_Position = nullptr;
    QLabel *lab_Duration = nullptr;
    QLabel *statusLabel = nullptr;

    qint64 currentDuration = 0;
    int previousVolume = 70;
    QSet<QUrl> failedPlaybackSources;
    bool playbackRequested = false;
};

#endif // MAINWINDOW_H
