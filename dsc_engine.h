#ifndef DSC_ENGINE_H
#define DSC_ENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>

// Forward declare C codec entry point
#ifdef __cplusplus
extern "C" {
#endif
int codec_main(int argc, char *argv[]);
#ifdef __cplusplus
}
#endif

class DSCEngine : public QObject
{
    Q_OBJECT
public:
    explicit DSCEngine(QObject *parent = nullptr);

    // Run DSC codec with the given parameters
    void runEncodeDecode(const QString &cfgFilePath);
    void runEncode(const QString &cfgFilePath);
    void runDecode(const QString &cfgFilePath);

signals:
    void started();
    void finished(int exitCode);
    void outputLine(const QString &line);
    void error(const QString &message);
};

#endif // DSC_ENGINE_H
