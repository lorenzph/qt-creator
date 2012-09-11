/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "debuggerkitconfigwidget.h"
#include "debuggerkitinformation.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <QUrl>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QDialogButtonBox>

namespace Debugger {
namespace Internal {


static const char dgbToolsDownloadLink32C[] = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char dgbToolsDownloadLink64C[] = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget:
// -----------------------------------------------------------------------

DebuggerKitConfigWidget::DebuggerKitConfigWidget(ProjectExplorer::Kit *k,
                                                 const DebuggerKitInformation *ki,
                                                 QWidget *parent) :
    ProjectExplorer::KitConfigWidget(parent),
    m_kit(k),
    m_info(ki),
    m_dirty(false),
    m_label(new QLabel(this)),
    m_button(new QPushButton(tr("Manage..."), this))
{
    setToolTip(tr("The debugger to use for this kit."));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_label);

    // ToolButton with Menu, defaulting to 'Autodetect'.
    QMenu *buttonMenu = new QMenu(m_button);
    QAction *autoDetectAction = buttonMenu->addAction(tr("Auto-detect"));
    connect(autoDetectAction, SIGNAL(triggered()), this, SLOT(autoDetectDebugger()));
    QAction *changeAction = buttonMenu->addAction(tr("Edit..."));
    connect(changeAction, SIGNAL(triggered()), this, SLOT(showDialog()));
    m_button->setMenu(buttonMenu);

    discard();
}

QWidget *DebuggerKitConfigWidget::buttonWidget() const
{
       return m_button;
}

QString DebuggerKitConfigWidget::displayName() const
{
    return tr("Debugger:");
}

void DebuggerKitConfigWidget::makeReadOnly()
{
    m_button->setEnabled(false);
}

void DebuggerKitConfigWidget::apply()
{
    DebuggerKitInformation::setDebuggerItem(m_kit, m_item);
    m_dirty = false;
}

void DebuggerKitConfigWidget::discard()
{
    doSetItem(DebuggerKitInformation::debuggerItem(m_kit));
    m_dirty = false;
}

void DebuggerKitConfigWidget::autoDetectDebugger()
{
    setItem(DebuggerKitInformation::autoDetectItem(m_kit));
}

void DebuggerKitConfigWidget::doSetItem(const DebuggerKitInformation::DebuggerItem &item)
{
    m_item = item;
    m_label->setText(DebuggerKitInformation::userOutput(m_item));
}

void DebuggerKitConfigWidget::setItem(const DebuggerKitInformation::DebuggerItem &item)
{
    if (m_item != item) {
        m_dirty = true;
        doSetItem(item);
        emit dirty();
    }
}

void DebuggerKitConfigWidget::showDialog()
{
    DebuggerKitConfigDialog dialog;
    dialog.setWindowTitle(tr("Debugger for \"%1\"").arg(m_kit->displayName()));
    dialog.setDebuggerItem(m_item);
    if (dialog.exec() == QDialog::Accepted)
        setItem(dialog.item());
}

// -----------------------------------------------------------------------
// DebuggerKitConfigDialog:
// -----------------------------------------------------------------------

DebuggerKitConfigDialog::DebuggerKitConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_comboBox(new QComboBox(this))
    , m_label(new QLabel(this))
    , m_chooser(new Utils::PathChooser(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *layout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(GdbEngineType), QVariant(int(GdbEngineType)));
    if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS) {
        m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(CdbEngineType), QVariant(int(CdbEngineType)));
    } else {
        m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(LldbEngineType), QVariant(int(LldbEngineType)));
    }
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshLabel()));
    QLabel *engineTypeLabel = new QLabel(tr("&Engine:"));
    engineTypeLabel->setBuddy(m_comboBox);
    formLayout->addRow(engineTypeLabel, m_comboBox);

    m_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_label->setOpenExternalLinks(true);
    formLayout->addRow(m_label);

    QLabel *binaryLabel = new QLabel(tr("&Binary:"));
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    binaryLabel->setBuddy(m_chooser);
    formLayout->addRow(binaryLabel, m_chooser);
    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);
}

DebuggerEngineType DebuggerKitConfigDialog::engineType() const
{
    const int index = m_comboBox->currentIndex();
    return static_cast<DebuggerEngineType>(m_comboBox->itemData(index).toInt());
}

void DebuggerKitConfigDialog::setEngineType(DebuggerEngineType et)
{
    const int size = m_comboBox->count();
    for (int i = 0; i < size; ++i) {
        if (m_comboBox->itemData(i).toInt() == et) {
            m_comboBox->setCurrentIndex(i);
            refreshLabel();
            break;
        }
    }
}

Utils::FileName DebuggerKitConfigDialog::fileName() const
{
    return m_chooser->fileName();
}

void DebuggerKitConfigDialog::setFileName(const Utils::FileName &fn)
{
    m_chooser->setFileName(fn);
}

void DebuggerKitConfigDialog::refreshLabel()
{
    QString text;
    const DebuggerEngineType type = engineType();
    switch (type) {
    case CdbEngineType: {
#ifdef Q_OS_WIN
        const bool is64bit = Utils::winIs64BitSystem();
#else
        const bool is64bit = false;
#endif
        const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
        const QString versionString = is64bit ? tr("64-bit version") : tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = tr("<html><body><p>Specify the path to the "
                  "<a href=\"%1\">Windows Console Debugger executable</a>"
                  " (%2) here.</p>""</body></html>").arg(link, versionString);
    }
        break;
    default:
        break;
    }
    m_label->setText(text);
    m_label->setVisible(!text.isEmpty());
    m_chooser->setCommandVersionArguments(type == CdbEngineType ?
                                          QStringList(QLatin1String("-version")) :
                                          QStringList(QLatin1String("--version")));
}

void DebuggerKitConfigDialog::setDebuggerItem(const DebuggerKitInformation::DebuggerItem &item)
{
    setEngineType(item.engineType);
    setFileName(item.binary);
}

} // namespace Internal
} // namespace Debugger
