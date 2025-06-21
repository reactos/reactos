@ stdcall -private D3DAssemble(ptr long str ptr ptr long ptr ptr)
@ stdcall D3DCompile(ptr long str ptr ptr str str long long ptr ptr)
@ stdcall D3DCompile2(ptr long str ptr ptr str str long long long ptr long ptr ptr)
@ stdcall D3DCompileFromFile(wstr ptr ptr str str long long ptr ptr)
@ stub D3DCompressShaders
@ stdcall D3DCreateBlob(long ptr)
@ stub D3DCreateFunctionLinkingGraph
@ stdcall D3DCreateLinker(ptr)
@ stub D3DDecompressShaders
@ stdcall D3DDisassemble(ptr long long ptr ptr)
@ stub D3DDisassemble10Effect(ptr long ptr)
@ stub D3DDisassemble11Trace
@ stub D3DDisassembleRegion
@ stdcall D3DGetBlobPart(ptr long long long ptr)
@ stdcall D3DGetDebugInfo(ptr long ptr)
@ stdcall D3DGetInputAndOutputSignatureBlob(ptr long ptr)
@ stdcall D3DGetInputSignatureBlob(ptr long ptr)
@ stdcall D3DGetOutputSignatureBlob(ptr long ptr)
@ stub D3DGetTraceInstructionOffsets
@ stdcall D3DLoadModule(ptr long ptr)
@ stdcall D3DPreprocess(ptr long str ptr ptr ptr ptr)
@ stdcall D3DReadFileToBlob(wstr ptr)
@ stdcall D3DReflect(ptr long ptr ptr)
@ stub D3DReflectLibrary
@ stub D3DReturnFailure1
@ stub D3DSetBlobPart
@ stdcall D3DStripShader(ptr long long ptr)
@ stdcall D3DWriteBlobToFile(ptr wstr long)
@ stub DebugSetMute
