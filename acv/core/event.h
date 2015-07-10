#ifndef ACV_EVENT_H
#define ACV_EVENT_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>


namespace acv {

class event {
public:
	event();
	event(QDateTime const& ts);
	virtual ~event();

	virtual QString to_json();

protected:
	QString type_;
	QDateTime ts_;
	QJsonObject j_;
};

}

#endif //ACV_EVENT_H
