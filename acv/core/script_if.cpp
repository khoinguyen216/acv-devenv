#include "script_if.h"

#include <cassert>
#include <cstring>

#include "script_cmd.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork>

namespace acv {

script_if::script_if(QObject *parent) : QObject(parent) {
	L_ = luaL_newstate();
	luaL_openlibs(L_);
	chunkbuf_ = new char[MAX_CHUNK_LENGTH];

	lua_register_cmds(L_);
}

script_if::~script_if() {
	lua_close(L_);
	delete[] chunkbuf_;
}

void script_if::start_server(unsigned short port) {
	tcpserver_ = new QTcpServer(this);
	tcpserver_->setMaxPendingConnections(1);
	if (!tcpserver_->listen(QHostAddress::Any, port)) {
		tcpserver_->close();
		delete tcpserver_;
		tcpserver_ = NULL;
		return;
	}

	connect(tcpserver_, &QTcpServer::newConnection,
			this, &script_if::on_newConnection);
}

void script_if::on_newConnection() {
	if (client_ != NULL) {
		tcpserver_->nextPendingConnection()->disconnectFromHost();
	}

	client_ = tcpserver_->nextPendingConnection();
	chunksz_ = 0;
	connect(client_, &QTcpSocket::disconnected,
			this, &script_if::on_disconnected);
	connect(client_, &QTcpSocket::readyRead,
			this, &script_if::on_readyRead);
}

void script_if::on_readyRead() {
	if (!client_->canReadLine())
		return;

	long bytes = client_->readLine(chunkbuf_, MAX_LINE_LENGTH);
	if (bytes <= 0) {
		// Error, disconnect client
		client_->disconnectFromHost();
		return;
	} else if (bytes == MAX_LINE_LENGTH - 1) {
		client_->write("Input line is too long");
		client_->disconnectFromHost();
		return;
	}

	chunksz_ = bytes;
	on_inputLine();
}

void script_if::on_disconnected() {
	delete client_;
	client_ = NULL;
}

void script_if::on_inputLine() {
	assert(chunksz_ > 0);
	assert(chunkbuf_[chunksz_ - 1] == '\n');

	int status;

	if (load_chunk()) {
		status = docall(0, LUA_MULTRET);
		if (status != LUA_OK) {
			report(status);
		}
		print_result();
		lua_settop(L_, 0);
	} else if (!incomplete_) {
		report(status);
		lua_settop(L_, 0);
	}

	QMetaObject::invokeMethod(this, "on_readyRead", Qt::QueuedConnection);
}

int script_if::msghandler(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg == NULL) {  // is error object not a string?
		if (luaL_callmeta(L, 1, "__tostring")
			&& lua_type(L, -1) == LUA_TSTRING)
			return 1;  // that is the message
		else
			msg = lua_pushfstring(L, "(error object is a %s value)",
								  luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);  // append a standard traceback
	return 1;  // return the traceback
}

bool script_if::prompt(int firstline) {
	if (firstline)
		return client_->write(USER_PROMPT) > 0;
	else
		return client_->write(USER_PROMPT2) > 0;
}

int script_if::incomplete(int status) {
#define EOFMARK        "<eof>"
#define marklen        (sizeof(EOFMARK)/sizeof(char) - 1)
	if (status == LUA_ERRSYNTAX) {
		size_t lmsg;
		const char *msg = lua_tolstring(L_, -1, &lmsg);
		if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
			lua_pop(L_, 1);
			return 1;
		}
	}
	return 0;
}

int script_if::addreturn() {
	int status;
	size_t len;
	const char *line;
	lua_pushliteral(L_, "return ");
	lua_pushvalue(L_, -2);  // duplicate line
	lua_concat(L_, 2);  // new line is "return ..."
	line = lua_tolstring(L_, -1, &len);
	if ((status = luaL_loadbuffer(L_, line, len, "=acv")) == LUA_OK)
		lua_remove(L_, -3);  // remove original line
	else
		lua_pop(L_, 2);  // remove result from 'luaL_loadbuffer' and new line

	return status;
}

int script_if::docall(int narg, int nres) {
	int status;
	int base = lua_gettop(L_) - narg;  // function index
	lua_pushcfunction(L_, msghandler);  // push message handler
	lua_insert(L_, base);  // put it under function and args
	status = lua_pcall(L_, narg, nres, base);
	lua_remove(L_, base);  // remove message handler from the stack
	return status;
}

bool script_if::load_chunk() {
	assert(chunksz_ > 0);
	assert(chunkbuf_[chunksz_ - 1] == '\n');

	int status;
	bool ok;

	push_line(!incomplete_);

	if (!incomplete_) {
		// New command
		status = addreturn();
		if (status == LUA_OK) {
			ok = true;
		} else {
			size_t len;
			const char *line = lua_tolstring(L_, 1, &len);  // get what it has
			int status = luaL_loadbuffer(L_, line, len, "=acv");  // try it
			if (status == LUA_OK)
				ok = true;
			else if (!incomplete(status)) {
				report(status);
				ok = false;
			}
			else {
				incomplete_ = true;
				ok = false;
			}
		}
	} else {
		// Continuing command
		lua_pushliteral(L_, "\n");
		lua_insert(L_, -1);
		lua_concat(L_, 3);
		status = addreturn();
		if (status == LUA_OK) {
			incomplete_ = false;
			ok = true;
		} else {
			size_t len;
			const char *line = lua_tolstring(L_, 1, &len);  // get what it has
			int status = luaL_loadbuffer(L_, line, len, "=acv");  // try it
			if (status == LUA_OK) {
				incomplete_ = false;
				ok = true;
			} else if (!incomplete(status)) {
				incomplete_ = false;
				report(status);
				ok = false;
			}
			else {
				ok = false;
			}
		}
	}

	if (ok)
		lua_remove(L_, 1); // Pop the command string off the stack

	return ok;
}

void script_if::push_line(bool firstline) {
	assert(chunksz_ > 0);
	assert(chunkbuf_[chunksz_ - 1] == '\n');

	// Remove the newline character at the end of the line
	chunkbuf_[--chunksz_] = '\0';
	if (firstline && chunkbuf_[0] == '=')
		lua_pushfstring(L_, "return %s", chunkbuf_ + 1);
	else
		lua_pushlstring(L_, chunkbuf_, chunksz_);
}

int script_if::report(int status) {
	if (status != LUA_OK) {
		char const *msg = lua_tostring(L_, -1);
		client_->write(msg);
		client_->write("\n");
		lua_pop(L_, 1);
	}

	return status;
}

void script_if::stackdump() {
	int n = lua_gettop(L_);
	if (n > 0) {  /* any result to be printed? */
		for (int i = 1; i <= n; ++i) {
			int t = lua_type(L_, i);
			QString s;
			switch (t) {
				case LUA_TSTRING:
					s = (lua_tostring(L_, i));
					break;
				case LUA_TBOOLEAN:
					s = (lua_toboolean(L_, i) ? "true" : "false");
					break;
				case LUA_TNUMBER:
					s = (lua_tonumber(L_, i));
					break;
				default:
					s = QString("lua_") + (luaL_typename(L_, i));
					break;
			}
			qDebug() << s.replace("\r", ">").replace("\n", ">");
		}
	}
}

void script_if::print_result() {
	int n = lua_gettop(L_);
	if (n > 0) {
		QJsonArray results;
		for (int i = 1; i <= n; ++i) {
			int t = lua_type(L_, i);
			QString s;
			switch (t) {
				case LUA_TSTRING:
					s = QString("%1").arg(lua_tostring(L_, i));
					break;
				case LUA_TBOOLEAN:
					s = QString("%1").arg(
							lua_toboolean(L_, i) ? "true" : "false");
					break;
				case LUA_TNUMBER:
					s = QString::number(lua_tonumber(L_, i));
					break;
				default:
					s = QString("lua_") + (luaL_typename(L_, i));
					break;
			}
			results.push_back(s);
		}
		QJsonObject jo;
		jo["result"] = results;
		QJsonDocument doc(jo);
		client_->write(doc.toJson(QJsonDocument::Compact));
		client_->write("\n");
	}
}

}
