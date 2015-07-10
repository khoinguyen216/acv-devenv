#include "script_cmd.h"

#ifdef WITH_GUI
#include <QApplication>
#else
#include <QCoreApplication>
#endif
#include <QDebug>

#include "app.h"


void lua_register_cmds(lua_State* L) {
	lua_pushcfunction(L, lua_acv_version);
	lua_setglobal(L, "acv_version");
	lua_pushcfunction(L, lua_acv_modstart);
	lua_setglobal(L, "acv_modstart");
	lua_pushcfunction(L, lua_acv_modsetopt);
	lua_setglobal(L, "acv_modsetopt");
}

int lua_acv_version(lua_State* L) {
	lua_pushstring(L, qApp->applicationVersion().toStdString().c_str());
	return 1;
}

int lua_acv_modstart(lua_State* L) {
	char const* m = luaL_checkstring(L, 1);
	int ok = static_cast<acv::app*>(qApp->instance())->start_module(m);
	lua_pushboolean(L, ok);
	return 1;
}

int lua_acv_modsetopt(lua_State* L) {
	int nargs = lua_gettop(L);
	if (nargs == 3) {
		char const* m = luaL_checkstring(L, 1);
		char const* o = luaL_checkstring(L, 2);
		char const* v = luaL_checkstring(L, 3);
		int ok = static_cast<acv::app*>(qApp->instance())->setopt(m, o, v);
		lua_pushboolean(L, ok);
	} else {
		lua_pushboolean(L, 0);
	}


	return 1;
}