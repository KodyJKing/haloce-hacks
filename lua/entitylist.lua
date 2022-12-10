local entityListAddr = 0x008603b0
local entityEntrySize = 12 -- Bytes
function findEntityIndex(queryEntityPointer)
    local entityList = readInteger(entityListAddr) --0x400506E4
    local entityCountPtr = entityList + 0x2E
    local entityListArray = entityList + 0x38

    local entityCount = readSmallInteger(entityCountPtr)
    for i = 0, entityCount - 1 do
        local address = entityListArray + i * entityEntrySize + 0x08
        local entityPointer = readInteger(address)
        --print(hex(entityPointer))
        if entityPointer == queryEntityPointer then
            print("Found at index " .. hex(i))
            print("Address: " .. hex(address))
            return i
        end
    end
    print("Entity not found. Scanned " .. tostring(entityCount) .. " entities.")
    return nil
end

function getEntityRecordPointer(handle)
    local i = bAnd(handle, 0xFFFF)
    local entityList = readInteger(entityListAddr)
    local entityListArray = entityList + 0x38
    return entityListArray + i * entityEntrySize
end

function findEntityPointer(handle)
    local address = getEntityRecordPointer(handle) + 0x08
    local entityPointer = readInteger(address)
    print("Found entity pointer " .. hex(entityPointer))
    writeToClipboard(hex(entityPointer))
    return entityPointer
end

function printRecordForHandle(handle)
    local address = getEntityRecordPointer(handle)
    local a = hex(readSmallInteger(address + 0x00))
    local b = hex(readSmallInteger(address + 0x02))
    local c = hex(readSmallInteger(address + 0x04))
    local d = hex(readSmallInteger(address + 0x06))
    print(hex(handle) .. ":", a, b, c, d)
    return false
end
