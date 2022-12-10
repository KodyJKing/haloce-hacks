function reloadPackage(path)
    package.loaded[path] = nil
    return require(path)
end

dofile("lua/debuggerscripts.lua")
dofile("lua/virtualmemory.lua")
dofile("lua/entitylist.lua")

local json = reloadPackage("lua/json")

function pprint(value)
    print(json.encode(value))
end

function hex(x)
    return string.format("%x", x):upper()
end

function getCWD()
    return io.popen "cd":read '*l'
end

function clearConsole()
    GetLuaEngine().MenuItem5.doClick()
end

function checkSymbol(symbol)
    if not pcall(function() getAddress(symbol) end) then return false end
    local addr = getAddress(symbol)
    if addr == nil then return false end
    local value = readInteger(addr)
    if value == nil then return false end
    return true
end

function tryInjectDll(memrec)
    local dllName = memrec.Description .. ".dll"
    local dllPath = getCWD() .. "\\dlls\\Debug\\" .. dllName
    if checkSymbol(dllName) then
        beep()
    else
        injectDLL(dllPath)
    end
    createTimer(1, function() memrec.Active = false end)
end

function reopenProcess(memrec)
    openProcess(getOpenedProcessID())
    createTimer(1, function() memrec.Active = false end)
end
