// g++ -std=c++11 -fpic `pkg-config --libs --cflags Qt5Gui Qt5Widgets` -o wheeltest wheeltest.cpp
// g++ -std=c++11 `pkg-config --libs --cflags QtGui` -o wheeltest wheeltest.cpp

#include <QApplication>
#include <QElapsedTimer>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtDebug>

#define TXTCLASS QTextEdit
#define STR(t)  # t
#define STRING(t) STR(t)

#if TXTCLASS == QTextBrowser
#include <QTextBrowser>
#elif TXTCLASS == QTextEdit
#include <QTextEdit>
#endif

template <class QTxt>
class QTextWidget : public QTxt
{
public:
    QTextWidget() : QTxt(), accidentalModifier(false), lastWheelEventUnmodified(false)
    {
        qWarning() << Q_FUNC_INFO << "protected" << STRING(TXTCLASS) << "allocated:" << this;
    }
protected:
    void keyPressEvent(QKeyEvent *e)
    {
        qDebug() << "Keypress  " << e;
        QTxt::keyPressEvent(e);
    }

    void keyReleaseEvent(QKeyEvent *e)
    {
        qDebug() << "Keyrelease" << e;
        QTxt::keyReleaseEvent(e);
    }

    void wheelEvent(QWheelEvent *we) {
        bool skip = false;
        qint64 deltaT = lastWheelEvent.elapsed();
        Qt::KeyboardModifiers modState = we->modifiers();
        int canHScroll, canVScroll;
        QScrollBar *hScrollBar = QTxt::horizontalScrollBar(),
                    *vScrollBar = QTxt::verticalScrollBar();
        if (hScrollBar && hScrollBar->minimum() == hScrollBar->maximum()) {
            hScrollBar = NULL;
        }
        if (vScrollBar && vScrollBar->minimum() == vScrollBar->maximum()) {
            vScrollBar = NULL;
        }
        if (hScrollBar) {
            if (((hScrollBar->value() < hScrollBar->maximum()) || we->delta() > 0)
                && ((hScrollBar->value() > hScrollBar->minimum()) || we->delta() < 0)) {
                canHScroll = true;
            }
            else {
                canHScroll = false;
            }
        }
        else {
            canHScroll = -1;
        }
        if (vScrollBar) {
            if (((vScrollBar->value() < vScrollBar->maximum()) || we->delta() > 0)
                && ((vScrollBar->value() > vScrollBar->minimum()) || we->delta() < 0)) {
                canVScroll = true;
            }
            else {
                canVScroll = false;
            }
        }
        else {
            canVScroll = -1;
        }
        if (!(canHScroll && canVScroll)) {
            qDebug() << "scrollbar(s) at extreme(s): ignoring event" << we;
            we->ignore();
            return;
        }
        if (modState & Qt::ControlModifier) {
            // Pressing the Control/Command key within 200ms of the previous "unmodified" wheelevent
            // is not allowed to toggle on text zooming
            if (lastWheelEventUnmodified && deltaT < 200) {
                    accidentalModifier = true;
            }
            else {
                // hold the Control/Command key for 1s without scrolling to re-allow text zooming
                if (deltaT > 1000) {
                    accidentalModifier = false;
                }
            }
            lastWheelEventUnmodified = false;
//             if (qAbs(we->delta()) > 120) {
//                 qDebug() << "skipping because of high delta" << we->delta() << "dT=" << deltaT;
//                 skip = true;
//             }
//             /*else */if (deltaT < 60) {
//                 qDebug() << "skipping because of fast wheel dT=" << deltaT << "delta=" << we->delta();
//                 skip = true;
//             }
            if (accidentalModifier) {
                qDebug() << "skipping because of accidental keypress; dT=" << deltaT;
                skip = true;
            }
            if (!skip) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
                qDebug() << "accepting event" << we << "with phase" << we->phase();
#else
                qDebug() << "accepting event" << we;
#endif
            }
        } else {
            lastWheelEventUnmodified = true;
            accidentalModifier = false;
        }
        lastWheelEvent.start();
        if (skip) {
#if 1
            // if we leave the ControlModifier set, calling QScrollBar::event(we)
            // will lead to accelerated scroll. Removing the bit gives us normal scroll.
            // This could be under control of a user option.
            // Evidently the accelerated scroll shouldn't only happen inertially in that
            // case, though.
            modState &= ~Qt::ControlModifier;
            we->setModifiers(modState);
            if (canVScroll > 0 ) {
                vScrollBar->event(we);
			 return;
            }
            if (canHScroll > 0 ) {
                hScrollBar->event(we);
                qDebug() << "event handed off to horizontalScrollBar";
			 return;
            }
#else
            modState &= ~Qt::ControlModifier;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            QWheelEvent *lwe = new QWheelEvent(we->pos(), we->globalPos(), we->pixelDelta(), we->angleDelta(),
                                            we->delta(), we->orientation(), we->buttons(), modState, we->phase());
#else
            QWheelEvent *lwe = new QWheelEvent(we->pos(), we->globalPos(), we->delta(), we->buttons(), modState, we->orientation());
#endif
            if (lwe) {
                QTxt::wheelEvent(lwe);
                delete lwe;
                return;
            }
#endif
        }
        // we can get here with skip==true in case of failure to create a temporary event, so retest skip
        if (!skip) {
            QTxt::wheelEvent(we);
        }
    }
private:
    QElapsedTimer lastWheelEvent;
    bool accidentalModifier;
    bool lastWheelEventUnmodified;
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QTextWidget<TXTCLASS> *t = new QTextWidget<TXTCLASS>;
    QString s;
    for (int i = 0; i < 10; ++i) {
        s += "\nMinions ipsum aute velit hahaha voluptate me want bananaaa! Qui wiiiii pepete duis underweaaar. Daa tempor consequat bee do bee do bee do pepete nostrud incididunt belloo! Ut bananaaaa jeje. Hana dul sae po kass tulaliloo daa magna bappleees. Belloo! exercitation qui reprehenderit wiiiii. Bee do bee do bee do duis butt veniam ex aaaaaah po kass magna incididunt poulet tikka masala. Chasy nostrud pepete duis ut et laboris la bodaaa duis para tú."
        "\nSed sit amet bee do bee do bee do chasy poulet tikka masala bananaaaa labore exercitation duis. Para tú elit aliquip jiji. Daa belloo! Esse tank yuuu! Commodo potatoooo minim tatata bala tu. Esse exercitation ad eiusmod laboris aute gelatooo laboris belloo! Sit amet. Aute dolore aute baboiii labore. Po kass belloo! Laboris aaaaaah qui bananaaaa po kass tatata bala tu. Qui dolor consequat butt aute poulet tikka masala butt gelatooo consectetur tempor. Belloo! ad wiiiii para tú. Enim ut underweaaar nisi hana dul sae poopayee poopayee butt tulaliloo gelatooo."
        "\nNisi hahaha daa tempor jeje. Exercitation magna aute underweaaar para tú po kass daa nisi veniam duis ut. Occaecat aliquip exercitation ad bappleees para tú tempor ex voluptate. Consectetur officia ut po kass hana dul sae officia. Belloo! dolore labore hahaha bananaaaa hana dul sae uuuhhh hana dul sae dolor elit. Esse commodo me want bananaaa! Laboris hahaha la bodaaa quis. Para tú baboiii hana dul sae jiji labore baboiii la bodaaa esse duis officia. Bee do bee do bee do poulet tikka masala hana dul sae ullamco pepete occaecat elit. Bappleees po kass irure tatata bala tu potatoooo minim me want bananaaa! Incididunt commodo bananaaaa."
        "\nBaboiii officia consequat tempor aliquip poulet tikka masala reprehenderit para tú tulaliloo ad pepete. Baboiii la bodaaa belloo! Velit poulet tikka masala aliqua. Reprehenderit tank yuuu! Consectetur aaaaaah nisi veniam officia butt tatata bala tu consectetur duis. Tank yuuu! ex sed poopayee magna. Jeje incididunt labore laboris tulaliloo underweaaar. Aliquip minim bappleees la bodaaa aute.";
    }
    t->setWindowTitle("Protected against accidental text zooming");
    t->setText(s);
    t->resize(296,480);
    t->show();

    TXTCLASS qt;
    qt.setWindowTitle("Stock " STRING(TXTCLASS));
    qt.setText(s);
    qt.resize(296,480);
    qt.show();
    return a.exec();
}
