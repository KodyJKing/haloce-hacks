function getMemoryInfo(address)
    local m = createMemoryStream()
    m.size = 8 * 8
    local returnedBytes = executeCodeLocalEx("VirtualQuery", address, m.Memory, m.size)
    if returnedBytes == 0 then
        m.destroy();
        return nil
    end
    local result = {
        BaseAddress = m.readQword(),
        AllocationBase = m.readQword(),
        AllocationProtect = m.readDword(),
        PartitionId = m.readDword(),
        RegionSize = m.readQword(),
        State = m.readDword(),
        Protect = m.readDword(),
        Type = m.readDword(),
    }
    m.destroy();
    return result
end

local PAGE_EXECUTE = 0x10
local PAGE_EXECUTE_READ = 0x20
local PAGE_EXECUTE_READWRITE = 0x40
local PAGE_EXECUTE_WRITECOPY = 0x80
local PAGE_NOACCESS = 0x01
local PAGE_READONLY = 0x02
local PAGE_READWRITE = 0x04
local PAGE_WRITECOPY = 0x08

function isExecutable(protect)
    return protect == PAGE_EXECUTE or
        protect == PAGE_EXECUTE_READ or
        protect == PAGE_EXECUTE_READWRITE or
        protect == PAGE_EXECUTE_WRITECOPY
end

function isAddresExecutable(address)
    local info = getMemoryInfo(address)
    if info then
        return isExecutable(info.Protect)
    end
    return false
end
