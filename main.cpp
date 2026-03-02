#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <qqml.h>

#include "control.h"
#include "imagedisplay.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qmlRegisterType<ImageDisplay>("Labeling", 1, 0, "ImageDisplay");

    Control control;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("control"), &control);
    const QUrl url(QStringLiteral("qrc:/Labeling/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);
    if (!engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (window) {
            window->showMaximized();
        }
    }

    return app.exec();
}
