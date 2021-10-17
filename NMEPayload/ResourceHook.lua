-- print out the resources the game's trying to load:

if not NIK_original_openResource and not NIK_original__GETRHCD then
	NIK_original_openResource = openResource
	NIK_original__GETRHCD = _GETRHCD
	function openResource(_type, _name)
		print("Opening resource ", _type, "|", _name)
		return NIK_original_openResource(_type, _name)
	end

	function _GETRHCD(_type, _name)
		print("Getting resource ", _type, "|", _name)
		return NIK_original__GETRHCD(_type, _name)
	end
end



