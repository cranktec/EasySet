#ifndef INFORMATIONWIDGET_HPP
#define INFORMATIONWIDGET_HPP

#include <iostream>
#include <QWidget>
#include <QLabel>
#include <QLCDNumber>
#include <QTimer>
#include <QTime>

class InformationWidget : public QWidget
{
    Q_OBJECT

private:
    QLabel *m_playerNum;
    QLabel *m_scores;
    QLabel *m_deckLength;
    QTimer *m_refreshTimer;
    QLCDNumber *m_waitTime;
    int m_startValue;
    int m_currentValue;
    QLabel *m_playerCount;
    QString scoreboard;

public:
    explicit InformationWidget(QWidget *parent = 0);
    void setPlayerNumber(int p_number);
    void setPlayerCount(short p_playerCount);
    void setDeckLength(short p_deckLength);
    void setSetCount(short p_setCount);
    void setWaitTimeValue(int p_value);
    void restartTimer();
    void setScores(QByteArray p_scores);

signals:

private slots:
    void showCountDown();

};

#endif // INFORMATIONWIDGET_HPP
