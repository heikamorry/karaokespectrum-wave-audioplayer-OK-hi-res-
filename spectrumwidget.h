#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include <QVector>

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

public slots:
    void setSpectrumData(const QVector<float> &bars, const QVector<float> &waveform);
    void clearData();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<float> m_bars;
    QVector<float> m_waveform;
};

#endif // SPECTRUMWIDGET_H