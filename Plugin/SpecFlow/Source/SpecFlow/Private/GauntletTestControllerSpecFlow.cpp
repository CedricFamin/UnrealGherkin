// Fill out your copyright notice in the Description page of Project Settings.


#include "GauntletTestControllerSpecFlow.h"
#include "SpecFlowStep.h"
#include "Steps/SpecFlowStep_Simple.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "HttpServerRequest.h"
#include "IHttpRouter.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "SpecFlow.h"

FCommonSpecFlowStepResponse::FCommonSpecFlowStepResponse()
{
	JsonObject = MakeShared<FJsonObject>();
}

TSharedPtr<FJsonObject> FCommonSpecFlowStepResponse::AddDataField(const FString& Name, const TObjectPtr<USpecFlowStep>& Item)
{
	JsonObject->SetField(Name, MakeShared<FJsonValueObject>(Item->JsonVariables()));
	return JsonObject;
}

void FCommonSpecFlowStepResponse::SendResponse(const FHttpResultCallback& OnComplete) const
{
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Code = EHttpServerResponseCodes::BadRequest;
	Response->Headers.FindOrAdd("content-type", { "Application/json" });

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		Response->Body.Append(Body.GetCharArray());
	}
	OnComplete(MoveTemp(Response));
}

void UGauntletTestControllerSpecFlow::OnInit()
{
	Super::OnInit();
	HttpRouter = FHttpServerModule::Get().GetHttpRouter(9876, /* bFailOnBindFailure = */ true);
	FHttpServerModule::Get().StartAllListeners();
	BindManagementRoute();
	LoadBlueprintSpecFlowSteps();
	LoadSpecFlowSteps();
}

void UGauntletTestControllerSpecFlow::OnPreMapChange()
{
	Super::OnPreMapChange();
}

void UGauntletTestControllerSpecFlow::OnPostMapChange(UWorld* World)
{
	Super::OnPostMapChange(World);
}

void UGauntletTestControllerSpecFlow::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);
	if (CurrentStep != nullptr)
	{
		CurrentStep->OnTick(TimeDelta);
		if (CurrentStep->IsStepFinished())
		{
			FFinishedStep EndInfo;
			EndInfo.NetworkID = CurrentStep->GetNetworkID();
			EndInfo.ResponseVariables = CurrentStep->JsonVariables();
			FinishedSteps.Emplace(EndInfo);
			CurrentStep = nullptr;
		}
	}
}

void UGauntletTestControllerSpecFlow::OnStateChange(FName OldState, FName NewState)
{
	Super::OnStateChange(OldState, NewState);
}

void UGauntletTestControllerSpecFlow::BindManagementRoute()
{
	HttpRouter->BindRoute(FHttpPath(TEXT("/SpecFlow/Steps")), EHttpServerRequestVerbs::VERB_POST
		, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) -> bool
		{
			TUniquePtr<FCommonSpecFlowStepResponse> Response = MakeUnique<FCommonSpecFlowStepResponse>();
			if (!Steps.IsEmpty())
			{
				Response->AddStringField("Warning", "Steps already loaded.");
			}
			
			Response->AddNumberField("StepLoaded", Steps.Num());
			Response->SendResponse(OnComplete);
			return true;
		});
	
	HttpRouter->BindRoute(FHttpPath(TEXT("/SpecFlow/Steps")), EHttpServerRequestVerbs::VERB_GET
		, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) -> bool
		{
			TUniquePtr<FCommonSpecFlowStepResponse> Response = MakeUnique<FCommonSpecFlowStepResponse>();
			
			Response->AddField("Steps", Steps,
			[](const TObjectPtr<USpecFlowStep>& Step) -> TSharedPtr<FJsonValue>
			{
				TSharedPtr<FJsonObject> JsonStep = MakeShared<FJsonObject>();
				JsonStep->SetStringField("Name", Step->StepName);
				JsonStep->SetStringField("HttpRoute", Step->HttpRoute);
				return MakeShared<FJsonValueObject>(JsonStep);
			});
			
			Response->SendResponse(OnComplete);
			return true;
		});

	HttpRouter->BindRoute(FHttpPath(TEXT("/SpecFlow/Finished")), EHttpServerRequestVerbs::VERB_GET
		, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) -> bool
		{
			TUniquePtr<FCommonSpecFlowStepResponse> Response = MakeUnique<FCommonSpecFlowStepResponse>();
			
			Response->AddField("Steps", FinishedSteps,
			[](const FFinishedStep& Info) -> TSharedPtr<FJsonValue>
			{
				return MakeShareable(new FJsonValueNumber(Info.NetworkID.ObjectId));
			});
			
			Response->SendResponse(OnComplete);
			return true;
		});
}


void UGauntletTestControllerSpecFlow::RegisterNewStep(USpecFlowStep* Step)
{
	Steps.Emplace(Step);
	HttpRouter->BindRoute(FHttpPath(Step->HttpRoute), EHttpServerRequestVerbs::VERB_POST
		, [this, Step](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) -> bool
		{
			USpecFlowStep* NewStep = DuplicateObject(Step, GetTransientPackage());
			NewStep->SetNetworkID(NetworkGuidIndex++);
			ensure(CurrentStep == nullptr);
			CurrentStep = NewStep;

			TUniquePtr<FCommonSpecFlowStepResponse> Response = MakeUnique<FCommonSpecFlowStepResponse>();
			Response->AddNumberField("StepID", NewStep->GetNetworkID().ObjectId);
			
			if (!CurrentStep->SetupStep(Request))
			{
				Response->AddStringField("Error", "Can't setup test step.");
				Response->AddDataField("Data", CurrentStep);
				UE_LOG(LogSpecFlow, Error, TEXT("Can't setup test step: %s, class: %s"), *CurrentStep->StepName, *CurrentStep->StaticClass()->GetName());
			}
			UE_LOG(LogSpecFlow, Display, TEXT("Set current step: %s"), *CurrentStep->StepName);
			
			Response->SendResponse(OnComplete);
			return true;
		});
}

void UGauntletTestControllerSpecFlow::LoadSpecFlowSteps()
{
	LoadNativeSteps();
	//LoadBlueprintSpecFlowSteps();
}

void UGauntletTestControllerSpecFlow::LoadNativeSteps()
{
	RegisterNewStep(USpecFlowStep_Simple::FromLambda("IsSpecFLowPluginInitialized", [this]() -> bool
		{
			if (Steps.IsEmpty())
			{
				return false;
			}
			
			return true;
		}));

	RegisterNewStep(USpecFlowStep_Simple::FromLambda("LoadBlueprintSteps", [this]() -> bool
		{
			if (Steps.IsEmpty())
			{
				LoadBlueprintSpecFlowSteps();
			}
			
			return true;
		}));
}

void UGauntletTestControllerSpecFlow::LoadBlueprintSpecFlowSteps()
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetsData;
	FARFilter Filter;
	Filter.PackagePaths.Emplace(TEXT("/Game/Test/"));
	Filter.ClassPaths.Emplace(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	AssetRegistryModule.Get().GetAssets(Filter, AssetsData);
	for (FAssetData AssetData : AssetsData)
	{
		UObject* Asset = AssetData.GetAsset();
		const UBlueprint* BlueprintAsset = Cast<UBlueprint>(Asset);

		if (!BlueprintAsset || !BlueprintAsset->GeneratedClass)
		{
			continue;
		}

		if (!BlueprintAsset->GeneratedClass->IsChildOf(USpecFlowStep::StaticClass()))
		{
			continue;
		}

		USpecFlowStep* Step = NewObject<USpecFlowStep>(GetTransientPackage(), BlueprintAsset->GeneratedClass);
		if (!Step)
		{
			continue;
		}
		RegisterNewStep(Step);
	}
}
