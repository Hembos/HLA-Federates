#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "panel.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    AppCore appCore;    // Create the application core with signals and slots

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    /* We load the object into the context to establish the connection,
     * and also define the name "appCore" by which the connection will occur
     * */
    context->setContextProperty("appCore", &appCore);

    const QUrl url(QStringLiteral("qrc:/panel.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
