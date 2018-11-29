-- --luacheck: globals import
-- Editor or test use vfs.local to manage a VFS dir/repo.
-- It read/write file from/to a repo

local localvfs = {} ; localvfs.__index = localvfs

local lfs = require "lfs"
local access = require "vfs.repoaccess"
local platform = require "platform"
local localfile = require "filesystem.file"

local function isdir(filepath)
	return lfs.attributes(filepath, "mode") == "directory"
end

--luacheck: ignore readmount
local function readmount(filename)
	local f = localfile.open(filename, "rb")
	local ret = {}
	if not f then
		return ret
	end
	for line in f:lines() do
		local name, path = line:match "^(.-):(.*)"
		if name == nil then
			f:close()
			error ("Invalid .mount file : " .. line)
		end
		ret[name] = path
	end
	f:close()
	return ret
end

-- open a repo in repopath

local cachemeta = { __mode = "kv" }
local self

local function mount_repo(mountpoint, repopath)
	local rootpath = mountpoint[''] or repopath
	local mountname = access.mountname(mountpoint)

	return {
		_mountname = mountname,
		_mountpoint = mountpoint,
		_root = rootpath,
		_cache = setmetatable({} , cachemeta),
		_repo = rootpath .. "/.repo",
	}
end

function localvfs.mount(mountpoint, enginepath)
	self = mount_repo(mountpoint, enginepath or ".")
end

function localvfs.open(repopath)
	assert(self == nil, "Can't open twice")
	if not isdir(repopath) then
		return
	end

	local mountpoint = access.readmount(repopath .. "/.mount")
	self = mount_repo(mountpoint, repopath)
	return true
end

function localvfs.realpath(pathname)
	local rp = access.realpath(self, pathname)
	local lk = rp .. ".lk"
	if lfs.exist(lk) then
		local binhash = access.build_from_path(self, platform.os(), pathname)
		return access.repopath(self, binhash)
	end
	return  rp, pathname:match "^/?(.-)/?$"
end

-- list files { name : type (dir/file) }
function localvfs.list(path)
	path = path:match "^/?(.-)/?$"
	local item = self._cache[path]
	if item then
		return item
	end
	local files = access.list_files(self, path)
	path = path .. '/'
	item = {}
	for filename in pairs(files) do
		local realpath = access.realpath(self, path .. filename)
		item[filename] = not not isdir(realpath)
	end
	self._cache[path] = item
	return item
end

function localvfs.type(filepath)
	local rp = access.realpath(self, filepath)
	local mode = lfs.attributes(rp, "mode")
	if mode then
		if mode == "directory" then
			return "dir"
		end

		if mode == "file" then
			return "file"
		end
	end
end

function localvfs.repopath()
	return self._repo
end

return localvfs

