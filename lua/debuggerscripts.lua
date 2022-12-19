function prints(value)
    print(value)
    return false
end

function beeps()
    beep()
    return false
end

math.randomseed(os.time())
function maybe(probability)
    return math.random() < probability
end

function scanRegisters(query)
    local registers = {
        EAX = EAX, EBX = EBX, ECX = ECX,
        EDX = EDX, EDI = EDI, ESI = ESI,
        EBP = EBP, ESP = ESP, EIP = EIP
    }
    for name, value in pairs(registers) do
        if value == query then
            --print("Found value in register " .. name)
            return true
        end
    end
    return false
end

function scanStack(query, maxOffset, stride)
    for offset = 0, maxOffset, stride do
        local value = readInteger(ESP + offset)
        if value == query then
            print("Found value at offset " .. hex(offset))
            return true
        end
    end
    return false
end

function scanForValue(query, maxOffset, stride)
    if scanRegisters(query) then return true end
    return scanStack(query, maxOffset, stride)
end

-------------------------------------------------------------------

function traceCallPath(address, steps)
    local step = 0
    local printNextEIP = false
    debug_setBreakpoint(address, function()
        debug_removeBreakpoint(address)

        if printNextEIP then print(EIP) end
        printNextEIP = false

        local dString = disassemble(EIP)
        local extra, opcode, bytes, address = splitDisassembledString(dString)

        --print(tostring(step) .. " " .. opcode)
        if opcode == "ret" then
            printNextEIP = true
        end

        if step == steps then
            debug_continueFromBreakpoint(co_run)
        else
            debug_continueFromBreakpoint(co_stepinto)
        end

        step = step + 1
    end)
end
