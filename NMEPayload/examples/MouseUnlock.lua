--[[
	Window Mouse Unlock patch by nkrapivindev.
	
	Installation:
	- add `require("MouseUnlock")` into Initial.lua
	- rerun the game
	- enjoy
]]--

-- unlock the mouse
grabMouse(false)
-- stub out the thing so the game won't call it anymore >:(
grabMouse = function(_enableFlag) end

print("MouseUnlock is ready!")

-- this module doesn't do anything, so it just returns an empty table.
return { }