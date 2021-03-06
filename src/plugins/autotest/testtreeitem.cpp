/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "autotestconstants.h"
#include "autotest_utils.h"
#include "testcodeparser.h"
#include "testconfiguration.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cplusplus/Icons.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type)
    : TreeItem( { name } ),
      m_name(name),
      m_filePath(filePath),
      m_type(type),
      m_line(0),
      m_status(NewlyAdded)
{
    m_checked = (m_type == TestCase || m_type == TestFunctionOrSet) ? Qt::Checked : Qt::Unchecked;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::SlotPrivateIconType),
        QIcon(QLatin1String(":/images/data.png"))
    };

    if (int(type) >= int(sizeof icons / sizeof *icons))
        return icons[2];
    return icons[type];
}

QVariant TestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == Root && childCount() == 0)
            return QString(m_name + QCoreApplication::translate("TestTreeItem", " (none)"));
        else
            return m_name;
    case Qt::ToolTipRole:
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        return QVariant();
    case LinkRole: {
        QVariant itemLink;
        itemLink.setValue(TextEditor::TextEditorWidget::Link(m_filePath, m_line, m_column));
        return itemLink;
    }
    case ItalicRole:
        return false;
    case TypeRole:
        return m_type;
    }
    return QVariant();
}

bool TestTreeItem::setData(int /*column*/, const QVariant &data, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState old = checked();
        setChecked((Qt::CheckState)data.toInt());
        return checked() != old;
    }
    return false;
}

bool TestTreeItem::modifyTestCaseContent(const QString &name, unsigned line, unsigned column)
{
    bool hasBeenModified = modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(location.m_name);
    hasBeenModified |= modifyLineAndColumn(location);
    return hasBeenModified;
}

bool TestTreeItem::modifyDataTagContent(const QString &fileName,
                                        const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyName(location.m_name);
    hasBeenModified |= modifyLineAndColumn(location);
    return hasBeenModified;
}

bool TestTreeItem::modifyLineAndColumn(const TestCodeLocationAndType &location)
{
    return modifyLineAndColumn(location.m_line, location.m_column);
}

bool TestTreeItem::modifyLineAndColumn(unsigned line, unsigned column)
{
    bool hasBeenModified = false;
    if (m_line != line) {
        m_line = line;
        hasBeenModified = true;
    }
    if (m_column != column) {
        m_column = column;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::setChecked(const Qt::CheckState checkState)
{
    switch (m_type) {
    case TestFunctionOrSet: {
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        parentItem()->revalidateCheckState();
        break;
    }
    case TestCase: {
        Qt::CheckState usedState = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        for (int row = 0, count = childCount(); row < count; ++row)
            childItem(row)->setChecked(usedState);
        m_checked = usedState;
    }
    default:
        return;
    }
}

Qt::CheckState TestTreeItem::checked() const
{
    switch (m_type) {
    case TestCase:
    case TestFunctionOrSet:
        return m_checked;
    default:
        return Qt::Unchecked;
    }
}

void TestTreeItem::markForRemoval(bool mark)
{
    m_status = mark ? MarkedForRemoval : Cleared;
}

void TestTreeItem::markForRemovalRecursively(bool mark)
{
    markForRemoval(mark);
    for (int row = 0, count = childCount(); row < count; ++row)
        childItem(row)->markForRemovalRecursively(mark);
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::childItem(int row) const
{
    return static_cast<TestTreeItem *>(child(row));
}

TestTreeItem *TestTreeItem::findChildByName(const QString &name)
{
    return findChildBy([name](const TestTreeItem *other) -> bool {
        return other->name() == name;
    });
}

TestTreeItem *TestTreeItem::findChildByFile(const QString &filePath)
{
    return findChildBy([filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByNameAndFile(const QString &name, const QString &filePath)
{
    return findChildBy([name, filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath && other->name() == name;
    });
}

QList<TestConfiguration *> TestTreeItem::getAllTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

QList<TestConfiguration *> TestTreeItem::getSelectedTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

void TestTreeItem::revalidateCheckState()
{
    if (childCount() == 0)
        return;
    bool foundChecked = false;
    bool foundUnchecked = false;
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        switch (child->type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            continue;
        default:
            break;
        }

        foundChecked |= (child->checked() != Qt::Unchecked);
        foundUnchecked |= (child->checked() == Qt::Unchecked);
        if (foundChecked && foundUnchecked) {
            m_checked = Qt::PartiallyChecked;
            return;
        }
    }
    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
}

inline bool TestTreeItem::modifyFilePath(const QString &filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        return true;
    }
    return false;
}

inline bool TestTreeItem::modifyName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        return true;
    }
    return false;
}

TestTreeItem *TestTreeItem::findChildBy(CompareFunction compare)
{
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (compare(child))
            return child;
    }
    return 0;
}

AutoTestTreeItem *AutoTestTreeItem::createTestItem(const TestParseResult &result)
{
    const QtTestParseResult &parseResult = static_cast<const QtTestParseResult &>(result);
    AutoTestTreeItem *item = new AutoTestTreeItem(result.testCaseName, result.fileName, TestCase);
    item->setProFile(parseResult.proFile);
    item->setLine(parseResult.line);
    item->setColumn(parseResult.column);

    foreach (const QString &functionName, parseResult.functions.keys()) {
        const TestCodeLocationAndType &locationAndType = parseResult.functions.value(functionName);
        const QString qualifiedName = result.testCaseName + QLatin1String("::") + functionName;
        item->appendChild(createFunctionItem(functionName, locationAndType,
                                             parseResult.dataTags.value(qualifiedName)));
    }
    return item;
}

AutoTestTreeItem *AutoTestTreeItem::createFunctionItem(const QString &functionName,
                                                       const TestCodeLocationAndType &location,
                                                       const TestCodeLocationList &dataTags)
{
    AutoTestTreeItem *item = new AutoTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);

    // if there are any data tags for this function add them
    foreach (const TestCodeLocationAndType &tagLocation, dataTags)
        item->appendChild(createDataTagItem(location.m_name, tagLocation));
    return item;
}

AutoTestTreeItem *AutoTestTreeItem::createDataTagItem(const QString &fileName,
                                                      const TestCodeLocationAndType &location)
{
    AutoTestTreeItem *tagItem = new AutoTestTreeItem(location.m_name, fileName, location.m_type);
    tagItem->setLine(location.m_line);
    tagItem->setColumn(location.m_column);
    return tagItem;
}

QVariant AutoTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        default:
            return checked();
        }
    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        default:
            return false;
        }
    }
    return TestTreeItem::data(column, role);
}

bool AutoTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
    case TestFunctionOrSet:
    case TestDataTag:
        return true;
    default:
        return false;
    }
}

TestConfiguration *AutoTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    TestConfiguration *config = 0;
    switch (type()) {
    case TestCase:
        config = new TestConfiguration(name(), QStringList(), childCount());
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(), proFile()));
        break;
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        config = new TestConfiguration(parent->name(), QStringList() << name());
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(
                TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    case TestDataTag:{
        const TestTreeItem *function = parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : 0;
        if (!parent)
            return 0;
        const QString functionWithTag = function->name() + QLatin1Char(':') + name();
        config = new TestConfiguration(parent->name(), QStringList() << functionWithTag);
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(),
                                                                         parent->proFile()));
        break;
    }
    default:
        return 0;
    }
    return config;
}

QList<TestConfiguration *> AutoTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        TestConfiguration *tc = new TestConfiguration(child->name(), QStringList(),
                                                      child->childCount());
        tc->setProFile(child->proFile());
        tc->setProject(project);
        tc->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                     child->proFile()));
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> AutoTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    TestConfiguration *testConfiguration = 0;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new TestConfiguration(child->name(), QStringList(), child->childCount());
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            const QString childName = child->name();
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            testConfiguration = new TestConfiguration(childName, testCases);
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
        }
    }

    return result;
}

QuickTestTreeItem *QuickTestTreeItem::createTestItem(const TestParseResult &result)
{
    const QuickTestParseResult &parseResult = static_cast<const QuickTestParseResult &>(result);
    QuickTestTreeItem *item = new QuickTestTreeItem(parseResult.testCaseName, parseResult.fileName,
                                                    TestCase);
    item->setProFile(result.proFile);
    item->setLine(result.line);
    item->setColumn(result.column);
    foreach (const QString &functionName, parseResult.functions.keys())
        item->appendChild(createFunctionItem(functionName, parseResult.functions.value(functionName)));
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createFunctionItem(const QString &functionName,
                                                         const TestCodeLocationAndType &location)
{
    QuickTestTreeItem *item = new QuickTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createUnnamedQuickTestItem(const TestParseResult &result)
{
    const QuickTestParseResult &parseResult = static_cast<const QuickTestParseResult &>(result);
    QuickTestTreeItem *item = new QuickTestTreeItem(QString(), QString(), TestCase);
    foreach (const QString &functionName, parseResult.functions.keys())
        item->appendChild(createUnnamedQuickFunctionItem(functionName, parseResult));
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createUnnamedQuickFunctionItem(const QString &functionName,
                                                                     const TestParseResult &result)
{
    const QuickTestParseResult &parseResult = static_cast<const QuickTestParseResult &>(result);
    const TestCodeLocationAndType &location = parseResult.functions.value(functionName);
    QuickTestTreeItem *item = new QuickTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    item->setProFile(parseResult.proFile);
    return item;
}

QVariant QuickTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == TestCase && name().isEmpty())
            return QObject::tr(Constants::UNNAMED_QUICKTESTS);
        break;
    case Qt::ToolTipRole:
        if (type() == TestCase && name().isEmpty())
            return QObject::tr("<p>Give all test cases a name to ensure correct behavior "
                               "when running test cases and to be able to select them.</p>");
        break;
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        case TestCase:
            return name().isEmpty() ? QVariant() : checked();
        case TestFunctionOrSet:
            return (parentItem() && !parentItem()->name().isEmpty()) ? checked() : QVariant();
        default:
            return checked();
        }

    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        case TestCase:
            return name().isEmpty();
        case TestFunctionOrSet:
            return parentItem() ? parentItem()->name().isEmpty() : false;
        default:
            return false;
        }
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

bool QuickTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
        return !name().isEmpty();
    case TestFunctionOrSet:
        return !parentItem()->name().isEmpty();
    default:
        return false;
    }
}

TestConfiguration *QuickTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    TestConfiguration *config = 0;
    switch (type()) {
    case TestCase: {
        QStringList testFunctions;
        for (int row = 0, count = childCount(); row < count; ++row)
            testFunctions << name() + QLatin1String("::") + childItem(row)->name();
        config = new TestConfiguration(QString(), testFunctions);
        config->setProFile(proFile());
        config->setProject(project);
        break;
    }
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        QStringList testFunction(parent->name() + QLatin1String("::") + name());
        config = new TestConfiguration(QString(), testFunction);
        config->setProFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return 0;
    }
    return config;
}

QList<TestConfiguration *> QuickTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, int> foundProFiles;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString &proFile = grandChild->proFile();
                foundProFiles.insert(proFile, foundProFiles[proFile] + 1);
            }
            continue;
        }
        // named Quick Test
        const QString &proFile = child->proFile();
        foundProFiles.insert(proFile, foundProFiles[proFile] + child->childCount());
    }
    // create TestConfiguration for each project file
    QHash<QString, int>::ConstIterator it = foundProFiles.begin();
    QHash<QString, int>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
        TestConfiguration *tc = new TestConfiguration(QString(), QStringList(), it.value());
        tc->setProFile(it.key());
        tc->setProject(project);
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> QuickTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    TestConfiguration *tc = 0;
    QHash<QString, TestConfiguration *> foundProFiles;
    // unnamed Quick Tests must be handled first
    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int childRow = 0, ccount = unnamed->childCount(); childRow < ccount; ++ childRow) {
            const TestTreeItem *grandChild = unnamed->childItem(childRow);
            const QString &proFile = grandChild->proFile();
            if (foundProFiles.contains(proFile)) {
                QTC_ASSERT(tc,
                           qWarning() << "Illegal state (unnamed Quick Test listed as named)";
                           return QList<TestConfiguration *>());
                foundProFiles[proFile]->setTestCaseCount(tc->testCaseCount() + 1);
            } else {
                tc = new TestConfiguration(QString(), QStringList());
                tc->setTestCaseCount(1);
                tc->setUnnamedOnly(true);
                tc->setProFile(proFile);
                tc->setProject(project);
                foundProFiles.insert(proFile, tc);
            }
        }
    }

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests have been handled separately already
        if (child->name().isEmpty())
            continue;

        // named Quick Tests
        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
        case Qt::PartiallyChecked:
        default:
            QStringList testFunctions;
            int grandChildCount = child->childCount();
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->type() != TestFunctionOrSet)
                    continue;
                testFunctions << child->name() + QLatin1String("::") + grandChild->name();
            }
            if (foundProFiles.contains(child->proFile())) {
                tc = foundProFiles[child->proFile()];
                QStringList oldFunctions(tc->testCases());
                // if oldFunctions.size() is 0 this test configuration is used for at least one
                // unnamed test case
                if (oldFunctions.size() == 0) {
                    tc->setTestCaseCount(tc->testCaseCount() + testFunctions.size());
                    tc->setUnnamedOnly(false);
                } else {
                    oldFunctions << testFunctions;
                    tc->setTestCases(oldFunctions);
                }
            } else {
                tc = new TestConfiguration(QString(), testFunctions);
                tc->setProFile(child->proFile());
                tc->setProject(project);
                foundProFiles.insert(child->proFile(), tc);
            }
            break;
        }
    }
    QHash<QString, TestConfiguration *>::ConstIterator it = foundProFiles.begin();
    QHash<QString, TestConfiguration *>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
        TestConfiguration *config = it.value();
        if (!config->unnamedOnly())
            result << config;
        else
            delete config;
    }

    return result;
}

TestTreeItem *QuickTestTreeItem::unnamedQuickTests() const
{
    if (type() != Root)
        return 0;

    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (child->name().isEmpty())
            return child;
    }
    return 0;
}

static QString gtestFilter(GoogleTestTreeItem::TestStates states)
{
    if ((states & GoogleTestTreeItem::Parameterized) && (states & GoogleTestTreeItem::Typed))
        return QLatin1String("*/%1/*.%2");
    if (states & GoogleTestTreeItem::Parameterized)
        return QLatin1String("*/%1.%2/*");
    if (states & GoogleTestTreeItem::Typed)
        return QLatin1String("%1/*.%2");
    return QLatin1String("%1.%2");
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestItem(const TestParseResult &result)
{
    const GoogleTestParseResult &parseResult = static_cast<const GoogleTestParseResult &>(result);
    GoogleTestTreeItem *item = new GoogleTestTreeItem(parseResult.testCaseName, QString(), TestCase);
    item->setProFile(parseResult.proFile);
    if (parseResult.parameterized)
        item->setState(Parameterized);
    if (parseResult.typed)
        item->setState(Typed);
    if (parseResult.disabled)
        item->setState(Disabled);
    foreach (const TestCodeLocationAndType &location, parseResult.testSets)
        item->appendChild(createTestSetItem(result, location));
    return item;
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestSetItem(const TestParseResult &result,
                                                          const TestCodeLocationAndType &location)
{
    GoogleTestTreeItem *item = new GoogleTestTreeItem(location.m_name, result.fileName,
                                                      location.m_type);
    item->setStates(location.m_state);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    item->setProFile(result.proFile);
    return item;
}

QVariant GoogleTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            break;

        const QString &displayName = (m_state & GoogleTestTreeItem::Disabled)
                ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case Qt::CheckStateRole:
        switch (type()) {
        case TestCase:
        case TestFunctionOrSet:
            return checked();
        default:
            return QVariant();
        }
    case ItalicRole:
        return false;
    case StateRole:
        return (int)m_state;
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

TestConfiguration *GoogleTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    TestConfiguration *config = 0;
    switch (type()) {
    case TestCase: {
        const QString &testSpecifier = gtestFilter(state()).arg(name()).arg(QLatin1Char('*'));
        if (int count = childCount()) {
            config = new TestConfiguration(QString(), QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProFile(proFile());
            config->setProject(project);
            // item has no filePath set - so take it of the first children
            config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(childItem(0)->filePath(), proFile()));
            config->setTestType(TestTypeGTest);
        }
        break;
    }
    case TestFunctionOrSet: {
        GoogleTestTreeItem *parent = static_cast<GoogleTestTreeItem *>(parentItem());
        if (parent)
            return 0;
        const QString &testSpecifier = gtestFilter(parent->state()).arg(parent->name()).arg(name());
        config = new TestConfiguration(QString(), QStringList(testSpecifier));
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        config->setTestType(TestTypeGTest);
        break;
    }
    default:
        return 0;
    }
    return config;
}

// used as key inside getAllTestCases()/getSelectedTestCases() for Google Tests
class ProFileWithDisplayName
{
public:
    ProFileWithDisplayName(const QString &file, const QString &name)
        : proFile(file), displayName(name) {}
    QString proFile;
    QString displayName;

    bool operator==(const ProFileWithDisplayName &rhs) const
    {
        return proFile == rhs.proFile && displayName == rhs.displayName;
    }
};

// needed as ProFileWithDisplayName is used as key inside a QHash
bool operator<(const ProFileWithDisplayName &lhs, const ProFileWithDisplayName &rhs)
{
    return lhs.proFile == rhs.proFile ? lhs.displayName < rhs.displayName
                                      : lhs.proFile < rhs.proFile;
}

// needed as ProFileWithDisplayName is used as a key inside a QHash
uint qHash(const ProFileWithDisplayName &lhs)
{
    return ::qHash(lhs.proFile) ^ ::qHash(lhs.displayName);
}

QList<TestConfiguration *> GoogleTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, int> proFilesWithTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));

        const int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithTestSets.insert(key, proFilesWithTestSets[key] + 1);
            }
        }
    }

    QHash<ProFileWithDisplayName, int>::ConstIterator it = proFilesWithTestSets.begin();
    QHash<ProFileWithDisplayName, int>::ConstIterator end = proFilesWithTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        TestConfiguration *tc = new TestConfiguration(QString(), QStringList(), it.value());
        tc->setTestType(TestTypeGTest);
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

QList<TestConfiguration *> GoogleTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, QStringList> proFilesWithCheckedTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));
        if (child->checked() == Qt::Unchecked)
            continue;

        int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithCheckedTestSets[key].append(
                            gtestFilter(child->state()).arg(child->name()).arg(grandChild->name()));
            }
        }
    }

    QHash<ProFileWithDisplayName, QStringList>::ConstIterator it = proFilesWithCheckedTestSets.begin();
    QHash<ProFileWithDisplayName, QStringList>::ConstIterator end = proFilesWithCheckedTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        TestConfiguration *tc = new TestConfiguration(QString(), it.value());
        tc->setTestType(TestTypeGTest);
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

bool GoogleTestTreeItem::modifyTestSetContent(const QString &fileName,
                                              const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyLineAndColumn(location);
    if (m_state != location.m_state) {
        m_state = location.m_state;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *GoogleTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                              GoogleTestTreeItem::TestStates state,
                                                              const QString &proFile)
{
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        const GoogleTestTreeItem *gtestItem = static_cast<const GoogleTestTreeItem *>(other);
        return other->proFile() == proFile
                && other->name() == name
                && gtestItem->state() == state;
    });
}

QString GoogleTestTreeItem::nameSuffix() const
{
    static QString markups[] = { QCoreApplication::translate("GoogleTestTreeItem", "parameterized"),
                                 QCoreApplication::translate("GoogleTestTreeItem", "typed") };
    QString suffix;
    if (m_state & Parameterized)
        suffix =  QLatin1String(" [") + markups[0];
    if (m_state & Typed)
        suffix += (suffix.isEmpty() ? QLatin1String(" [") : QLatin1String(", ")) + markups[1];
    if (!suffix.isEmpty())
        suffix += QLatin1Char(']');
    return suffix;
}

} // namespace Internal
} // namespace Autotest
