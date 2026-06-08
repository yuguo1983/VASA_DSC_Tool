#include "mainwindow.h"
#include "dsc_engine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QApplication>
#include <QRegularExpression>
#include <QFileInfo>
#include <QThread>
#include <QDir>

/* ================================================================
 *  Helpers
 * ================================================================ */

static QWidget *formRow(const QString &label, QWidget *w, int labelW = 200)
{
    auto *row = new QWidget;
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 2, 0, 2);
    auto *lbl = new QLabel(label);
    lbl->setFixedWidth(labelW);
    lay->addWidget(lbl);
    lay->addWidget(w, 1);
    return row;
}

static QWidget *formRowCheck(QCheckBox *cb)
{
    auto *row = new QWidget;
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 2, 0, 2);
    lay->addSpacing(200);
    lay->addWidget(cb, 1);
    return row;
}

/* ================================================================
 *  Construction
 * ================================================================ */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_engine(new DSCEngine(this)), m_running(false)
{
    setWindowTitle(tr("DSC 1.2 Reference Codec"));
    resize(960, 720);
    createMenuBar();
    createWidgets();
    connect(m_engine, &DSCEngine::started,  this, &MainWindow::onEngineStarted);
    connect(m_engine, &DSCEngine::finished, this, &MainWindow::onEngineFinished);
    connect(m_engine, &DSCEngine::outputLine, this, &MainWindow::onEngineOutput);
}

MainWindow::~MainWindow() = default;

/* ================================================================
 *  Menu bar
 * ================================================================ */

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *newAct = fileMenu->addAction(tr("&New Config"));
    newAct->setShortcut(QKeySequence::New);
    connect(newAct, &QAction::triggered, this, &MainWindow::onNewConfig);

    auto *openAct = fileMenu->addAction(tr("&Open Config..."));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::onLoadConfig);

    auto *saveAct = fileMenu->addAction(tr("&Save Config"));
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onSaveConfig);

    auto *saveAsAct = fileMenu->addAction(tr("Save Config &As..."));
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::onSaveConfigAs);

    fileMenu->addSeparator();

    auto *exitAct = fileMenu->addAction(tr("E&xit"));
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
}

/* ================================================================
 *  All widgets
 * ================================================================ */

void MainWindow::createWidgets()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);

    /* -- Config file bar -- */
    auto *cfgRow = new QWidget;
    auto *cfgLay = new QHBoxLayout(cfgRow);
    cfgLay->setContentsMargins(0, 4, 0, 4);
    cfgLay->addWidget(new QLabel(tr("Config:")));
    m_configFileEdit = new QLineEdit;
    cfgLay->addWidget(m_configFileEdit, 1);
    auto *btnCfg = new QPushButton(tr("Browse..."));
    connect(btnCfg, &QPushButton::clicked, this, &MainWindow::onBrowseConfigFile);
    cfgLay->addWidget(btnCfg);
    mainLayout->addWidget(cfgRow);

    /* -- Tabs -- */
    auto *tabs = new QTabWidget;
    mainLayout->addWidget(tabs, 1);

    /* ============================================================
     *  Tab 1 : File I/O
     * ============================================================ */
    {
        auto *t = new QWidget;
        auto *v = new QVBoxLayout(t);
        v->setAlignment(Qt::AlignTop);

        // SRC_DIR (auto-scan directory)
        m_srcDir = new QLineEdit;
        auto *bs = new QPushButton(tr("Browse..."));
        connect(bs, &QPushButton::clicked, this, &MainWindow::onBrowseSrcDir);
        auto *r0s = new QWidget; auto *r0sl = new QHBoxLayout(r0s);
        r0sl->addWidget(new QLabel(tr("SRC_DIR (input directory):")), 1); r0sl->addWidget(m_srcDir, 3); r0sl->addWidget(bs);
        v->addWidget(r0s);

        m_outputDir = new QLineEdit;
        auto *bd = new QPushButton(tr("Browse..."));
        connect(bd, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDir);
        auto *r1 = new QWidget; auto *r1l = new QHBoxLayout(r1);
        r1l->addWidget(new QLabel(tr("Output Directory:")), 1); r1l->addWidget(m_outputDir, 3); r1l->addWidget(bd);
        v->addWidget(r1);

        v->addStretch();
        tabs->addTab(t, tr("File I/O"));
    }

    /* ============================================================
     *  Tab 2 : Codec Settings
     * ============================================================ */
    {
        auto *t = new QWidget;
        auto *v = new QVBoxLayout(t);
        v->setAlignment(Qt::AlignTop);

        m_function = new QComboBox;
        m_function->addItems({tr("0  -  Encode + Decode"), tr("1  -  Encode Only (writes .dsc)"), tr("2  -  Decode Only")});
        connect(m_function, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTabChanged);
        v->addWidget(formRow(tr("FUNCTION (Mode)"), m_function));

        m_bitsPerComponent = new QSpinBox; m_bitsPerComponent->setRange(8, 16); m_bitsPerComponent->setValue(8);
        v->addWidget(formRow(tr("BITS_PER_COMPONENT"), m_bitsPerComponent));

        m_bitsPerPixel = new QDoubleSpinBox; m_bitsPerPixel->setRange(4.0, 15.0);
        m_bitsPerPixel->setDecimals(2); m_bitsPerPixel->setValue(12.0); m_bitsPerPixel->setSingleStep(0.5);
        v->addWidget(formRow(tr("BITS_PER_PIXEL"), m_bitsPerPixel));

        m_picWidth  = new QSpinBox; m_picWidth->setRange(0, 65535);  m_picWidth->setValue(0);
        v->addWidget(formRow(tr("PIC_WIDTH  (0 = auto from input)"), m_picWidth));

        m_picHeight = new QSpinBox; m_picHeight->setRange(0, 65535); m_picHeight->setValue(0);
        v->addWidget(formRow(tr("PIC_HEIGHT (0 = auto from input)"), m_picHeight));

        m_sliceWidth  = new QSpinBox; m_sliceWidth->setRange(0, 65535);  m_sliceWidth->setValue(0);
        v->addWidget(formRow(tr("SLICE_WIDTH  (0 = full picture)"), m_sliceWidth));

        m_sliceHeight = new QSpinBox; m_sliceHeight->setRange(0, 65535); m_sliceHeight->setValue(0);
        v->addWidget(formRow(tr("SLICE_HEIGHT (0 = full picture)"), m_sliceHeight));

        m_chromaFormat = new QComboBox;
        m_chromaFormat->addItems({tr("RGB"), tr("YCbCr 4:4:4"), tr("YCbCr 4:2:2 (Simple)"), tr("YCbCr 4:2:2 (Native)"), tr("YCbCr 4:2:0 (Native)")});
        v->addWidget(formRow(tr("Chroma Format"), m_chromaFormat));

        m_lineBufferBpc = new QSpinBox; m_lineBufferBpc->setRange(8, 16); m_lineBufferBpc->setValue(16);
        v->addWidget(formRow(tr("LINE_BUFFER_BPC"), m_lineBufferBpc));

        m_blockPredEnable = new QCheckBox(tr("BLOCK_PRED_ENABLE"));
        m_blockPredEnable->setChecked(true);
        v->addWidget(formRowCheck(m_blockPredEnable));

        m_vbrEnable = new QCheckBox(tr("VBR_ENABLE (Variable Bit-Rate)"));
        v->addWidget(formRowCheck(m_vbrEnable));

        m_useYuvInput = new QCheckBox(tr("USE_YUV_INPUT"));
        v->addWidget(formRowCheck(m_useYuvInput));

        m_simple422 = new QCheckBox(tr("SIMPLE_422"));
        v->addWidget(formRowCheck(m_simple422));

        m_native422 = new QCheckBox(tr("NATIVE_422"));
        v->addWidget(formRowCheck(m_native422));

        m_native420 = new QCheckBox(tr("NATIVE_420"));
        v->addWidget(formRowCheck(m_native420));

        v->addStretch();
        tabs->addTab(t, tr("Codec"));
    }

    /* ============================================================
     *  Tab 3 : File Format
     * ============================================================ */
    {
        auto *t = new QWidget;
        auto *v = new QVBoxLayout(t);
        v->setAlignment(Qt::AlignTop);

        auto *grpR = new QGroupBox(tr("DPX Read"));
        auto *glR = new QVBoxLayout(grpR);
        m_dpxRPadEnds  = new QSpinBox; m_dpxRPadEnds->setRange(0,1);  m_dpxRPadEnds->setValue(1);
        glR->addWidget(formRow(tr("DPXR_PAD_ENDS"),      m_dpxRPadEnds,  180));
        m_dpxRDatumOrder = new QSpinBox; m_dpxRDatumOrder->setRange(0,1); m_dpxRDatumOrder->setValue(1);
        glR->addWidget(formRow(tr("DPXR_DATUM_ORDER"),    m_dpxRDatumOrder, 180));
        m_dpxRForceBe  = new QSpinBox; m_dpxRForceBe->setRange(0,1);   m_dpxRForceBe->setValue(0);
        glR->addWidget(formRow(tr("DPXR_FORCE_BE"),       m_dpxRForceBe,  180));
        v->addWidget(grpR);

        auto *grpW = new QGroupBox(tr("DPX Write"));
        auto *glW = new QVBoxLayout(grpW);
        m_dpxWPadEnds   = new QSpinBox; m_dpxWPadEnds->setRange(0,1);   m_dpxWPadEnds->setValue(1);
        glW->addWidget(formRow(tr("DPXW_PAD_ENDS"),       m_dpxWPadEnds,  180));
        m_dpxWDatumOrder = new QSpinBox; m_dpxWDatumOrder->setRange(0,1); m_dpxWDatumOrder->setValue(1);
        glW->addWidget(formRow(tr("DPXW_DATUM_ORDER"),     m_dpxWDatumOrder, 180));
        m_dpxWForcePacking = new QSpinBox; m_dpxWForcePacking->setRange(0,1); m_dpxWForcePacking->setValue(1);
        glW->addWidget(formRow(tr("DPXW_FORCE_PACKING"),   m_dpxWForcePacking, 180));
        v->addWidget(grpW);

        m_swapRGB    = new QCheckBox(tr("SWAP_R_AND_B"));          m_swapRGB->setChecked(true);
        m_swapRGBOut = new QCheckBox(tr("SWAP_R_AND_B_OUT"));      m_swapRGBOut->setChecked(true);
        m_ppmOutput  = new QCheckBox(tr("PPM_FILE_OUTPUT"));
        m_dpxOutput  = new QCheckBox(tr("DPX_FILE_OUTPUT"));        m_dpxOutput->setChecked(true);
        m_bmpOutput  = new QCheckBox(tr("BMP_FILE_OUTPUT"));
        m_bmpDscOutput = new QCheckBox(tr("BMP_DSC_OUTPUT (embed DSC in BMP)"));
        m_yuvOutput  = new QCheckBox(tr("YUV_FILE_OUTPUT"));
        m_printPps   = new QCheckBox(tr("PRINT_PPS (log PPS values)"));

        v->addWidget(formRowCheck(m_swapRGB));
        v->addWidget(formRowCheck(m_swapRGBOut));
        v->addWidget(formRowCheck(m_ppmOutput));
        v->addWidget(formRowCheck(m_dpxOutput));
        v->addWidget(formRowCheck(m_bmpOutput));
        v->addWidget(formRowCheck(m_bmpDscOutput));
        v->addWidget(formRowCheck(m_yuvOutput));
        v->addWidget(formRowCheck(m_printPps));

        v->addStretch();
        tabs->addTab(t, tr("Format"));
    }

    /* ============================================================
     *  Tab 4 : Rate Control
     * ============================================================ */
    {
        auto *t = new QWidget;
        auto *v = new QVBoxLayout(t);
        v->setAlignment(Qt::AlignTop);

        m_rcConfigFile = new QLineEdit;
        auto *br = new QPushButton(tr("Browse..."));
        connect(br, &QPushButton::clicked, this, &MainWindow::onBrowseConfig);
        auto *rr = new QWidget; auto *rrl = new QHBoxLayout(rr);
        rrl->addWidget(m_rcConfigFile, 1); rrl->addWidget(br);
        v->addWidget(formRow(tr("INCLUDE (RC .cfg file)"), rr));

        m_rcModelSize        = new QSpinBox; m_rcModelSize->setRange(64,4096);    m_rcModelSize->setValue(640);
        m_initialXmitDelay   = new QSpinBox; m_initialXmitDelay->setRange(0,1023); m_initialXmitDelay->setValue(112);
        m_initialDecDelay    = new QSpinBox; m_initialDecDelay->setRange(0,1023);  m_initialDecDelay->setValue(128);
        m_rcEdgeFactor       = new QSpinBox; m_rcEdgeFactor->setRange(0,15);       m_rcEdgeFactor->setValue(4);
        m_rcQuantIncrLimit0  = new QSpinBox; m_rcQuantIncrLimit0->setRange(0,31);   m_rcQuantIncrLimit0->setValue(7);
        m_rcQuantIncrLimit1  = new QSpinBox; m_rcQuantIncrLimit1->setRange(0,31);   m_rcQuantIncrLimit1->setValue(4);

        v->addWidget(formRow(tr("RC_MODEL_SIZE"),        m_rcModelSize));
        v->addWidget(formRow(tr("INITIAL_XMIT_DELAY"),   m_initialXmitDelay));
        v->addWidget(formRow(tr("INITIAL_DEC_DELAY"),    m_initialDecDelay));
        v->addWidget(formRow(tr("RC_EDGE_FACTOR"),       m_rcEdgeFactor));
        v->addWidget(formRow(tr("RC_QUANT_INCR_LIMIT0"), m_rcQuantIncrLimit0));
        v->addWidget(formRow(tr("RC_QUANT_INCR_LIMIT1"), m_rcQuantIncrLimit1));

        v->addStretch();
        tabs->addTab(t, tr("Rate Control"));
    }

    /* ============================================================
     *  Tab 5 : Output / Run
     * ============================================================ */
    {
        auto *t = new QWidget;
        auto *v = new QVBoxLayout(t);

        m_outputLog = new QTextEdit;
        m_outputLog->setReadOnly(true);
        m_outputLog->setFont(QFont("Consolas", 9));
        v->addWidget(m_outputLog, 1);

        auto *btnRow = new QWidget;
        auto *btnLay = new QHBoxLayout(btnRow);
        btnLay->setContentsMargins(0, 6, 0, 0);

        m_runButton  = new QPushButton(tr("\u25B6  Run"));
        m_runButton->setStyleSheet("background:#4CAF50;color:#fff;font-weight:bold;padding:8px 28px;border-radius:4px;font-size:13px;");
        connect(m_runButton, &QPushButton::clicked, this, &MainWindow::onRun);
        btnLay->addWidget(m_runButton);

        m_stopButton = new QPushButton(tr("\u25A0  Stop"));
        m_stopButton->setEnabled(false);
        m_stopButton->setStyleSheet("background:#f44336;color:#fff;font-weight:bold;padding:8px 28px;border-radius:4px;font-size:13px;");
        connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
        btnLay->addWidget(m_stopButton);

        btnLay->addStretch();
        v->addWidget(btnRow);

        tabs->addTab(t, tr("Run / Log"));
    }
}

/* ================================================================
 *  Browse callbacks
 * ================================================================ */

void MainWindow::onBrowseOutputDir()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));
    if (!d.isEmpty()) m_outputDir->setText(d);
}

void MainWindow::onBrowseConfig()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Select RC Config File"),
        QString(), tr("Config Files (*.cfg);;All Files (*)"));
    if (!f.isEmpty()) m_rcConfigFile->setText(f);
}

void MainWindow::onBrowseConfigFile()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Open Config File"),
        QString(), tr("Config Files (*.cfg);;All Files (*)"));
    if (!f.isEmpty()) {
        m_configFileEdit->setText(f);
        loadConfigFromFile(f);
    }
}

void MainWindow::onBrowseSrcDir()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select Input Directory"));
    if (!d.isEmpty()) m_srcDir->setText(d);
}

/* ================================================================
 *  Config load / save / new
 * ================================================================ */

static void setInt(QSpinBox *sp, const QString &v) { bool ok; int n = v.toInt(&ok); if (ok) sp->setValue(n); }
static void setDbl(QDoubleSpinBox *sp, const QString &v) { bool ok; double n = v.toDouble(&ok); if (ok) sp->setValue(n); }
static void setChk(QCheckBox *cb, const QString &v) { cb->setChecked(v.toInt()); }

void MainWindow::loadConfigFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open config file:\n%1").arg(path));
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) continue;
        int ci = line.indexOf("//");
        if (ci >= 0) line.truncate(ci);
        line = line.trimmed();
        auto parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 2) continue;
        QString key = parts[0].toUpper();
        QString val = parts[1];

        if      (key == "FUNCTION")            m_function->setCurrentIndex(val.toInt());
        else if (key == "BITS_PER_COMPONENT")  setInt(m_bitsPerComponent,  val);
        else if (key == "BITS_PER_PIXEL")      setDbl(m_bitsPerPixel,      val);
        else if (key == "PIC_WIDTH")           setInt(m_picWidth,          val);
        else if (key == "PIC_HEIGHT")          setInt(m_picHeight,         val);
        else if (key == "SLICE_WIDTH")         setInt(m_sliceWidth,        val);
        else if (key == "SLICE_HEIGHT")        setInt(m_sliceHeight,       val);
        else if (key == "LINE_BUFFER_BPC")     setInt(m_lineBufferBpc,     val);
        else if (key == "USE_YUV_INPUT")       setChk(m_useYuvInput,       val);
        else if (key == "BLOCK_PRED_ENABLE")   setChk(m_blockPredEnable,   val);
        else if (key == "VBR_ENABLE")          setChk(m_vbrEnable,         val);
        else if (key == "SIMPLE_422")          setChk(m_simple422,         val);
        else if (key == "NATIVE_422")          setChk(m_native422,         val);
        else if (key == "NATIVE_420")          setChk(m_native420,         val);
        else if (key == "DPXR_PAD_ENDS")       setInt(m_dpxRPadEnds,       val);
        else if (key == "DPXR_DATUM_ORDER")    setInt(m_dpxRDatumOrder,    val);
        else if (key == "DPXR_FORCE_BE")       setInt(m_dpxRForceBe,       val);
        else if (key == "DPXW_PAD_ENDS")       setInt(m_dpxWPadEnds,       val);
        else if (key == "DPXW_DATUM_ORDER")    setInt(m_dpxWDatumOrder,    val);
        else if (key == "DPXW_FORCE_PACKING")  setInt(m_dpxWForcePacking,  val);
        else if (key == "SWAP_R_AND_B")        setChk(m_swapRGB,           val);
        else if (key == "SWAP_R_AND_B_OUT")    setChk(m_swapRGBOut,        val);
        else if (key == "PPM_FILE_OUTPUT")     setChk(m_ppmOutput,         val);
        else if (key == "DPX_FILE_OUTPUT")     setChk(m_dpxOutput,         val);
        else if (key == "BMP_FILE_OUTPUT")     setChk(m_bmpOutput,         val);
        else if (key == "BMP_DSC_OUTPUT")      setChk(m_bmpDscOutput,      val);
        else if (key == "YUV_FILE_OUTPUT")     setChk(m_yuvOutput,         val);
        else if (key == "PRINT_PPS")           setChk(m_printPps,          val);
        else if (key == "RC_MODEL_SIZE")       setInt(m_rcModelSize,       val);
        else if (key == "INITIAL_DELAY")       setInt(m_initialXmitDelay,  val);
        else if (key == "INITIAL_FULLNESS_OFFSET") setInt(m_initialDecDelay, val);
        else if (key == "RC_EDGE_FACTOR")      setInt(m_rcEdgeFactor,      val);
        else if (key == "RC_QUANT_INCR_LIMIT0")setInt(m_rcQuantIncrLimit0, val);
        else if (key == "RC_QUANT_INCR_LIMIT1")setInt(m_rcQuantIncrLimit1, val);
        else if (key == "INCLUDE")             m_rcConfigFile->setText(val);
        else if (key == "SRC_DIR")             m_srcDir->setText(val);
    }
    m_currentConfigPath = path;
    setWindowTitle(tr("DSC 1.2 Reference Codec - %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onLoadConfig()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Open Config File"),
        QString(), tr("Config Files (*.cfg);;All Files (*)"));
    if (!f.isEmpty()) { m_configFileEdit->setText(f); loadConfigFromFile(f); }
}

void MainWindow::onSaveConfig()
{
    QString path = m_currentConfigPath.isEmpty()
        ? QFileDialog::getSaveFileName(this, tr("Save Config"), QString(), tr("Config Files (*.cfg)"))
        : m_currentConfigPath;
    if (path.isEmpty()) return;
    generateConfigFile(path);
    m_currentConfigPath = path;
    m_configFileEdit->setText(path);
    setWindowTitle(tr("DSC 1.2 Reference Codec - %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onSaveConfigAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Config As"), QString(), tr("Config Files (*.cfg)"));
    if (path.isEmpty()) return;
    generateConfigFile(path);
    m_currentConfigPath = path;
    m_configFileEdit->setText(path);
    setWindowTitle(tr("DSC 1.2 Reference Codec - %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onNewConfig()
{
    m_currentConfigPath.clear();
    m_configFileEdit->clear();
    m_srcDir->clear();
    m_outputDir->clear();
    m_rcConfigFile->clear();
    m_function->setCurrentIndex(0);
    m_bitsPerComponent->setValue(8);  m_bitsPerPixel->setValue(12.0);
    m_picWidth->setValue(0);  m_picHeight->setValue(0);
    m_sliceWidth->setValue(0);  m_sliceHeight->setValue(0);
    m_chromaFormat->setCurrentIndex(0);  m_lineBufferBpc->setValue(16);
    m_blockPredEnable->setChecked(true);  m_vbrEnable->setChecked(false);
    m_useYuvInput->setChecked(false);  m_simple422->setChecked(false);
    m_native422->setChecked(false);  m_native420->setChecked(false);
    m_dpxRPadEnds->setValue(1);  m_dpxRDatumOrder->setValue(1);  m_dpxRForceBe->setValue(0);
    m_dpxWPadEnds->setValue(1);  m_dpxWDatumOrder->setValue(1);  m_dpxWForcePacking->setValue(1);
    m_swapRGB->setChecked(true);  m_swapRGBOut->setChecked(true);
    m_ppmOutput->setChecked(false);  m_dpxOutput->setChecked(true);
    m_bmpOutput->setChecked(false);  m_bmpDscOutput->setChecked(false);
    m_yuvOutput->setChecked(false);  m_printPps->setChecked(false);
    m_rcModelSize->setValue(640);  m_initialXmitDelay->setValue(112);
    m_initialDecDelay->setValue(128);  m_rcEdgeFactor->setValue(4);
    m_rcQuantIncrLimit0->setValue(7);  m_rcQuantIncrLimit1->setValue(4);
    m_outputLog->clear();
    setWindowTitle(tr("DSC 1.2 Reference Codec"));
}

/* ================================================================
 *  Generate .cfg from current UI values
 * ================================================================ */

void MainWindow::generateConfigFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot write config file:\n%1").arg(path));
        return;
    }
    QTextStream out(&file);

    auto chk = [&](bool v){ return v ? "1" : "0"; };

    out << "// DSC 1.2 Codec Configuration — generated by GUI\n\n";
    out << "FUNCTION          " << m_function->currentIndex() << "\n";
    out << "SRC_DIR           " << m_srcDir->text() << "\n";
    out << "OUT_DIR           " << m_outputDir->text() << "\n";
    out << "BITS_PER_COMPONENT   " << m_bitsPerComponent->value() << "\n";
    out << "BITS_PER_PIXEL       " << QString::number(m_bitsPerPixel->value(), 'f', 2) << "\n";
    if (m_picWidth->value()  > 0) out << "PIC_WIDTH          " << m_picWidth->value()  << "\n";
    if (m_picHeight->value() > 0) out << "PIC_HEIGHT         " << m_picHeight->value() << "\n";
    if (m_sliceWidth->value()  > 0) out << "SLICE_WIDTH      " << m_sliceWidth->value()  << "\n";
    if (m_sliceHeight->value() > 0) out << "SLICE_HEIGHT     " << m_sliceHeight->value() << "\n";
    out << "LINE_BUFFER_BPC      " << m_lineBufferBpc->value() << "\n";
    out << "USE_YUV_INPUT        " << chk(m_useYuvInput->isChecked()) << "\n";
    out << "BLOCK_PRED_ENABLE    " << chk(m_blockPredEnable->isChecked()) << "\n";
    out << "VBR_ENABLE           " << chk(m_vbrEnable->isChecked()) << "\n";
    out << "SIMPLE_422           " << chk(m_simple422->isChecked()) << "\n";
    out << "NATIVE_422           " << chk(m_native422->isChecked()) << "\n";
    out << "NATIVE_420           " << chk(m_native420->isChecked()) << "\n\n";

    out << "// DPX read\n";
    out << "DPXR_PAD_ENDS        " << m_dpxRPadEnds->value()  << "\n";
    out << "DPXR_DATUM_ORDER     " << m_dpxRDatumOrder->value() << "\n";
    out << "DPXR_FORCE_BE        " << m_dpxRForceBe->value()  << "\n";
    out << "// DPX write\n";
    out << "DPXW_PAD_ENDS        " << m_dpxWPadEnds->value()   << "\n";
    out << "DPXW_DATUM_ORDER     " << m_dpxWDatumOrder->value() << "\n";
    out << "DPXW_FORCE_PACKING   " << m_dpxWForcePacking->value() << "\n";
    out << "SWAP_R_AND_B         " << chk(m_swapRGB->isChecked())   << "\n";
    out << "SWAP_R_AND_B_OUT     " << chk(m_swapRGBOut->isChecked()) << "\n\n";

    out << "PPM_FILE_OUTPUT      " << chk(m_ppmOutput->isChecked()) << "\n";
    out << "DPX_FILE_OUTPUT      " << chk(m_dpxOutput->isChecked()) << "\n";
    out << "BMP_FILE_OUTPUT      " << chk(m_bmpOutput->isChecked()) << "\n";
    out << "BMP_DSC_OUTPUT       " << chk(m_bmpDscOutput->isChecked()) << "\n";
    out << "YUV_FILE_OUTPUT      " << chk(m_yuvOutput->isChecked()) << "\n";
    if (m_printPps->isChecked())
        out << "PRINT_PPS\n";
    out << "\n";

    out << "RC_MODEL_SIZE          " << m_rcModelSize->value()        << "\n";
    out << "INITIAL_DELAY          " << m_initialXmitDelay->value()   << "\n";
    out << "INITIAL_FULLNESS_OFFSET " << m_initialDecDelay->value()   << "\n";
    out << "RC_EDGE_FACTOR         " << m_rcEdgeFactor->value()       << "\n";
    out << "RC_QUANT_INCR_LIMIT0   " << m_rcQuantIncrLimit0->value()  << "\n";
    out << "RC_QUANT_INCR_LIMIT1   " << m_rcQuantIncrLimit1->value()  << "\n";

    // INCLUDE must be last so it can override the above RC values
    if (!m_rcConfigFile->text().isEmpty())
        out << "\nINCLUDE              " << m_rcConfigFile->text() << "\n";
}

/* ================================================================
 *  Mode change → enable/disable controls
 * ================================================================ */

void MainWindow::onTabChanged(int)
{
    // Function == 2 (decode): disable encoder-only controls
    bool enc = m_function->currentIndex() != 2;
    m_bitsPerComponent->setEnabled(enc);
    m_bitsPerPixel->setEnabled(enc);
    m_picWidth->setEnabled(enc);
    m_picHeight->setEnabled(enc);
    m_sliceWidth->setEnabled(enc);
    m_sliceHeight->setEnabled(enc);
    m_chromaFormat->setEnabled(enc);
    m_lineBufferBpc->setEnabled(enc);
    m_useYuvInput->setEnabled(enc);
    m_blockPredEnable->setEnabled(enc);
    m_vbrEnable->setEnabled(enc);
    m_simple422->setEnabled(enc);
    m_native422->setEnabled(enc);
    m_native420->setEnabled(enc);
}

/* ================================================================
 *  Validate & Run
 * ================================================================ */

bool MainWindow::validateInputs()
{
    if (m_srcDir->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please select an input directory."));
        return false;
    }
    int f = m_function->currentIndex();
    if (f == 1 && m_rcConfigFile->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Encode mode requires a Rate Control config file."));
        return false;
    }
    return true;
}

void MainWindow::onRun()
{
    if (!validateInputs()) return;
    if (m_running) return;

    // Write temp config file
    QString cfgPath = QDir::temp().filePath("dsc_gui_config.cfg");
    generateConfigFile(cfgPath);

    m_runButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_running = true;

    // Run in a worker thread so the UI stays responsive
    QThread *worker = QThread::create([this, cfgPath]() {
        m_engine->runEncodeDecode(cfgPath);
    });
    connect(worker, &QThread::finished, worker, &QThread::deleteLater);
    connect(m_engine, &DSCEngine::finished, worker, &QThread::quit);
    worker->start();
}

void MainWindow::onStop()
{
    // codec_main() runs in the current process so we can't kill it
    // gracefully. Show a message and disable the button.
    m_outputLog->append(tr("\n*** Stop requested — codec is running in-process and cannot be cancelled safely ***\n"));
    m_stopButton->setEnabled(false);
}

void MainWindow::onEngineStarted()
{
    m_outputLog->clear();
    m_outputLog->append(tr("=== DSC Codec Started ===\n"));
}

void MainWindow::onEngineFinished(int exitCode)
{
    m_running = false;
    m_runButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_outputLog->append(tr("\n=== Finished (exit code %1) ===").arg(exitCode));
}

void MainWindow::onEngineOutput(const QString &text)
{
    m_outputLog->append(text);
}
