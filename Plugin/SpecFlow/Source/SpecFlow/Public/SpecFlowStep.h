// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HttpResultCallback.h"
#include "SpecFlowStep.generated.h"

struct FHttpServerRequest;

UENUM(BlueprintType)
enum class ESpecFlowTestStep : uint8
{
	Start,
	Run,
	End,
	Finished
};
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SPECFLOW_API USpecFlowStep : public UObject
{
	GENERATED_BODY()
public:
	typedef bool (USpecFlowStep::*StepFunc)();
	
	UPROPERTY(EditAnywhere)
	FString HttpRoute;
	
	UPROPERTY(EditAnywhere)
	FString StepName;
	
public:
	void RunTestStep(ESpecFlowTestStep NextStep, StepFunc Func);
	void OnTick(float TimeDelta);
	
	virtual bool ParsePostParams(const FHttpServerRequest& Request);
	virtual bool SetupStep(const FHttpServerRequest& Request);
	
	virtual bool OnStepStart() { return true; }
	
	virtual bool OnStepRun() { return true; }
	
	virtual bool OnStepEnd() { return true; }

	bool IsStepFinished() const { return TestStep == ESpecFlowTestStep::Finished; }
	
	void SetNetworkID(const uint64 NetID) { NetworkID = FNetworkGUID::CreateFromIndex(NetID, false);}
	const FNetworkGUID& GetNetworkID() const { return NetworkID; }
	const TSharedPtr<FJsonObject>& JsonVariables() const { return JsonReponseRaw; }

protected:
	ESpecFlowTestStep TestStep = ESpecFlowTestStep::Start;
	
	void SetResponseError(const FString& Error);

	TSharedPtr<FJsonObject> PostParamsRaw;
	TSharedPtr<FJsonObject> JsonReponseRaw;
private:
	FNetworkGUID NetworkID;
};
