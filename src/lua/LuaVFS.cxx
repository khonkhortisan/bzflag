
#include "common.h"

#include "LuaVFS.h"

#include <string>
#include <vector>
#include <map>

using namespace std;

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "BzVFS.h"


static BzDocket* luaWorldFS = NULL;


/******************************************************************************/
/******************************************************************************/

bool LuaVFS::PushEntries(lua_State* L)
{
  PUSH_LUA_CFUNC(L, FileExists);
  PUSH_LUA_CFUNC(L, FileSize);
  PUSH_LUA_CFUNC(L, ReadFile);
  PUSH_LUA_CFUNC(L, Include);
  PUSH_LUA_CFUNC(L, DirList);
  return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaVFS::FileExists(lua_State* L)
{
  const LuaHandle* lh = LuaHandle::GetActiveHandle();

  const char* path = luaL_checkstring(L, 1);
  string modes = luaL_optstring(L, 2, lh->GetFSRead().c_str());
  modes = BzVFS::filterModes(modes, lh->GetFSReadAll().c_str());

  lua_pushboolean(L, bzVFS.fileExists(path, modes));

  return 1;
}


int LuaVFS::FileSize(lua_State* L)
{
  const LuaHandle* lh = LuaHandle::GetActiveHandle();

  const char* path = luaL_checkstring(L, 1);
  string modes = luaL_optstring(L, 2, lh->GetFSRead().c_str());
  modes = BzVFS::filterModes(modes, lh->GetFSReadAll().c_str());

  const int size = bzVFS.fileSize(path, modes);
  if (size < 0) {
    return 0;
  }
    
  lua_pushinteger(L, size);

  return 1;
}


int LuaVFS::ReadFile(lua_State* L)
{
  const LuaHandle* lh = LuaHandle::GetActiveHandle();

  const char* path = luaL_checkstring(L, 1);
  string modes = luaL_optstring(L, 2, lh->GetFSRead().c_str());
  modes = BzVFS::filterModes(modes, lh->GetFSReadAll().c_str());

  string data;
  if (!bzVFS.readFile(path, modes, data)) {
    return 0;
  }
  lua_pushlstring(L, data.data(), data.size());
  return 1;
}


int LuaVFS::Include(lua_State* L)
{
  const LuaHandle* lh = LuaHandle::GetActiveHandle();

  lua_settop(L, 3);
  const char* path = luaL_checkstring(L, 1);
  string modes = luaL_optstring(L, 2, lh->GetFSRead().c_str());
  modes = BzVFS::filterModes(modes, lh->GetFSReadAll().c_str());

  string code;
  if (!bzVFS.readFile(path, modes, code)) {
    luaL_error(L, "file not found");
    return 0;
  }

  // compile the code
  int error = luaL_loadbuffer(L, code.data(), code.size(), path);
  if (error != 0) {
    lua_error(L);
  }

  if (lua_istable(L, 3)) {
    lua_pushvalue(L, 3);
  }
  else if (lua_isnil(L, 3)) {
    LuaUtils::PushCurrentFuncEnv(L, "Include");
  }
  else {
    luaL_error(L, "bad fenv parameter");
  }

  if (lua_setfenv(L, -2) == 0) {
    luaL_error(L, "Include: error with setfenv");
  }

  const int paramTop = lua_gettop(L) - 1;

  // execute the code
  error = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (error != 0) {
    lua_error(L);
  }

  return lua_gettop(L) - paramTop;
}


int LuaVFS::DirList(lua_State* L)
{
  const LuaHandle* lh = LuaHandle::GetActiveHandle();

  const char* path = luaL_checkstring(L, 1);
  string modes = luaL_optstring(L, 2, lh->GetFSRead().c_str());
  modes = BzVFS::filterModes(modes, lh->GetFSReadAll().c_str());
  const bool recursive = lua_isboolean(L, 3) && lua_tobool(L, 3);

  vector<string> files;
  vector<string> dirs;
  if (!bzVFS.dirList(path, modes, recursive, dirs, files)) {
    return 0;
  }

  lua_createtable(L, files.size(), 0);
  for (size_t i = 0; i < files.size(); i++) {
    const string& fpath = files[i];
    lua_pushinteger(L, i + 1);
    lua_pushlstring(L, fpath.data(), fpath.size());
    lua_rawset(L, -3);
  }

  lua_createtable(L, dirs.size(), 0);
  for (size_t i = 0; i < dirs.size(); i++) {
    const string& dpath = dirs[i];
    lua_pushinteger(L, i + 1);
    lua_pushlstring(L, dpath.data(), dpath.size());
    lua_rawset(L, -3);
  }

  return 2;
}


/******************************************************************************/
/******************************************************************************/
