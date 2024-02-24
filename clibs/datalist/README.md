## 数据列表

这是我们游戏引擎使用的一种数据格式。它类似于更简单的 YAML 或者增强版的 JSON。

### 简单字典

键应为不带空格的字符串，可以使用 `:` 或 `=` 来分隔值。

```lua
a = datalist.parse [[
x : 1 # 注释 ...
y = 2 # = 等同于 :
100 : number
]]

-- a = { x = 1, y = 2, ["100"] = "number" }
```

或者您可以使用 `datalist.parse_list` 将字典解析为带有键值对的列表。
```lua
a = datalist.parse_list [[
x : 1
y : 2
]]

-- a = { x , 1, y , 2 }
```

### 简单列表

使用空格（空格、制表符、回车、换行等）来分隔元素。
```lua
a = datalist.parse[[
hello "world"
0x1p+0 # 十六进制浮点数 1.0
2 
0x3 # 十六进制整数
nil
true 
false
on  # true
off # false
yes # true
no  # false
]]

-- a = { "hello", "world", 1.0, 2, 3, nil, true, false, true, false, true, false }
```

### 分节列表

`---` 可以用来分隔列表的不同部分。
```lua
a = datalist.parse [[
---
x : hello
y : world
---
1 2 3
]]

-- a = { { x = "hello", y = "world" }, { 1,2,3 } }
```

### 使用缩进或 `{}` 来描述多层结构

```lua
a = datalist.parse [[
x :
  1 2 3
y :
  dict : "hello world"
z : { foobar }
]]

-- a = {  x = { 1,2,3 },  y = { dict = "hello world" }, z = { "foobar" } }

b = datalist.parse [[
---
hello world
  ---
  x : 1
  y : 2
]]

-- a = { "hello", "world", { x = 1, y = 2 } }
```

### 使用标签引用结构

标签是一个 64 位十六进制整数 ID。

```lua
a = datalist.parse [[
--- &1   # 用 1 标记了这个结构
"hello\nworld"
---
x : *1   # 值是标记为 1 的结构
]]

-- a = { { "hello\nworld" } , { x = { "hello\nworld" } } }
```

### 转换器

方括号中的结构将由用户函数转换。

```lua
a = datalist.parse( "[ 1, 2, 3 ]" , function (t)
  local s = 0
  for _, v in ipairs(t) do  -- t = { 1,2,3 }
    s = s + v
  end
  return s)
  
-- a = { 6 }

a = datalist.parse([[
[ sum 1 2 3 ]
[ vec 4 5 6 ]
]], function (t)
  if t[1] == "sum" then
    local s = 0
    for i = 2, #t do
      s = s + t[i]
    end
  elseif t[2] == "vec" then
    table.remove(t, 1)
  end
  return t)
  
-- a = { 6 , { 4,5,6 } }
```