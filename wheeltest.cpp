// g++ -std=c++11 -fpic `pkg-config --libs --cflags Qt5Gui Qt5Widgets` -o wheeltest wheeltest.cpp
// g++ -std=c++11 `pkg-config --libs --cflags QtGui` -o wheeltest wheeltest.cpp

#include <QApplication>
#include <QElapsedTimer>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtDebug>
#if QT_VERSION >= QT_VERSION_CHECK(5,2,0)
#include <QCommandLineParser>
#endif

#define TXTCLASS QTextEdit
#define STR(t)  # t
#define STRING(t) STR(t)

#include <QTextBrowser>
#include <QTextEdit>

#include <QIODevice>
#include <QDebug>

template <class QTxt>
class QTextWidget : public QTxt
{
public:
    QTextWidget(QWidget *parent=nullptr)
        : QTxt(parent)
        , dumpEvents(false)
        , accidentalModifier(false)
        , allowAcceleratedScroll(false)
        , lastWheelEventUnmodified(false)
        , lastEventSource(-1)
    {
        qWarning() << Q_FUNC_INFO << "protected" << QTxt::metaObject()->className() << "allocated:" << this;
        qerr.open(stderr, QIODevice::WriteOnly);
    }
    QTxt *instance()
    {
        return this;
    }
    bool acceleratedScrolling()
    {
        return allowAcceleratedScroll;
    }
    void setAcceleratedScrolling(bool val)
    {
        allowAcceleratedScroll = val;
    }

//     QMenu *createStandardContextMenu()
//     {
//         qWarning() << Q_FUNC_INFO;
//         return QTxt::createStandardContextMenu();
//     }
// 
//     QMenu *createStandardContextMenu(const QPoint &position)
//     {
//         qWarning() << Q_FUNC_INFO << "pos=" << position;
//         return QTxt::createStandardContextMenu(position);
//     }

protected:
//     void keyPressEvent(QKeyEvent *e)
//     {
//         qDebug() << "Keypress  " << e;
//         QTxt::keyPressEvent(e);
//     }
// 
//     void keyReleaseEvent(QKeyEvent *e)
//     {
//         qDebug() << "Keyrelease" << e;
//         QTxt::keyReleaseEvent(e);
//     }

    bool event(QEvent *e) {
        if (e->type() != QEvent::UpdateRequest && dumpEvents) {
            QDebug(&qerr) << e << endl;
        }
        return QTxt::event(e);
    }

    void contextMenuEvent(QContextMenuEvent* e)
    {
        if (dumpEvents) {
            QDebug(&qerr) << "contextmenu pos=" << e->pos() << "globalPos=" << e->globalPos();
        }
        QTxt::contextMenuEvent(e);
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
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        Qt::MouseEventSource s = we->source();
        if (s != lastEventSource) {
            qWarning() << "WheelEvent source is now" << s;
            lastEventSource = s;
        }
#endif
        if (!(canHScroll && canVScroll)) {
            qWarning() << "scrollbar(s) at extreme(s): ignoring event" << we;
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
//                 qWarning() << "skipping because of high delta" << we->delta() << "dT=" << deltaT;
//                 skip = true;
//             }
//             /*else */if (deltaT < 60) {
//                 qWarning() << "skipping because of fast wheel dT=" << deltaT << "delta=" << we->delta();
//                 skip = true;
//             }
            if (accidentalModifier) {
                qWarning() << "skipping because of accidental keypress; dT=" << deltaT;
                skip = true;
            }
            if (!skip) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
                qWarning() << "accepting event" << we << "with phase" << we->phase();
#else
                qWarning() << "accepting event" << we;
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
            if (!allowAcceleratedScroll) {
                modState &= ~Qt::ControlModifier;
                we->setModifiers(modState);
            }
            if (canVScroll > 0 ) {
                vScrollBar->event(we);
                return;
            }
            if (canHScroll > 0 ) {
                hScrollBar->event(we);
                qWarning() << "event handed off to horizontalScrollBar";
                return;
            }
#else
            modState &= ~Qt::ControlModifier;
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
            QWheelEvent lwe(we->pos(), we->globalPos(), we->pixelDelta(), we->angleDelta(),
                                            we->delta(), we->orientation(), we->buttons(), modState, we->phase(), Qt::MouseEventSynthesizedByApplication);
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            QWheelEvent lwe(we->pos(), we->globalPos(), we->pixelDelta(), we->angleDelta(),
                                            we->delta(), we->orientation(), we->buttons(), modState, we->phase());
#else
            QWheelEvent lwe(we->pos(), we->globalPos(), we->delta(), we->buttons(), modState, we->orientation());
#endif
            QTxt::wheelEvent(&lwe);
            return;
#endif
        }
        // we can get here with skip==true in case of failure to create a temporary event, so retest skip
        if (!skip) {
            QTxt::wheelEvent(we);
        } else {
            we->ignore();
        }
    }

public:
    bool dumpEvents;
private:
    QElapsedTimer lastWheelEvent;
    bool accidentalModifier;
    bool allowAcceleratedScroll;
    bool lastWheelEventUnmodified;
    int lastEventSource;
    QFile qerr;
};

class QTextEditWidget : public QTextWidget<QTextEdit>
{
    Q_OBJECT
public:
    QTextEditWidget(QWidget *parent=nullptr)
        : QTextWidget<QTextEdit>(parent)
    {
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(showCursorPosition()));
        title = windowTitle();
    }

    void setWindowTitle(const QString &txt)
    {
        title = txt;
        QTextEdit::setWindowTitle(txt);
    }

public slots:
    void showCursorPosition()
    {
        const auto cursorPos = textCursor();
        QTextEdit::setWindowTitle(QString("[%1,%2] ").arg(cursorPos.blockNumber()).arg(cursorPos.positionInBlock()) + title);
    }

private:
    QString title;
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    bool useTextEdit = false, allowAccelerated = false, dumpEvents = false;
#if QT_VERSION >= QT_VERSION_CHECK(5,2,0)
    QCommandLineParser parser;
    parser.setApplicationDescription(a.translate("wheeltest", "testing protection against accidental zooming/fast-scrolling"));
    const QCommandLineOption qteOption(QStringLiteral("QTextEdit"), QStringLiteral("Protect a QTextEdit widget"));
    const QCommandLineOption qtbOption(QStringLiteral("QTextBrowser"), QStringLiteral("Protect a QTextBrowser widget (default)"));
    const QCommandLineOption accelOption(QStringLiteral("accelerated"), QStringLiteral("Pressing the Ctrl/Command key during scrolling activates activated scrolling"));
    const QCommandLineOption dumpOption(QStringLiteral("dumpEvents"), QStringLiteral("print events on the terminal"));
    parser.addOptions({qteOption, qtbOption, accelOption, dumpOption});
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(a);
    useTextEdit = parser.isSet("QTextEdit");
    allowAccelerated = parser.isSet("accelerated");
    dumpEvents= parser.isSet("dumpEvents");
#else
    for (int i = 1 ; i < argc ; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--QTextEdit") == 0) {
            useTextEdit = true;
        } else if (strcmp(arg, "--QTextBrowser") == 0) {
            useTextEdit = false;
        } else if (strcmp(arg, "--accelerated") == 0) {
            allowAccelerated = true;
        } else if (strcmp(arg, "--dumpEvents") == 0) {
            dumpEvents = true;
        }
    }
#endif

    QString s;
    for (int i = 0; i < 10; ++i) {
        s += "\nMinions ipsum aute velit hahaha voluptate me want bananaaa! Qui wiiiii pepete duis underweaaar. Daa tempor consequat bee do bee do bee do pepete nostrud incididunt belloo! Ut bananaaaa jeje. Hana dul sae po kass tulaliloo daa magna bappleees. Belloo! exercitation qui reprehenderit wiiiii. Bee do bee do bee do duis butt veniam ex aaaaaah po kass magna incididunt poulet tikka masala. Chasy nostrud pepete duis ut et laboris la bodaaa duis para tú."
        "\nSed sit amet bee do bee do bee do chasy poulet tikka masala bananaaaa labore exercitation duis. Para tú elit aliquip jiji. Daa belloo! Esse tank yuuu! Commodo potatoooo minim tatata bala tu. Esse exercitation ad eiusmod laboris aute gelatooo laboris belloo! Sit amet. Aute dolore aute baboiii labore. Po kass belloo! Laboris aaaaaah qui bananaaaa po kass tatata bala tu. Qui dolor consequat butt aute poulet tikka masala butt gelatooo consectetur tempor. Belloo! ad wiiiii para tú. Enim ut underweaaar nisi hana dul sae poopayee poopayee butt tulaliloo gelatooo."
        "\nNisi hahaha daa tempor jeje. Exercitation magna aute underweaaar para tú po kass daa nisi veniam duis ut. Occaecat aliquip exercitation ad bappleees para tú tempor ex voluptate. Consectetur officia ut po kass hana dul sae officia. Belloo! dolore labore hahaha bananaaaa hana dul sae uuuhhh hana dul sae dolor elit. Esse commodo me want bananaaa! Laboris hahaha la bodaaa quis. Para tú baboiii hana dul sae jiji labore baboiii la bodaaa esse duis officia. Bee do bee do bee do poulet tikka masala hana dul sae ullamco pepete occaecat elit. Bappleees po kass irure tatata bala tu potatoooo minim me want bananaaa! Incididunt commodo bananaaaa."
        "\nBaboiii officia consequat tempor aliquip poulet tikka masala reprehenderit para tú tulaliloo ad pepete. Baboiii la bodaaa belloo! Velit poulet tikka masala aliqua. Reprehenderit tank yuuu! Consectetur aaaaaah nisi veniam officia butt tatata bala tu consectetur duis. Tank yuuu! ex sed poopayee magna. Jeje incididunt labore laboris tulaliloo underweaaar. Aliquip minim bappleees la bodaaa aute.";
    }

    // QTextBrowser inherits QTextEdit so we can do this:
    QTextEdit *t, *qt;
    if (useTextEdit) {
        QTextEditWidget *tmp = new QTextEditWidget;
        tmp->setWindowTitle("Protected against accidental text zooming");
        if (allowAccelerated) {
            tmp->setAcceleratedScrolling(true);
        }
        tmp->dumpEvents = dumpEvents;
        t = tmp->instance();
        qt = new QTextEdit;
        qt->setWindowTitle("Stock QTextEdit");
    } else {
        QTextWidget<QTextBrowser> *tmp = new QTextWidget<QTextBrowser>;
        tmp->setWindowTitle("Protected against accidental text zooming");
        if (allowAccelerated) {
            tmp->setAcceleratedScrolling(true);
        }
        tmp->dumpEvents = dumpEvents;
        t = tmp->instance();
        qt = new QTextBrowser;
        qt->setWindowTitle("Stock QTextBrowser");
    }
    t->setText(s);
    t->resize(296,480);
    t->setReadOnly(false);

    qt->setText(s);
    qt->resize(306,470);
    qt->setReadOnly(false);

    qt->show();
    t->show();

    return a.exec();
}

#include "wheeltest.moc"
