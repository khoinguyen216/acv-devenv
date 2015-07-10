#ifndef ACV_APP_H
#define ACV_APP_H

#ifdef WITH_GUI
#include <QApplication>
#else
#include <QCoreApplication>
#endif
#include <QHash>
#include <QString>

#include "module.h"
#include "module_factory.h"
#include "module_graph.h"


namespace acv {

#ifdef WITH_GUI
class app : public QApplication {
#else
class app : public QCoreApplication {
#endif

public:
#ifdef WITH_GUI
	app(int& argc, char** argv) : QApplication(argc, argv) {}
#else
	app(int &argc, char **argv) : QCoreApplication(argc, argv) {}
#endif

	virtual ~app() {}
	virtual void preprocess_cmdargs() = 0;
	virtual void postprocess_cmdargs() = 0;
	virtual void load_config() = 0;
	virtual void setup() = 0;

	virtual bool add_module(QString const& id, QString const& modtype);
	virtual bool add_cable(QString const& e0, QString const& e1);
	virtual void remove_module(QString const& id);
	virtual bool start_module(QString const& id);
	virtual bool setopt(QString const& id, QString const& o, QString const& v);

protected:
	QHash<QString, module*> modules_;
	module_factory*			module_factory_;
	module_graph			module_graph_;
};

}

#endif //ACV_APP_H
