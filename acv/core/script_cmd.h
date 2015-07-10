#ifndef ACV_SCRIPT_CMD_H
#define ACV_SCRIPT_CMD_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


void lua_register_cmds(lua_State* L);
int lua_acv_version(lua_State* L);
int lua_acv_modstart(lua_State* L);
int lua_acv_modsetopt(lua_State* L);

#endif //ACV_SCRIPT_CMD_H
