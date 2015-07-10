#ifndef ACV_MODULE_FACTORY_H
#define ACV_MODULE_FACTORY_H

#include <QHash>
#include <QObject>
#include <QString>

#include "module.h"

#include <QStringList>


namespace acv {
class module_factory {
	typedef module* (*module_factory_func)(void);
	typedef QHash<QString, module_factory_func> map_type;

public:
	module_factory();
	virtual ~module_factory() {}

	bool register_module(QString const& m, module_factory_func f);
	module* create_instance(QString const& m);
	QStringList list();

protected:
	map_type& get_map() { return map_; }

private:
	map_type map_;
};
}

#endif //ACV_MODULE_FACTORY_H
