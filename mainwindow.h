#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class DSCEngine;
class QGroupBox;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowseOutputDir();
    void onBrowseConfig();
    void onBrowseConfigFile();
    void onBrowseSrcDir();
    void onRun();
    void onStop();
    void onEngineOutput(const QString &text);
    void onEngineFinished(int exitCode);
    void onEngineStarted();

    // Config file load
    void onLoadConfig();
    void onSaveConfig();
    void onSaveConfigAs();
    void onNewConfig();
    void onTabChanged(int index);
    void loadConfigFromFile(const QString &path);

private:
    void createWidgets();
    void createMenuBar();
    QString modeString() const;
    void populateModeControls();
    bool validateInputs();
    void generateConfigFile(const QString &path);

    // Engine
    DSCEngine *m_engine;
    bool m_running;

    // Widgets - File I/O tab
    QLineEdit *m_srcDir;
    QLineEdit *m_outputDir;
    QPushButton *m_browseSrcDir;
    QPushButton *m_browseOutputDir;

    // Widgets - Codec Settings tab
    QComboBox *m_function;
    QSpinBox *m_bitsPerComponent;
    QDoubleSpinBox *m_bitsPerPixel;
    QSpinBox *m_picWidth;
    QSpinBox *m_picHeight;
    QSpinBox *m_sliceWidth;
    QSpinBox *m_sliceHeight;
    QComboBox *m_chromaFormat;
    QSpinBox *m_lineBufferBpc;
    QCheckBox *m_blockPredEnable;
    QCheckBox *m_vbrEnable;
    QCheckBox *m_useYuvInput;
    QCheckBox *m_simple422;
    QCheckBox *m_native422;
    QCheckBox *m_native420;

    // Widgets - File Format tab
    QSpinBox *m_dpxRPadEnds;
    QSpinBox *m_dpxRDatumOrder;
    QSpinBox *m_dpxRForceBe;
    QSpinBox *m_dpxWPadEnds;
    QSpinBox *m_dpxWDatumOrder;
    QSpinBox *m_dpxWForcePacking;
    QCheckBox *m_swapRGB;
    QCheckBox *m_swapRGBOut;
    QCheckBox *m_ppmOutput;
    QCheckBox *m_dpxOutput;
    QCheckBox *m_bmpOutput;
    QCheckBox *m_bmpDscOutput;
    QCheckBox *m_yuvOutput;
    QCheckBox *m_printPps;

    // Widgets - Rate Control tab
    QLineEdit *m_rcConfigFile;
    QPushButton *m_browseRcConfig;
    QSpinBox *m_rcModelSize;
    QSpinBox *m_initialXmitDelay;
    QSpinBox *m_initialDecDelay;
    QSpinBox *m_rcEdgeFactor;
    QSpinBox *m_rcQuantIncrLimit0;
    QSpinBox *m_rcQuantIncrLimit1;

    // Widgets - Output tab
    QTextEdit *m_outputLog;
    QPushButton *m_runButton;
    QPushButton *m_stopButton;

    // Config file path
    QLineEdit *m_configFileEdit;

    // Config file path for loading
    QString m_currentConfigPath;
};

#endif // MAINWINDOW_H
