

# Hacks for the Gearbox port of Halo CE, v1.0.10 (not Custom Edition)

## Building

Install the D3D9 SDK, make sure the environment variable `DXSDK_DIR` is set and build with Visual Studio 19+

## Running

Use a DLL injector to inject `aimbot.dll` from `dlls/Debug`. I use Cheat Engine's injector.

## Debug an injected DLL in Visual Studio:
1. Make sure to enable PDB output.
2. Debug > Atatch to Process (CTRL+ALT+P) ***or*** Debug > Reattach to Process (SHIFT+ALT+P)


## Enable / Disable PDB Output in Visual Studio:
You'll need to enable PDB output to debug in Visual Studio.

Depending on your DLL injector, you may need to disable PDB output to rebuild after injecting.
Cheat Engine's injector holds locks on these files even after the DLL unloads and will prevent recompiling.
```
[project] > Properties > Linker > Debugging > Generate Debug Info
```

You can also just close and reattach halo.exe in Cheat Engine to unload the PDB file. There's a script in the cheat table to do this mapped to F9.