#include "WasmGuestActor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_WASM3_INTEGRATION

// C-Style callback for Wasm3. extern "C" is crucial for correct linking.
extern "C" const void* Native_CallUEFunction(IM3Runtime runtime, M3ImportContext* _ctx, uint64* _sp, void* _userData)
{
	uint32_t messagePtr = *(_sp);
	uint32_t memorySize = 0;
	uint8_t* wasmMemory = m3_GetMemory(runtime, &memorySize, 0);

	if (wasmMemory && messagePtr < memorySize)
	{
		const char* messageFromWasm = (const char*)(wasmMemory + messagePtr);
		FString ueMessage(messageFromWasm);

		UE_LOG(LogTemp, Warning, TEXT("SUCCESS! Native_CallUEFunction was executed!"));
		UKismetSystemLibrary::PrintString(nullptr, ueMessage, true, true, FLinearColor::Red, 30.0f);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Native_CallUEFunction failed: could not read wasm memory."));
	}

	return m3Err_none;
}

#endif // WITH_WASM3_INTEGRATION


AWasmGuestActor::AWasmGuestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 【最佳实践】在构造函数中初始化所有指针成员为nullptr
#if WITH_WASM3_INTEGRATION
	M3Environment = nullptr;
	M3Runtime = nullptr;
	M3Module = nullptr;
	WasmStartFunction = nullptr;
#endif
}

void AWasmGuestActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeWasm();
}

void AWasmGuestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
#if WITH_WASM3_INTEGRATION
	// 【关键修正】现在只释放Environment即可，它会管理所有相关资源，不会再崩溃
	if (M3Environment)
	{
		m3_FreeEnvironment(M3Environment);
		M3Environment = nullptr; // good practice to null the pointer after freeing
		UE_LOG(LogTemp, Log, TEXT("WasmGuestActor: Wasm3 Environment released."));
	}
#endif
}

void AWasmGuestActor::InitializeWasm()
{
#if WITH_WASM3_INTEGRATION
	// ---- 1. 创建Wasm3环境和运行时 ----
	M3Environment = m3_NewEnvironment(); //
	if (!M3Environment) { UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to create environment.")); return; }
	
	M3Runtime = m3_NewRuntime(M3Environment, 65536, nullptr);
	if (!M3Runtime) { UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to create runtime.")); return; }

	// ---- 2. 从Content目录加载.wasm文件 ----
	TArray<uint8> WasmFileBytes;
	FString PathToWasm = FPaths::ProjectContentDir() + TEXT("release.wasm");
	if (!FFileHelper::LoadFileToArray(WasmFileBytes, *PathToWasm))
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to load .wasm file from %s"), *PathToWasm);
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Wasm3: Loaded %d bytes from wasm file."), WasmFileBytes.Num());

	// ---- 3. 解析模块 ----
	// 【Bug修正】只调用一次ParseModule，并检查返回的错误码
	M3Result parseResult = m3_ParseModule(M3Environment, &M3Module, WasmFileBytes.GetData(), WasmFileBytes.Num());
	if (parseResult != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_ParseModule failed with error: %s"), ANSI_TO_TCHAR(parseResult));
		return;
	}
	
	// ---- 4. 将模块加载到运行时 ----
	M3Result loadResult = m3_LoadModule(M3Runtime, M3Module);
	if (loadResult != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_LoadModule failed with error: %s"), ANSI_TO_TCHAR(loadResult));
		return;
	}
	//
	// // ---- 5. 链接导入函数 ----
	// M3Result linkResult = m3_LinkRawFunction(M3Module, "env", "callUEFunction", "v(i)", &Native_CallUEFunction);
	// if (linkResult != m3Err_none)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_LinkRawFunction failed with error: %s"), ANSI_TO_TCHAR(linkResult));
	// 	return;
	// }

	// ---- 6. 查找并缓存导出函数 ----
	M3Result findResult = m3_FindFunction(&WasmStartFunction, M3Runtime, "add");
	if (findResult != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_FindFunction failed with error: %s"), ANSI_TO_TCHAR(findResult));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("WasmGuestActor: Initialization successful. 'add' function is cached."));

#else
	UE_LOG(LogTemp, Warning, TEXT("WasmGuestActor: WasmRuntime plugin is disabled (WITH_WASM3_INTEGRATION=0)."));
#endif
}

void AWasmGuestActor::TriggerWasmLogic()
{
#if WITH_WASM3_INTEGRATION
	if (WasmStartFunction)
	{
		UE_LOG(LogTemp, Log, TEXT("UE -> Calling cached Wasm function 'startMyLogic'..."));
		M3Result result = m3_CallV(WasmStartFunction);
		if (result != m3Err_none)
		{
			UE_LOG(LogTemp, Error, TEXT("Wasm3: Error calling 'startMyLogic': %s"), ANSI_TO_TCHAR(result));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WasmGuestActor: Trigger failed, 'startMyLogic' function pointer is not valid. Initialization might have failed."));
	}
#endif
}

int32 AWasmGuestActor::CallWasmAdd(int32 a, int32 b)
{
#if WITH_WASM3_INTEGRATION
	if (!WasmStartFunction)
	{
		UE_LOG(LogTemp, Error, TEXT("WasmGuestActor: add function not available"));
		return 0;
	}

	// 调用 wasm 函数并传入参数
	M3Result result = m3_CallV(WasmStartFunction, a, b);
	if (result != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: Error calling 'add': %s"), ANSI_TO_TCHAR(result));
		return 0;
	}

	// 获取返回值
	uint64_t returnValue = 0;
	result = m3_GetResultsV(WasmStartFunction, &returnValue);
	if (result != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: Error getting return value: %s"), ANSI_TO_TCHAR(result));
		return 0;
	}

	int32 wasmResult = static_cast<int32>(returnValue);
	UE_LOG(LogTemp, Log, TEXT("Wasm add(%d, %d) = %d"), a, b, wasmResult);
    
	return wasmResult;
#else
	UE_LOG(LogTemp, Warning, TEXT("WasmGuestActor: WITH_WASM3_INTEGRATION is disabled"));
	return 0;
#endif
}