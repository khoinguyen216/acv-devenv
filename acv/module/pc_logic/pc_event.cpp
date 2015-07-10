#include "pc_event.h"

#include <QVariantHash>


namespace acv {


pc_event::pc_event() : pc_event(QDateTime::currentDateTime()) {

}

pc_event::pc_event(QDateTime const &ts) : event(ts) {
	type_ = "peoplecount";
}

pc_event::~pc_event() {
}

void pc_event::add_person(int id, double x, double y, double w, double h) {
	QJsonObject person {
			{"id", id},
			{"x", x}, {"y", y}, {"w", w}, {"h", h}
	};
	people_.push_back(person);
	j_["objects"] = people_;
}

}
