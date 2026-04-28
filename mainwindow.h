#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QtMultimedia/QtMultimedia>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QFileDialog>

class QAudioOutput;
class QAudioBufferOutput;
class AudioAnalyzer;
class SpectrumWidget;
class LyricsController;
class KaraokeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QMediaPlayer *player = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QAudioBufferOutput *audioBufferOutput = nullptr;

    AudioAnalyzer *audioAnalyzer = nullptr;
    SpectrumWidget *spectrumWidget = nullptr;
    LyricsController *lyricsController = nullptr;
    KaraokeWidget *karaokeWidget = nullptr;

    bool loopPlay = true;
    QString durationTime;
    QString positionTime;

    QPushButton *btn_Add = nullptr;
    QPushButton *btn_Remove = nullptr;
    QPushButton *btn_Clear = nullptr;
    QPushButton *btn_Exit = nullptr;

    QListWidget *list_Music = nullptr;
    QLabel *pic_Music = nullptr;

    QPushButton *btn_Play = nullptr;
    QPushButton *btn_Pause = nullptr;
    QPushButton *btn_Stop = nullptr;
    QPushButton *btn_Last = nullptr;
    QPushButton *btn_Next = nullptr;

    QDoubleSpinBox *spn_Rate = nullptr;
    QPushButton *btn_Loop = nullptr;
    QPushButton *btn_Sound = nullptr;
    QSlider *sld_Sound = nullptr;

    QLabel *lab_Name = nullptr;
    QSlider *sld_Position = nullptr;
    QLabel *lab_Duration = nullptr;

    void setUpUi(QWidget *parent);
    QUrl getUrlFromItem(QListWidgetItem *item);
    bool eventFilter(QObject *watched, QEvent *event) override;

    void on_btn_Add_clicked();
    void on_btn_Remove_clicked();
    void on_btn_Clear_clicked();
    void on_list_Music_doubleClicked(const QModelIndex &index);
    void on_btn_Last_clicked();
    void on_btn_Next_clicked();
    void on_btn_Play_clicked();
    void on_btn_Pause_clicked();
    void on_btn_Stop_clicked();
    void on_doubleSpinBox_valueChanged(double arg1);
    void on_btn_Loop_clicked(bool checked);
    void on_sld_Position_valueChanged(int value);
    void on_btn_Sound_clicked();
    void on_sld_Sound_valueChanged(int value);

private slots:
    void do_stateChanged(QMediaPlayer::PlaybackState state);
    void do_sourceChanged(const QUrl &media);
    void do_positionChanged(qint64 pos);
    void do_durationChanged(qint64 dur);
    void do_metaDataChanged();

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
};

#endif // MAINWINDOW_H