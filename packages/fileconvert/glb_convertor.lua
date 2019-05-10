local glTF = import_package "ant.glTF"
local glbloader = glTF.glb
local gltfloader = glTF.gltf

local gltf_converter = require "gltf.converter"

local componenttype_bgfxtype_maper = {
	[5120] = "", 		--"BYTE",
	[5121] = "UINT8", 	--"UNSIGNED_BYTE",
	[5122] = "INT16", 	--"SHORT",
	[5123] = "", 		--UNSIGNED_SHORT",
	[5125] = "", 		--"UNSIGNED_INT",
	[5126] = "FLOAT",	--"FLOAT"
}

local accessor_types = {
	SCALAR = 0,
	VEC2 = 1,
	VEC3 = 2,
	VEC4 = 3,
	MAT2 = 4,
	MAT3 = 5,
	MAT4 = 6,
}

local function find_accessor_type(t)
	for k, v in pairs(accessor_types) do
		if v == t then
			return k
		end
	end
end

local ENUM_ARRAY_BUFFER = 34962
local ENUM_ELEMENT_ARRAY_BUFFER = 34963


local bufferview_sizebytes = 4 + 4 + 4 + 4

local function is_4_byte_align(num)
	if num // 4 ~= num / 4 then
		error("not 4 byte align")
	end
end

is_4_byte_align(bufferview_sizebytes)

local function compile_bufferview(bv)
	return string.pack("<I4I4I4I4",
		bv.byteoffset, 
		bv.bytelength,
		bv.stride or 0,
		bv.target or ENUM_ARRAY_BUFFER)
end

local function compile_number_array(numberarray)
	local num = #numberarray
	for _=num+1, 16 do
		numberarray[#numberarray+1] = 0
	end
	assert(#numberarray == 16)
	return string.pack("<I4ffffffffffffffff", num, table.unpack(numberarray))
end

local accessor_sizebytes = (4 + 4 + 4 + 1 + 1 + 1 + 1) + (4 + 16 * 4) + (4 + 16 * 4)
is_4_byte_align(accessor_sizebytes)

local function compile_accessor(accessor, new_bvidx)
	return string.pack("<I4I4I4I1I1I1I1", 
		new_bvidx,
		accessor.byteoffset,
		accessor.componenttype,
		accessor.normalized and 1 or 0,
		accessor.count,
		accessor_types[accessor.type],
		0)	-- padding data for 4 bytes align
		.. compile_number_array(accessor.min or {})
		.. compile_number_array(accessor.max or {})
end

local attribname_mapper = {
	POSITION = 0,

	NORMAL = 1,
	TANGENT = 2,
	BITANGENT = 3,
	BINORMAL = 3,

	COLOR = 4,
	COLOR_0 = 4,
	COLOR_1 = 5,
	COLOR_2 = 6,
	COLOR_3 = 7,

	TEXCOORD_0 = 8,
	TEXCOORD_1 = 9,
	TEXCOORD_2 = 10,
	TEXCOORD_3 = 11,
	TEXCOORD_4 = 12,
	TEXCOORD_5 = 13,
	TEXCOORD_6 = 14,
	TEXCOORD_7 = 15,

	WEIGHT = 16,
	INDICES = 17,
}

local function compile_primitive(scene, primitive)
	local attributes = primitive.attributes

	local accessors = scene.accessors
	local bufferviews = scene.bufferviews

	local seri_attrib = {}
	local seri_accessor = {}
	local seri_bufferview = {}

	local function serialize_accessor(accessor)
		local new_bvidx = #seri_bufferview+1
		seri_accessor[#seri_accessor+1] = compile_accessor(accessor, new_bvidx-1)
		seri_bufferview[#seri_bufferview+1] = compile_bufferview(bufferviews[accessor.buffview])
	end

	for name, accidx in pairs(attributes) do
		local accessor = accessors[accidx]
		local attribidx = attribname_mapper[name]
		seri_attrib[#seri_attrib+1] = string.pack("<I4I4", attribidx, #seri_accessor)

		serialize_accessor(accessor)
	end

	if primitive.indices then
		serialize_accessor(accessors[primitive.indices])
	end

	local function concat_table(t)
		return string.pack("<I4", #t) .. table.concat(t, "")
	end

	return concat_table(seri_attrib) .. 
			string.pack("<I4", primitive.indices or 0xffffffff) .. 
			concat_table(seri_accessor) .. 
			concat_table(seri_bufferview)
end

local function find_attrib_name(attribname_idx)
	for k, v in pairs(attribname_mapper)do
		if v == attribname_idx then
			return k
		end
	end
end

local function deserialize_bufferview(seri_data, seri_offset)
	local bv = {}
	bv.byteoffset, bv.bytelength, bv.stride, bv.target = 
	string.unpack("<I4I4I4I4", seri_data, seri_offset)	
	seri_offset = seri_offset + 4 + 4 + 4 + 4
	return bv, seri_offset
end

local function deserialize_accessor(seri_data, seri_offset)
	local acc = {}
	acc.bufferview, acc.byteoffset,	acc.componenttype,	
	acc.normalized,	acc.count,	acc.type = 
	string.unpack("<I4I4I4I1I1I1I1", seri_data, seri_offset)
	
	seri_offset = seri_offset + 4 + 4 + 4 + 1 + 1 + 1 + 1

	acc.normalized = acc.normalized ~= 0 and true or false
	acc.type = find_accessor_type(acc.type)

	local function unpack_array(seri_data, seri_offset)
		local array_fmt = "<I4ffffffffffffffff"
		local nummin, min = string.unpack(array_fmt, seri_data, seri_offset)
		local t = {}
		for i = 1, nummin do
			t[i] = min[1]
		end
		return t
	end

	acc.min = unpack_array(seri_data, seri_offset)
	seri_offset = seri_offset + 4 + 16 * 4
	acc.max = unpack_array(seri_data, seri_offset)
	seri_offset = seri_offset + 4 + 16 * 4
	return acc, seri_offset
end

local function deserialize_primitive_itself(seri_data, seri_offset)
	local numattrib = string.unpack("<I4", seri_data, seri_offset)
	seri_offset = seri_offset + 4

	local prim = {}

	for _=1, numattrib do
		local attribidx, accidx = string.unpack("<I4I4", seri_data, seri_offset)
		local name = find_attrib_name(attribidx)
		prim[name] = accidx
		seri_offset = seri_offset + 4 + 4
	end

	local index_buffer_idx = string.unpack("<I4", seri_data, seri_offset)
	seri_offset = seri_offset + 4
	prim.indices = index_buffer_idx ~= 0xffffffff and index_buffer_idx or nil
	return prim, seri_offset
end

local function deserialize_primitive(seri_data)	
	local prim, seri_offset = deserialize_primitive_itself(seri_data, 0)

	local accessors = {}	
	local num_accessor = string.unpack("<I4", seri_data, seri_offset)
	seri_offset = seri_offset + 4
	for ii=1, num_accessor do
		accessors[ii], seri_offset = deserialize_accessor(seri_data, seri_offset)
	end

	local bufferviews = {}	
	local num_bufferview = string.unpack("<I4", seri_data, seri_offset)
	seri_offset = seri_offset + 4
	for ii=1, num_bufferview do
		bufferviews[ii], seri_offset = deserialize_bufferview(seri_data, seri_offset)
	end

	return prim, accessors, bufferviews
end

return function (srcname, dstname, cfg)
	local version, jsondata, bindata = glbloader.decode(srcname)
	local scene = gltfloader.decode(jsondata)

	local scenes, nodes, meshes = scene.scenes, scene.nodes, scene.meshes

	local new_bindata_table = {}
	local bindata_offset = 0

	local new_accessors = {}
	local new_bufferviews = {}	
	
	local function refine_prim_offset(newprim, newacc, newbvs)
		local accessor_index_offset = #new_accessors
		local bufferview_index_offset = #new_bufferviews

		for _, acc in ipairs(newacc) do
			acc.bufferview = acc.bufferview + bufferview_index_offset
		end

		local newattributes = newprim.attributes
		for k, v in pairs(newattributes) do
			newattributes[k] = v + accessor_index_offset
		end

		for _, bv in ipairs(newbvs) do
			bv.offset = bv.offset + bindata_offset
		end
	end

	local function fetch_mesh_buffers(scenenodes)
		for _, nodeidx in ipairs(scenenodes) do
			local node = nodes[nodeidx + 1]
			if node.children then
				fetch_mesh_buffers(node.children)
			end
			
			if node.mesh then
				local meshidx = node.mesh + 1
				local mesh = meshes[meshidx]
				local primitives = mesh.primitives
				for idx, prim in ipairs(primitives) do
					local seri_prim = compile_primitive(scene, prim)
					local new_seri_prim, prim_binary_buffers = gltf_converter.convert_buffers(seri_prim, bindata, cfg)
					local newprim, newacc, newbvs = deserialize_primitive(new_seri_prim)

					refine_prim_offset(newprim, newacc, newbvs)

					primitives[idx] = newprim
					table.move(newacc, 1, #newacc, #new_accessors+1, new_accessors)
					table.move(newbvs, 1, #newbvs, #new_bufferviews+1, new_bufferviews)

					new_bindata_table[#new_bindata_table+1] = prim_binary_buffers
					bindata_offset = bindata_offset + #prim_binary_buffers
				end
			end
		end
	end

	fetch_mesh_buffers(scenes[scene.scene])
	
	local new_bindata = table.concat(new_bindata_table, "")

	local newscene = {
		scene = scene.scene,
		scenes = scenes,
		nodes = nodes,
		meshes = meshes,
		accessors = new_accessors,
		bufferviews = new_bufferviews,
		buffers = {
			{bytelength = #new_bindata,}
		},
	}

	local new_jsondata = gltfloader.encode(newscene)
	glbloader.encode(dstname, version, new_jsondata, new_bindata)

end