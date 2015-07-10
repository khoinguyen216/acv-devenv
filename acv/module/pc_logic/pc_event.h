#ifndef ACV_PC_EVENT_H
#define ACV_PC_EVENT_H

#include "core/event.h"

#include <QJsonArray>


namespace acv {

class pc_event : public event {
public:
	pc_event();
	pc_event(QDateTime const& ts);
	~pc_event();

	void add_person(int id, double x, double y, double w, double h);

private:
	QJsonArray people_;
};

}

#endif //ACV_PC_EVENT_H
