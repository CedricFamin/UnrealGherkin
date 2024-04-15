// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecFlowStep.h"
#include "HttpServerRequest.h"
#include "Commandlets/GatherTextCommandlet.h"

void USpecFlowStep::RunTestStep(ESpecFlowTestStep NextStep, StepFunc Func)
{
	if (bool bProceedToNextStep = (this->*(Func))())
	{
		TestStep = NextStep;
	}
}

void USpecFlowStep::OnTick(float TimeDelta)
{
	switch (TestStep)
	{
	case ESpecFlowTestStep::Start:
		RunTestStep(ESpecFlowTestStep::Run, &USpecFlowStep::OnStepStart);
		break;
	case ESpecFlowTestStep::Run:
		RunTestStep(ESpecFlowTestStep::End, &USpecFlowStep::OnStepRun);
		break;
	case ESpecFlowTestStep::End:
		RunTestStep(ESpecFlowTestStep::Finished, &USpecFlowStep::OnStepEnd);
		break;
	case ESpecFlowTestStep::Finished:
	default:
		ensure(false);
		break;
	}
}

bool USpecFlowStep::ParsePostParams(const FHttpServerRequest& Request)
{
	const FString BodyAsString = FString(reinterpret_cast<const char*>(Request.Body.GetData()), Request.Body.Num());
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyAsString);
	
	if (FJsonSerializer::Deserialize(Reader, PostParamsRaw))
	{
		return true;
	}

	return false;
}

bool USpecFlowStep::SetupStep(const FHttpServerRequest& Request)
{
	PostParamsRaw = MakeShared<FJsonObject>();
	JsonReponseRaw = MakeShared<FJsonObject>();

	if (!ParsePostParams(Request))
	{
		SetResponseError("Can't parse body.");
		return false;
	}
	
	return true;
}

void USpecFlowStep::SetResponseError(const FString& Error)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField("Error", Error);
}
