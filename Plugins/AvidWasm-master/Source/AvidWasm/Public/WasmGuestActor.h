#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// 包含Wasm3的头文件
#if WITH_WASM3
#include "wasm3.h"
#endif
// 包含Wasmtime的头文件
#if WITH_WASMTIME
#include "wasmtime.h"
#endif
#include "WasmGuestActor.generated.h"

UCLASS()
class AVIDWASM_API AWasmGuestActor : public AActor
{
	GENERATED_BODY()

public:
	AWasmGuestActor();

	UFUNCTION(BlueprintCallable, Category = "Wasm")
	void TriggerWasmLogic();

public:
	UFUNCTION(BlueprintCallable, Category = "Wasm")
	int32 CallWasmAdd(int32 a, int32 b);
	
	UFUNCTION(BlueprintCallable, Category = "Wasm")
	void TestWasm3CallUE();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeWasm();

#if WITH_WASMTIME
	wasm_engine_t* Engine = nullptr;
	wasmtime_store_t* Store = nullptr;
#endif

#if WITH_WASM3
	// 【关键修正】将Environment也作为成员变量，以保证其生命周期
	IM3Environment M3Environment;

	IM3Runtime M3Runtime;
	IM3Module M3Module;
	IM3Function WasmStartFunction;
#endif
};