#include "dsc_engine.h"
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QCoreApplication>

// We need to capture stdout/stderr from codec_main.
// Since codec_main uses printf/fprintf directly, we redirect
// stdout/stderr to a pipe before calling it.

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <poll.h>
#endif

static QString g_outputBuffer;

// Simple redirect: capture stdout/stderr to a temp file, then read it back.
static QString s_captureFile;

#ifdef _WIN32
static HANDLE s_hStdoutRead = NULL;
static HANDLE s_hStdoutWrite = NULL;
static HANDLE s_hStderrWrite = NULL;
static HANDLE s_hOldStdout = NULL;
static HANDLE s_hOldStderr = NULL;

static void startCapture()
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&s_hStdoutRead, &s_hStdoutWrite, &sa, 65536))
        return;
    SetHandleInformation(s_hStdoutRead, HANDLE_FLAG_INHERIT, 0);

    s_hOldStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    s_hOldStderr = GetStdHandle(STD_ERROR_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, s_hStdoutWrite);
    SetStdHandle(STD_ERROR_HANDLE, s_hStdoutWrite);
    s_hStderrWrite = s_hStdoutWrite;
}

static void stopCapture()
{
    if (s_hStdoutWrite) {
        FlushFileBuffers(s_hStdoutWrite);
        CloseHandle(s_hStdoutWrite);
        s_hStdoutWrite = NULL;
    }
    SetStdHandle(STD_OUTPUT_HANDLE, s_hOldStdout);
    SetStdHandle(STD_ERROR_HANDLE, s_hOldStderr);

    // Read captured data
    char buf[4096];
    DWORD bytesRead;
    while (ReadFile(s_hStdoutRead, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        g_outputBuffer += QString::fromUtf8(buf);
    }
    CloseHandle(s_hStdoutRead);
    s_hStdoutRead = NULL;
}
#else
static int s_pipeFd[2] = {-1, -1};
static int s_oldStdout = -1;
static int s_oldStderr = -1;

static void startCapture()
{
    if (pipe(s_pipeFd) == 0) {
        s_oldStdout = dup(STDOUT_FILENO);
        s_oldStderr = dup(STDERR_FILENO);
        dup2(s_pipeFd[1], STDOUT_FILENO);
        dup2(s_pipeFd[1], STDERR_FILENO);
    }
}

static void stopCapture()
{
    if (s_pipeFd[1] >= 0) {
        close(s_pipeFd[1]);
        s_pipeFd[1] = -1;
    }
    if (s_oldStdout >= 0) dup2(s_oldStdout, STDOUT_FILENO);
    if (s_oldStderr >= 0) dup2(s_oldStderr, STDERR_FILENO);

    char buf[4096];
    ssize_t n;
    while ((n = read(s_pipeFd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        g_outputBuffer += QString::fromUtf8(buf);
    }
    if (s_pipeFd[0] >= 0) { close(s_pipeFd[0]); s_pipeFd[0] = -1; }
    if (s_oldStdout >= 0) { close(s_oldStdout); s_oldStdout = -1; }
    if (s_oldStderr >= 0) { close(s_oldStderr); s_oldStderr = -1; }
}
#endif

DSCEngine::DSCEngine(QObject *parent) : QObject(parent)
{
}

static void call_codec_main(const QString &cfgFilePath, DSCEngine *engine)
{
    QByteArray path = cfgFilePath.toLocal8Bit();
    char *argv[] = { (char *)"DSC", (char *)"-F", path.data(), nullptr };
    int ret = codec_main(3, argv);
    emit engine->outputLine(g_outputBuffer);
    emit engine->finished(ret);
}

void DSCEngine::runEncodeDecode(const QString &cfgFilePath)
{
    emit started();
    g_outputBuffer.clear();
    startCapture();
    call_codec_main(cfgFilePath, this);
    stopCapture();
}

void DSCEngine::runEncode(const QString &cfgFilePath)
{
    emit started();
    g_outputBuffer.clear();
    startCapture();
    call_codec_main(cfgFilePath, this);
    stopCapture();
}

void DSCEngine::runDecode(const QString &cfgFilePath)
{
    emit started();
    g_outputBuffer.clear();
    startCapture();
    call_codec_main(cfgFilePath, this);
    stopCapture();
}
