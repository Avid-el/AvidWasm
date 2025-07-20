#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#define WITH_WASM3_INTEGRATION 1
// 包含Wasm3的头文件
#if WITH_WASM3_INTEGRATION
#include "wasm3.h"
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
	void RunWasmCode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeWasm();

#if WITH_WASM3_INTEGRATION
	// 【关键修正】将Environment也作为成员变量，以保证其生命周期
	IM3Environment M3Environment;

	IM3Runtime M3Runtime;
	IM3Module M3Module;
	IM3Function WasmStartFunction;
#endif
};