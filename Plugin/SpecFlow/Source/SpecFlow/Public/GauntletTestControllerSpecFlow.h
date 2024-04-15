// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GauntletTestController.h"
#include "HttpResultCallback.h"

#include "GauntletTestControllerSpecFlow.generated.h"

struct FHttpServerResponse;
class IHttpRouter;
class USpecFlowStep;

USTRUCT()
struct FFinishedStep
{
	GENERATED_BODY();
	
	FNetworkGUID NetworkID;
	TSharedPtr<FJsonObject> ResponseVariables;
};

USTRUCT()
struct FCommonSpecFlowStepResponse
{
	GENERATED_BODY();
	FCommonSpecFlowStepResponse();

	template<typename T>
	struct JsonValueConverter
	{
		typedef TFunction<TSharedPtr<FJsonValue>(const T&)> Func;
	};
	
	template<typename T>
	TSharedPtr<FJsonObject> AddField(const FString& Name, const TArray<T>& Array, JsonValueConverter<T>::Func Func)
	{
		TArray<TSharedPtr<FJsonValue>> JsonValueItems;
		for (const T& Item : Array)
		{
			JsonValueItems.Emplace(Func(Item));
		}
		
		JsonObject->SetArrayField(Name, JsonValueItems);
		return JsonObject;
	}

	template<typename T>
	TSharedPtr<FJsonObject> AddField(const FString& Name, const T& Item, JsonValueConverter<T>::Func Func)
	{
		JsonObject->SetField(Name, Func(Item));
		return JsonObject;
	}

	template<typename T>
	TSharedPtr<FJsonObject> AddNumberField(const FString& Name, const T& Item)
	{
		JsonObject->SetField(Name, MakeShared<FJsonValueNumber>(Item));
		return JsonObject;
	}

	TSharedPtr<FJsonObject> AddStringField(const FString& Name, const FString& Item)
	{
		JsonObject->SetField(Name, MakeShared<FJsonValueString>(Item));
		return JsonObject;
	}

	TSharedPtr<FJsonObject> AddDataField(const FString& Name, const TObjectPtr<USpecFlowStep>& Item);
	
	void SendResponse(const FHttpResultCallback& OnComplete) const;
	TSharedPtr<FJsonObject> JsonObject;
};
/**
 * 
 */
UCLASS()
class SPECFLOW_API UGauntletTestControllerSpecFlow : public UGauntletTestController
{
	GENERATED_BODY()

public:
	virtual void OnInit() override;
	virtual void OnPreMapChange() override;
	virtual void OnPostMapChange(UWorld* World) override;
	virtual void OnTick(float TimeDelta) override;
	virtual void OnStateChange(FName OldState, FName NewState) override;

private:
	void BindManagementRoute();
	void RegisterNewStep(USpecFlowStep* Step);
	void LoadNativeSteps();
	void LoadSpecFlowSteps();
	void LoadBlueprintSpecFlowSteps();
	
private:
	uint64 NetworkGuidIndex = 0;
	
	TSharedPtr<IHttpRouter> HttpRouter;

	UPROPERTY()
	TArray<FFinishedStep> FinishedSteps;
	
	UPROPERTY()
	TArray<TObjectPtr<USpecFlowStep>> Steps;

	UPROPERTY()
	TObjectPtr<USpecFlowStep> CurrentStep = nullptr;
	
};
