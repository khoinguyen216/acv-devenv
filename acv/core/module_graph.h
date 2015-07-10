#ifndef ACV_MODULE_GRAPH_H
#define ACV_MODULE_GRAPH_H

#include <QHash>
#include <QString>
#include <QStringList>

#include "cable_info.h"
#include "module.h"


namespace acv {
class module_graph {
public:
	module_graph();
	~module_graph();

	QStringList list_modules() const;
	bool add_module(QString const& id, module* m);
	bool add_cable(cable_info const&c);
	module* get_module(QString const& id);

// Disable copy constructor and assignment operator
private:
	module_graph(module_graph const&);
	module_graph& operator=(module_graph const&);

private:
	bool connect_by_name(QObject* emitter, QString const& sig,
						QObject* receiver, QString const& slot);

private:
	QHash<QString, module*> modules_;
};
}

#endif //ACV_MODULE_GRAPH_H
