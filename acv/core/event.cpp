#include <QtCore/qjsondocument.h>
#include "event.h"


namespace acv {

event::event() : event(QDateTime::currentDateTime()) {
}

event::event(QDateTime const& ts) :
	ts_(ts) {
}

event::~event() {
}

QString event::to_json() {
	if (type_.isEmpty())
		type_ = "generic";

	j_["evt"] = type_;

	QJsonDocument jdoc(j_);
	return jdoc.toJson(QJsonDocument::Compact);
}

}