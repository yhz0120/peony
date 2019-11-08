#include "properties-window.h"

#include "properties-window-tab-page-plugin-iface.h"

#include "basic-properties-page-factory.h"

#include <QToolBar>
#include <QDialogButtonBox>

using namespace Peony;

//plugin manager

static PropertiesWindowPluginManager *global_instance = nullptr;

PropertiesWindowPluginManager *PropertiesWindowPluginManager::getInstance()
{
    if (!global_instance)
        global_instance = new PropertiesWindowPluginManager;
    return global_instance;
}

void PropertiesWindowPluginManager::release()
{
    deleteLater();
}

PropertiesWindowPluginManager::PropertiesWindowPluginManager(QObject *parent) : QObject (parent)
{
    //register internal factories.
    registerFactory(BasicPropertiesPageFactory::getInstance());
}

PropertiesWindowPluginManager::~PropertiesWindowPluginManager()
{
    for (auto factory : m_factory_hash) {
        factory->closeFactory();
    }
    m_factory_hash.clear();
}


bool PropertiesWindowPluginManager::registerFactory(PropertiesWindowTabPagePluginIface *factory)
{
    auto id = factory->name();
    if (m_factory_hash.value(id)) {
        return false;
    }
    m_factory_hash.insert(id, factory);
    m_sorted_factory_map.insert(-factory->tabOrder(), id);
    return true;
}

const QStringList PropertiesWindowPluginManager::getFactoryNames()
{
    QStringList l;
    for (auto factoryId : m_sorted_factory_map) {
        l<<factoryId;
    }
    return l;
}

PropertiesWindowTabPagePluginIface *PropertiesWindowPluginManager::getFactory(const QString &id)
{
    return m_factory_hash.value(id);
}

PropertiesWindow::PropertiesWindow(const QStringList &uris, QWidget *parent) : QMainWindow (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setContentsMargins(0, 15, 0, 0);
    setMinimumSize(360, 480);
    auto w = new PropertiesWindowPrivate(uris, this);
    setCentralWidget(w);

    QToolBar *buttonToolBar = new QToolBar(this);
    addToolBar(Qt::BottomToolBarArea, buttonToolBar);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok,
                                                 buttonToolBar);
    box->setContentsMargins(0, 5, 5, 5);
    buttonToolBar->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, [=](){
        this->close();
    });
}

//properties window
PropertiesWindowPrivate::PropertiesWindowPrivate(const QStringList &uris, QWidget *parent) : QTabWidget(parent)
{
    setTabsClosable(false);
    setMovable(false);

    auto manager = PropertiesWindowPluginManager::getInstance();
    auto names = manager->getFactoryNames();
    for (auto name : names) {
        auto factory = manager->getFactory(name);
        if (factory->supportUris(uris)) {
            auto tabPage = factory->createTabPage(uris);
            tabPage->setParent(this);
            addTab(tabPage, factory->name());
        }
    }
}
