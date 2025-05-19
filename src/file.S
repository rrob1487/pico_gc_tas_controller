.section .bindata , "a"
.balign 4
.global g_rkg
.global g_rkg_end

g_rkg:
#.incbin "../LC_Demo.rkg"
#.incbin "../test_inputs3.dtm"
.incbin "../LuaFiles/drmariobttinputs.lua"
#.incbin "../LuaFiles/test.lua"
#.incbin "../LuaFiles/roywr.lua"
#.incbin "../LuaFiles/pichubtt.lua"
#.incbin "../LuaFiles/gawbtt.lua"
#.incbin "../LuaFiles/luigi285.lua"
.byte 0  // Null Terminator Only Needed For Lua
g_rkg_end: