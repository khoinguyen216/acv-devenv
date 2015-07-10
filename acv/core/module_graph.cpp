#include "module_graph.h"

#include <QDebug>
#include <QMetaMethod>


namespace acv {
module_graph::module_graph() {
}

module_graph::~module_graph() {
}

QStringList module_graph::list_modules() const {
	return modules_.keys();
}

bool module_graph::connect_by_name(QObject *emitter, QString const &sig,
								   QObject *receiver, QString const &slot) {
	int index = emitter->metaObject()->indexOfSignal(
			QMetaObject::normalizedSignature(qPrintable(sig)));
	if (index == -1) {
		qWarning() << "No signal" << sig << "in" << emitter->objectName();
		return false;
	}
	auto const& sig_method = emitter->metaObject()->method(index);

	index = receiver->metaObject()->indexOfSlot(
			QMetaObject::normalizedSignature(qPrintable(slot)));
	if (index == -1) {
		qWarning() << "No slot" << slot << "in" << receiver->objectName();
		return false;
	}
	auto const& slot_method = receiver->metaObject()->method(index);
	QObject::connect(emitter, sig_method, receiver, slot_method);

	return true;
}

bool module_graph::add_module(QString const &id, module* m) {
	if (modules_.contains(id))
		return false;

	m->setObjectName(id);
	modules_.insert(id, m);
	return true;
}

bool module_graph::add_cable(cable_info const &c) {
	if (!modules_.contains(c.mod_a()) || !modules_.contains(c.mod_b())
			|| c.socket_a().isEmpty() || c.socket_b().isEmpty())
		return false;

	module* a = modules_[c.mod_a()];
	module* b = modules_[c.mod_b()];
	module_socket const* sock_a = a->expose_socket(c.socket_a());
	module_socket const* sock_b = b->expose_socket(c.socket_b());
	if (sock_a == NULL || sock_b == NULL
			|| sock_a->dir_out == sock_b->dir_out)
		return false;

	for (auto const& k : sock_a->wires.keys()) {
		if (!sock_b->wires.contains(k))
			continue;

		QString const& wire_a = sock_a->wires[k];
		QString const& wire_b = sock_b->wires[k];
		if (sock_a->dir_out)
			connect_by_name(a, wire_a, b, wire_b);
		else
			connect_by_name(b, wire_b, a, wire_a);
	}

	return true;
}

module *module_graph::get_module(QString const &id) {
	if (!modules_.contains(id))
		return NULL;
	return modules_.find(id).value();
}
}
